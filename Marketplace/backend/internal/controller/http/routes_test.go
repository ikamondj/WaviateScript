package httpcontroller

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	"github.com/waviate-script/marketplace/backend/internal/endpoints"
	"github.com/waviate-script/marketplace/backend/internal/persistence/memory"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

func TestRouterPublicHealthcheck(t *testing.T) {
	store := memory.NewStore()
	authManager := auth.NewManager(store, auth.Config{})
	router := NewRouter(service.NewService(store), auth.NewMiddleware(authManager), authManager, false)

	recorder := httptest.NewRecorder()
	router.ServeHTTP(recorder, httptest.NewRequest(http.MethodGet, "/healthz", nil))

	if recorder.Code != http.StatusOK {
		t.Fatalf("expected healthcheck OK, got HTTP %d", recorder.Code)
	}
	if !strings.Contains(recorder.Body.String(), `"status":"ok"`) {
		t.Fatalf("unexpected healthcheck response %q", recorder.Body.String())
	}
}

func TestRouterProtectedUploadRequiresAuth(t *testing.T) {
	store := memory.NewStore()
	authManager := auth.NewManager(store, auth.Config{})
	router := NewRouter(service.NewService(store), auth.NewMiddleware(authManager), authManager, false)

	recorder := httptest.NewRecorder()
	router.ServeHTTP(recorder, httptest.NewRequest(http.MethodPost, "/api/uploads", strings.NewReader(`{}`)))

	if recorder.Code != http.StatusUnauthorized {
		t.Fatalf("expected upload to require auth, got HTTP %d", recorder.Code)
	}
}

func TestSingleEndpointRouterOnlyServesSelectedEndpoint(t *testing.T) {
	store := memory.NewStore()
	authManager := auth.NewManager(store, auth.Config{})
	endpoint, ok := endpoints.Find(endpoints.Health)
	if !ok {
		t.Fatalf("missing health endpoint")
	}

	router := NewEndpointRouter(service.NewService(store), auth.NewMiddleware(authManager), authManager, endpoint)

	healthRecorder := httptest.NewRecorder()
	router.ServeHTTP(healthRecorder, httptest.NewRequest(http.MethodGet, "/healthz", nil))
	if healthRecorder.Code != http.StatusOK {
		t.Fatalf("expected selected endpoint to respond, got HTTP %d", healthRecorder.Code)
	}

	searchRecorder := httptest.NewRecorder()
	router.ServeHTTP(searchRecorder, httptest.NewRequest(http.MethodGet, "/api/search", nil))
	if searchRecorder.Code != http.StatusNotFound {
		t.Fatalf("expected unselected endpoint to be absent, got HTTP %d", searchRecorder.Code)
	}
}
