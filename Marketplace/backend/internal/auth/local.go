package auth

import (
	"context"
	"fmt"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

// LocalManager is a fake auth manager used in local development mode.
// It allows logging in as one of three test users (admin, premium, regular)
// without requiring any real OAuth provider. The username IS the password.
type LocalManager struct {
	store UserStore
	clock func() time.Time
}

func NewLocalManager(store UserStore) *LocalManager {
	return &LocalManager{
		store: store,
		clock: func() time.Time { return time.Now().UTC() },
	}
}

func (manager *LocalManager) Providers() []Provider {
	return []Provider{
		{
			Name:        "local",
			DisplayName: "Local Test Auth",
		},
	}
}

func (manager *LocalManager) AuthorizationURL(providerName string, client string) (string, error) {
	return "/api/auth/local/login?client=" + client, nil
}

func (manager *LocalManager) CompleteCallback(ctx context.Context, code string, rawState string) (CallbackResult, error) {
	// In the local login form, "code" carries the username.
	username := code
	if username == "" {
		return CallbackResult{}, fmt.Errorf("username required")
	}

	identity := domain.OAuthIdentity{
		Provider:    "local",
		Subject:     username,
		Username:    username,
		Email:       username + "@localhost",
		DisplayName: username,
	}

	user, err := manager.store.UpsertOAuthUser(ctx, identity)
	if err != nil {
		return CallbackResult{}, err
	}

	rawSessionToken := mustRandomToken(32)
	sessionExpiresAt := manager.clock().Add(defaultSessionTTL)
	if err := manager.store.CreateSession(ctx, domain.SessionToken{
		TokenHash: HashToken(rawSessionToken),
		UserID:    user.ID,
		ExpiresAt: sessionExpiresAt,
	}); err != nil {
		return CallbackResult{}, err
	}

	return CallbackResult{
		Session: domain.AuthSession{
			Token:     rawSessionToken,
			ExpiresAt: sessionExpiresAt,
			User:      user,
		},
		Client: rawState,
	}, nil
}

// VerifyBearerToken lets LocalManager satisfy the SessionVerifier interface
// so the auth middleware can validate tokens issued by the local login flow.
func (manager *LocalManager) VerifyBearerToken(ctx context.Context, rawToken string) (domain.AuthenticatedUser, error) {
	if rawToken == "" {
		return domain.AuthenticatedUser{}, ErrUnauthenticated
	}
	return manager.store.FindSessionByTokenHash(ctx, HashToken(rawToken), manager.clock())
}
