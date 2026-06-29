package auth

import (
	"context"
	"errors"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

func TestMiddlewareAllowsPublicRouteWithoutVerifier(t *testing.T) {
	middleware := NewMiddleware(nil)
	handler := middleware.Apply(Requirement{}, http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, httptest.NewRequest(http.MethodGet, "/public", nil))

	if recorder.Code != http.StatusNoContent {
		t.Fatalf("expected public route to pass, got HTTP %d", recorder.Code)
	}
}

func TestMiddlewareRejectsMissingBearerToken(t *testing.T) {
	middleware := NewMiddleware(fakeVerifier{})
	handler := middleware.Apply(Requirement{Required: true}, http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, httptest.NewRequest(http.MethodGet, "/protected", nil))

	if recorder.Code != http.StatusUnauthorized {
		t.Fatalf("expected unauthorized, got HTTP %d", recorder.Code)
	}
}

func TestMiddlewareRejectsInvalidOrExpiredToken(t *testing.T) {
	middleware := NewMiddleware(fakeVerifier{err: ErrUnauthenticated})
	handler := middleware.Apply(Requirement{Required: true}, http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))

	request := httptest.NewRequest(http.MethodGet, "/protected", nil)
	request.Header.Set("Authorization", "Bearer expired")

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, request)

	if recorder.Code != http.StatusUnauthorized {
		t.Fatalf("expected unauthorized, got HTTP %d", recorder.Code)
	}
}

func TestMiddlewareAttachesAuthenticatedUser(t *testing.T) {
	wantUser := domain.AuthenticatedUser{ID: "user-1", AuthorID: "author-1"}
	middleware := NewMiddleware(fakeVerifier{user: wantUser})
	handler := middleware.Apply(Requirement{Required: true}, http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		gotUser, ok := UserFromContext(r.Context())
		if !ok {
			t.Fatalf("expected user in context")
		}
		if gotUser.ID != wantUser.ID {
			t.Fatalf("unexpected user id %q", gotUser.ID)
		}
		w.WriteHeader(http.StatusNoContent)
	}))

	request := httptest.NewRequest(http.MethodGet, "/protected", nil)
	request.Header.Set("Authorization", "Bearer valid")

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, request)

	if recorder.Code != http.StatusNoContent {
		t.Fatalf("expected no content, got HTTP %d", recorder.Code)
	}
}

func TestMiddlewareEnforcesCreatorRequirement(t *testing.T) {
	middleware := NewMiddleware(fakeVerifier{user: domain.AuthenticatedUser{ID: "user-1"}})
	handler := middleware.Apply(Requirement{Required: true, CreatorRequired: true}, http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))

	request := httptest.NewRequest(http.MethodGet, "/creator", nil)
	request.Header.Set("Authorization", "Bearer valid")

	recorder := httptest.NewRecorder()
	handler.ServeHTTP(recorder, request)

	if recorder.Code != http.StatusForbidden {
		t.Fatalf("expected forbidden, got HTTP %d", recorder.Code)
	}
}

func TestMiddlewareEnforcesActivePremiumRequirement(t *testing.T) {
	handler := func(user domain.AuthenticatedUser) http.Handler {
		middleware := NewMiddleware(fakeVerifier{user: user})
		return middleware.Apply(Requirement{Required: true, RequiredPlan: domain.PlanPremium}, http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
			w.WriteHeader(http.StatusNoContent)
		}))
	}

	inactivePremium := domain.AuthenticatedUser{
		ID:                 "user-1",
		Plan:               domain.PlanPremium,
		SubscriptionActive: false,
	}
	request := httptest.NewRequest(http.MethodGet, "/premium", nil)
	request.Header.Set("Authorization", "Bearer valid")

	recorder := httptest.NewRecorder()
	handler(inactivePremium).ServeHTTP(recorder, request)

	if recorder.Code != http.StatusForbidden {
		t.Fatalf("expected inactive premium user to be forbidden, got HTTP %d", recorder.Code)
	}

	activePremium := domain.AuthenticatedUser{
		ID:                 "user-2",
		Plan:               domain.PlanPremium,
		SubscriptionActive: true,
	}
	request = httptest.NewRequest(http.MethodGet, "/premium", nil)
	request.Header.Set("Authorization", "Bearer valid")

	recorder = httptest.NewRecorder()
	handler(activePremium).ServeHTTP(recorder, request)

	if recorder.Code != http.StatusNoContent {
		t.Fatalf("expected active premium user through, got HTTP %d", recorder.Code)
	}
}

type fakeVerifier struct {
	user domain.AuthenticatedUser
	err  error
}

func (verifier fakeVerifier) VerifyBearerToken(context.Context, string) (domain.AuthenticatedUser, error) {
	if verifier.err != nil {
		return domain.AuthenticatedUser{}, verifier.err
	}
	if verifier.user.ID == "" {
		return domain.AuthenticatedUser{}, errors.New("missing test user")
	}

	return verifier.user, nil
}
