package server

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/waviate-script/marketplace/backend/internal/config"
	"github.com/waviate-script/marketplace/backend/internal/endpoints"
)

func TestNewEndpointHandlerWithConfig(t *testing.T) {
	handler, err := NewEndpointHandlerWithConfig(config.Config{Mode: "test"}, endpoints.Health)
	if err != nil {
		t.Fatalf("build endpoint handler: %v", err)
	}

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, httptest.NewRequest(http.MethodGet, "/healthz", nil))

	if recorder.Code != http.StatusOK {
		t.Fatalf("expected healthcheck OK, got HTTP %d", recorder.Code)
	}
}

func TestNewEndpointHandlerRejectsUnknownEndpoint(t *testing.T) {
	if _, err := NewEndpointHandlerWithConfig(config.Config{Mode: "test"}, "missing"); err == nil {
		t.Fatalf("expected unknown endpoint error")
	}
}
