package httpcontroller

import (
	"encoding/json"
	"errors"
	"fmt"
	"html/template"
	"net/http"
	"strconv"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	"github.com/waviate-script/marketplace/backend/internal/desktop"
	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

type Router struct {
	service     *service.Service
	auth        auth.Middleware
	authManager *auth.Manager
}

func NewRouter(service *service.Service, authMiddleware auth.Middleware, authManager *auth.Manager, adminEnabled bool) http.Handler {
	router := Router{
		service:     service,
		auth:        authMiddleware,
		authManager: authManager,
	}

	mux := http.NewServeMux()
	mux.HandleFunc("GET /healthz", router.health)
	mux.HandleFunc("GET /api/search", router.search)
	mux.HandleFunc("GET /api/tags", router.tags)
	mux.HandleFunc("POST /api/source/compile-check", router.compileCheck)
	mux.Handle("POST /api/uploads", authMiddleware.RequireUser(http.HandlerFunc(router.upload)))
	mux.HandleFunc("GET /api/uploads/{entryId}/source", router.sourceCode)
	mux.HandleFunc("GET /api/auth/providers", router.authProviders)
	mux.HandleFunc("GET /api/auth/google/start", router.googleAuthStart)
	mux.HandleFunc("GET /api/auth/google/callback", router.googleAuthCallback)
	mux.Handle("GET /api/auth/me", authMiddleware.RequireUser(http.HandlerFunc(router.me)))
	mux.HandleFunc("GET /api/desktop/open", router.desktopOpen)
	if adminEnabled {
		mux.HandleFunc("POST /api/admin/clear", router.adminClear)
		mux.HandleFunc("POST /api/admin/seed", router.adminSeed)
	}

	return withCORS(mux)
}

func (router Router) health(w http.ResponseWriter, _ *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
}

func (router Router) search(w http.ResponseWriter, r *http.Request) {
	query := r.URL.Query()
	includedTags := append([]string{}, query["includeTag"]...)
	includedTags = append(includedTags, query["includedTag"]...)
	if tag := query.Get("tag"); tag != "" {
		includedTags = append(includedTags, tag)
	}

	result, err := router.service.Search(r.Context(), domain.SearchQuery{
		Query:        query.Get("q"),
		QueryMode:    query.Get("qMode"),
		User:         query.Get("user"),
		UserMode:     query.Get("userMode"),
		IncludedTags: includedTags,
		ExcludedTags: append(query["excludeTag"], query["excludedTag"]...),
		Sort:         query.Get("sort"),
		Page:         readInt(query.Get("page"), 1),
		PageSize:     readInt(query.Get("pageSize"), 20),
	})
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) tags(w http.ResponseWriter, r *http.Request) {
	tags, err := router.service.ListTags(r.Context())
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, domain.TagsResponse{Tags: tags})
}

func (router Router) upload(w http.ResponseWriter, r *http.Request) {
	var request domain.UploadRequest
	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	entry, err := router.service.Upload(r.Context(), request)
	if errors.Is(err, service.ErrUploadLimitExceeded) {
		writeError(w, http.StatusForbidden, err)
		return
	}
	if errors.Is(err, auth.ErrUnauthenticated) {
		writeError(w, http.StatusUnauthorized, err)
		return
	}
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusCreated, entry)
}

func (router Router) compileCheck(w http.ResponseWriter, r *http.Request) {
	var request domain.CompileCheckRequest
	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	result, err := router.service.CompileCheck(r.Context(), request)
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) sourceCode(w http.ResponseWriter, r *http.Request) {
	result, err := router.service.SourceCode(r.Context(), r.PathValue("entryId"))
	if errors.Is(err, persistence.ErrNotFound) {
		writeError(w, http.StatusNotFound, err)
		return
	}
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) authProviders(w http.ResponseWriter, _ *http.Request) {
	writeJSON(w, http.StatusOK, router.authManager.Providers())
}

func (router Router) googleAuthStart(w http.ResponseWriter, r *http.Request) {
	loginURL, err := router.authManager.AuthorizationURL("google", r.URL.Query().Get("client"))
	if err != nil {
		writeError(w, http.StatusServiceUnavailable, err)
		return
	}

	if r.URL.Query().Get("format") == "json" {
		writeJSON(w, http.StatusOK, map[string]string{"url": loginURL})
		return
	}

	http.Redirect(w, r, loginURL, http.StatusFound)
}

func (router Router) googleAuthCallback(w http.ResponseWriter, r *http.Request) {
	result, err := router.authManager.CompleteCallback(
		r.Context(),
		r.URL.Query().Get("code"),
		r.URL.Query().Get("state"),
	)
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeLoginComplete(w, result)
}

func (router Router) me(w http.ResponseWriter, r *http.Request) {
	result, err := router.service.Me(r.Context())
	if errors.Is(err, auth.ErrUnauthenticated) {
		writeError(w, http.StatusUnauthorized, err)
		return
	}
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) desktopOpen(w http.ResponseWriter, r *http.Request) {
	entryID := r.URL.Query().Get("entryId")
	if entryID == "" {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "entryId is required"})
		return
	}

	writeJSON(w, http.StatusOK, desktop.LaunchRequest{
		EntryID: entryID,
		URI:     desktop.InstallURI(entryID),
	})
}

func (router Router) adminClear(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
		return
	}

	result, err := router.service.ClearMarketplaceData(r.Context())
	if err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) adminSeed(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
		return
	}

	result, err := router.service.RunSeedData(r.Context())
	if err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func readInt(raw string, fallback int) int {
	value, err := strconv.Atoi(raw)
	if err != nil {
		return fallback
	}

	return value
}

func writeJSON(w http.ResponseWriter, status int, payload any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(payload)
}

func writeError(w http.ResponseWriter, status int, err error) {
	writeJSON(w, status, map[string]string{"error": err.Error()})
}

func writeLoginComplete(w http.ResponseWriter, result auth.CallbackResult) {
	session := result.Session
	pasteToken := fmt.Sprintf("%s|%s", session.Token, session.ExpiresAt.UTC().Format(timeFormatRFC3339))

	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(http.StatusOK)

	_ = loginCompleteTemplate.Execute(w, map[string]string{
		"PasteToken": pasteToken,
		"ExpiresAt":  session.ExpiresAt.UTC().Format(timeFormatRFC3339),
		"Client":     result.Client,
	})
}

const timeFormatRFC3339 = "2006-01-02T15:04:05Z07:00"

var loginCompleteTemplate = template.Must(template.New("login-complete").Parse(`<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>WaviateScript Login Complete</title>
  <style>
    body { margin: 0; font-family: system-ui, sans-serif; background: #070812; color: #edf7ff; }
    main { max-width: 760px; margin: 10vh auto; padding: 28px; }
    code { display: block; overflow-wrap: anywhere; border: 1px solid #45f0ff66; border-radius: 8px; background: #050813; padding: 16px; }
  </style>
</head>
<body>
  <main>
    <h1>WaviateScript login complete</h1>
    <p>Your marketplace session expires at {{ .ExpiresAt }}.</p>
    <p>For the desktop client, paste this session value into WaviateScript:</p>
    <code>{{ .PasteToken }}</code>
  </main>
</body>
</html>`))

func withCORS(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Headers", "content-type, authorization")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		next.ServeHTTP(w, r)
	})
}
