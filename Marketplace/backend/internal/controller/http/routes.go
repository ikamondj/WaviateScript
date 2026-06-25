package httpcontroller

import (
	"encoding/json"
	"errors"
	"net/http"
	"strconv"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	"github.com/waviate-script/marketplace/backend/internal/desktop"
	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

type Router struct {
	service *service.Service
	auth    auth.Middleware
}

func NewRouter(service *service.Service, authMiddleware auth.Middleware, adminEnabled bool) http.Handler {
	router := Router{
		service: service,
		auth:    authMiddleware,
	}

	mux := http.NewServeMux()
	mux.HandleFunc("GET /healthz", router.health)
	mux.HandleFunc("GET /api/search", router.search)
	mux.HandleFunc("POST /api/source/compile-check", router.compileCheck)
	mux.Handle("POST /api/uploads", authMiddleware.RequireUser(http.HandlerFunc(router.upload)))
	mux.HandleFunc("GET /api/uploads/{entryId}/source", router.sourceCode)
	mux.HandleFunc("GET /api/auth/providers", router.authProviders)
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
	result, err := router.service.Search(r.Context(), domain.SearchQuery{
		Query:    query.Get("q"),
		User:     query.Get("user"),
		Tag:      query.Get("tag"),
		Sort:     query.Get("sort"),
		Page:     readInt(query.Get("page"), 1),
		PageSize: readInt(query.Get("pageSize"), 20),
	})
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	writeJSON(w, http.StatusOK, result)
}

func (router Router) upload(w http.ResponseWriter, r *http.Request) {
	var request domain.UploadRequest
	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	entry, err := router.service.Upload(r.Context(), request)
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
	// TODO: Hide providers whose environment variables are not configured.
	writeJSON(w, http.StatusOK, auth.DefaultProviders())
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
