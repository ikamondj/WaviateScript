package auth

import (
	"context"
	"crypto/hmac"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

const (
	defaultSessionTTL = 30 * 24 * time.Hour
	stateTTL          = 10 * time.Minute
	googleProvider    = "google"
)

var (
	ErrAuthUnavailable  = errors.New("authentication provider is not configured")
	ErrInvalidAuthState = errors.New("invalid OAuth state")
	ErrUnauthenticated  = errors.New("authentication required")
)

type Provider struct {
	Name         string   `json:"name"`
	DisplayName  string   `json:"displayName"`
	AuthURL      string   `json:"-"`
	TokenURL     string   `json:"-"`
	ProfileURL   string   `json:"-"`
	ClientID     string   `json:"-"`
	ClientSecret string   `json:"-"`
	RedirectURL  string   `json:"-"`
	Scopes       []string `json:"scopes"`
}

type Config struct {
	BaseURL            string
	Secret             string
	GoogleClientID     string
	GoogleClientSecret string
	SessionTTL         time.Duration
}

type UserStore interface {
	UpsertOAuthUser(ctx context.Context, identity domain.OAuthIdentity) (domain.AuthenticatedUser, error)
	CreateSession(ctx context.Context, token domain.SessionToken) error
	FindSessionByTokenHash(ctx context.Context, tokenHash string, now time.Time) (domain.AuthenticatedUser, error)
}

type Manager struct {
	store      UserStore
	config     Config
	httpClient *http.Client
	clock      func() time.Time
}

type CallbackResult struct {
	Session domain.AuthSession
	Client  string
}

type Middleware struct {
	verifier SessionVerifier
}

type SessionVerifier interface {
	VerifyBearerToken(ctx context.Context, rawToken string) (domain.AuthenticatedUser, error)
}

type contextKey struct{}

type oauthState struct {
	Provider  string `json:"provider"`
	Client    string `json:"client"`
	ExpiresAt int64  `json:"expiresAt"`
	Nonce     string `json:"nonce"`
}

type tokenResponse struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
	ExpiresIn    int64  `json:"expires_in"`
	TokenType    string `json:"token_type"`
}

type googleProfile struct {
	Subject       string `json:"sub"`
	Email         string `json:"email"`
	Name          string `json:"name"`
	GivenName     string `json:"given_name"`
	Picture       string `json:"picture"`
	EmailVerified bool   `json:"email_verified"`
}

func NewManager(store UserStore, config Config) *Manager {
	config = normalizeConfig(config)
	return &Manager{
		store:      store,
		config:     config,
		httpClient: http.DefaultClient,
		clock:      func() time.Time { return time.Now().UTC() },
	}
}

func NewMiddleware(verifier SessionVerifier) Middleware {
	return Middleware{verifier: verifier}
}

func DefaultProviders() []Provider {
	return []Provider{
		{
			Name:        googleProvider,
			DisplayName: "Google",
			AuthURL:     "https://accounts.google.com/o/oauth2/v2/auth",
			TokenURL:    "https://oauth2.googleapis.com/token",
			ProfileURL:  "https://openidconnect.googleapis.com/v1/userinfo",
			Scopes:      []string{"openid", "email", "profile"},
		},
	}
}

func (manager *Manager) Providers() []Provider {
	providers := make([]Provider, 0, 1)
	if provider, ok := manager.provider(googleProvider); ok {
		providers = append(providers, provider)
	}

	return providers
}

func (manager *Manager) AuthorizationURL(providerName string, client string) (string, error) {
	provider, ok := manager.provider(providerName)
	if !ok {
		return "", ErrAuthUnavailable
	}

	state, err := manager.encodeState(oauthState{
		Provider:  provider.Name,
		Client:    normalizeClient(client),
		ExpiresAt: manager.clock().Add(stateTTL).Unix(),
		Nonce:     mustRandomToken(12),
	})
	if err != nil {
		return "", err
	}

	authURL, err := url.Parse(provider.AuthURL)
	if err != nil {
		return "", err
	}

	query := authURL.Query()
	query.Set("client_id", provider.ClientID)
	query.Set("redirect_uri", provider.RedirectURL)
	query.Set("response_type", "code")
	query.Set("scope", strings.Join(provider.Scopes, " "))
	query.Set("state", state)
	query.Set("access_type", "offline")
	query.Set("include_granted_scopes", "true")
	authURL.RawQuery = query.Encode()

	return authURL.String(), nil
}

func (manager *Manager) CompleteCallback(ctx context.Context, code string, rawState string) (CallbackResult, error) {
	state, err := manager.decodeState(rawState)
	if err != nil {
		return CallbackResult{}, err
	}
	if state.Provider != googleProvider {
		return CallbackResult{}, ErrAuthUnavailable
	}

	provider, ok := manager.provider(state.Provider)
	if !ok {
		return CallbackResult{}, ErrAuthUnavailable
	}

	token, err := manager.exchangeCode(ctx, provider, strings.TrimSpace(code))
	if err != nil {
		return CallbackResult{}, err
	}

	profile, err := manager.loadGoogleProfile(ctx, provider, token.AccessToken)
	if err != nil {
		return CallbackResult{}, err
	}
	if strings.TrimSpace(profile.Subject) == "" {
		return CallbackResult{}, errors.New("google profile did not include a subject")
	}

	var providerTokenExpiry *time.Time
	if token.ExpiresIn > 0 {
		expiresAt := manager.clock().Add(time.Duration(token.ExpiresIn) * time.Second)
		providerTokenExpiry = &expiresAt
	}

	user, err := manager.store.UpsertOAuthUser(ctx, domain.OAuthIdentity{
		Provider:     provider.Name,
		Subject:      profile.Subject,
		Username:     firstNonEmpty(profile.Email, profile.Name, profile.Subject),
		Email:        profile.Email,
		DisplayName:  firstNonEmpty(profile.Name, profile.GivenName, profile.Email, "Waviate Creator"),
		AccessToken:  token.AccessToken,
		RefreshToken: token.RefreshToken,
		ExpiresAt:    providerTokenExpiry,
	})
	if err != nil {
		return CallbackResult{}, err
	}

	rawSessionToken := mustRandomToken(32)
	sessionExpiresAt := manager.clock().Add(manager.config.SessionTTL)
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
		Client: state.Client,
	}, nil
}

func (manager *Manager) VerifyBearerToken(ctx context.Context, rawToken string) (domain.AuthenticatedUser, error) {
	rawToken = strings.TrimSpace(rawToken)
	if rawToken == "" {
		return domain.AuthenticatedUser{}, ErrUnauthenticated
	}

	return manager.store.FindSessionByTokenHash(ctx, HashToken(rawToken), manager.clock())
}

func WithUser(ctx context.Context, user domain.AuthenticatedUser) context.Context {
	return context.WithValue(ctx, contextKey{}, user)
}

func UserFromContext(ctx context.Context) (domain.AuthenticatedUser, bool) {
	user, ok := ctx.Value(contextKey{}).(domain.AuthenticatedUser)
	return user, ok
}

func HashToken(rawToken string) string {
	sum := sha256.Sum256([]byte(rawToken))
	return hex.EncodeToString(sum[:])
}

func (middleware Middleware) RequireUser(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if middleware.verifier == nil {
			http.Error(w, ErrUnauthenticated.Error(), http.StatusUnauthorized)
			return
		}

		token, ok := readBearerToken(r.Header.Get("Authorization"))
		if !ok {
			http.Error(w, ErrUnauthenticated.Error(), http.StatusUnauthorized)
			return
		}

		user, err := middleware.verifier.VerifyBearerToken(r.Context(), token)
		if err != nil {
			http.Error(w, ErrUnauthenticated.Error(), http.StatusUnauthorized)
			return
		}

		next.ServeHTTP(w, r.WithContext(WithUser(r.Context(), user)))
	})
}

func (manager *Manager) provider(providerName string) (Provider, bool) {
	providerName = strings.ToLower(strings.TrimSpace(providerName))
	for _, provider := range DefaultProviders() {
		if provider.Name != providerName {
			continue
		}

		if provider.Name == googleProvider {
			if manager.config.GoogleClientID == "" || manager.config.GoogleClientSecret == "" {
				return Provider{}, false
			}

			provider.ClientID = manager.config.GoogleClientID
			provider.ClientSecret = manager.config.GoogleClientSecret
			provider.RedirectURL = strings.TrimRight(manager.config.BaseURL, "/") + "/api/auth/google/callback"
		}

		return provider, true
	}

	return Provider{}, false
}

func (manager *Manager) exchangeCode(ctx context.Context, provider Provider, code string) (tokenResponse, error) {
	if code == "" {
		return tokenResponse{}, errors.New("oauth code is required")
	}

	form := url.Values{}
	form.Set("code", code)
	form.Set("client_id", provider.ClientID)
	form.Set("client_secret", provider.ClientSecret)
	form.Set("redirect_uri", provider.RedirectURL)
	form.Set("grant_type", "authorization_code")

	request, err := http.NewRequestWithContext(ctx, http.MethodPost, provider.TokenURL, strings.NewReader(form.Encode()))
	if err != nil {
		return tokenResponse{}, err
	}
	request.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	request.Header.Set("Accept", "application/json")

	response, err := manager.httpClient.Do(request)
	if err != nil {
		return tokenResponse{}, err
	}
	defer response.Body.Close()

	body, err := io.ReadAll(io.LimitReader(response.Body, 1<<20))
	if err != nil {
		return tokenResponse{}, err
	}
	if response.StatusCode < 200 || response.StatusCode >= 300 {
		return tokenResponse{}, fmt.Errorf("google token exchange failed with HTTP %d: %s", response.StatusCode, strings.TrimSpace(string(body)))
	}

	var token tokenResponse
	if err := json.Unmarshal(body, &token); err != nil {
		return tokenResponse{}, err
	}
	if strings.TrimSpace(token.AccessToken) == "" {
		return tokenResponse{}, errors.New("google token exchange did not return an access token")
	}

	return token, nil
}

func (manager *Manager) loadGoogleProfile(ctx context.Context, provider Provider, accessToken string) (googleProfile, error) {
	request, err := http.NewRequestWithContext(ctx, http.MethodGet, provider.ProfileURL, nil)
	if err != nil {
		return googleProfile{}, err
	}
	request.Header.Set("Authorization", "Bearer "+accessToken)
	request.Header.Set("Accept", "application/json")

	response, err := manager.httpClient.Do(request)
	if err != nil {
		return googleProfile{}, err
	}
	defer response.Body.Close()

	body, err := io.ReadAll(io.LimitReader(response.Body, 1<<20))
	if err != nil {
		return googleProfile{}, err
	}
	if response.StatusCode < 200 || response.StatusCode >= 300 {
		return googleProfile{}, fmt.Errorf("google profile request failed with HTTP %d: %s", response.StatusCode, strings.TrimSpace(string(body)))
	}

	var profile googleProfile
	if err := json.Unmarshal(body, &profile); err != nil {
		return googleProfile{}, err
	}

	return profile, nil
}

func (manager *Manager) encodeState(state oauthState) (string, error) {
	payload, err := json.Marshal(state)
	if err != nil {
		return "", err
	}

	encodedPayload := base64.RawURLEncoding.EncodeToString(payload)
	signature := manager.sign(encodedPayload)
	return encodedPayload + "." + signature, nil
}

func (manager *Manager) decodeState(rawState string) (oauthState, error) {
	payload, signature, ok := strings.Cut(rawState, ".")
	if !ok || payload == "" || signature == "" {
		return oauthState{}, ErrInvalidAuthState
	}

	expectedSignature := manager.sign(payload)
	if !hmac.Equal([]byte(signature), []byte(expectedSignature)) {
		return oauthState{}, ErrInvalidAuthState
	}

	rawPayload, err := base64.RawURLEncoding.DecodeString(payload)
	if err != nil {
		return oauthState{}, ErrInvalidAuthState
	}

	var state oauthState
	if err := json.Unmarshal(rawPayload, &state); err != nil {
		return oauthState{}, ErrInvalidAuthState
	}
	if manager.clock().Unix() > state.ExpiresAt {
		return oauthState{}, ErrInvalidAuthState
	}

	return state, nil
}

func (manager *Manager) sign(value string) string {
	mac := hmac.New(sha256.New, []byte(manager.config.Secret))
	_, _ = mac.Write([]byte(value))
	return base64.RawURLEncoding.EncodeToString(mac.Sum(nil))
}

func normalizeConfig(config Config) Config {
	config.BaseURL = strings.TrimSpace(config.BaseURL)
	if config.BaseURL == "" {
		config.BaseURL = "http://localhost:8080"
	}
	config.GoogleClientID = strings.TrimSpace(config.GoogleClientID)
	config.GoogleClientSecret = strings.TrimSpace(config.GoogleClientSecret)
	config.Secret = strings.TrimSpace(config.Secret)
	if config.Secret == "" {
		config.Secret = firstNonEmpty(config.GoogleClientSecret, "local-dev-auth-secret")
	}
	if config.SessionTTL <= 0 {
		config.SessionTTL = defaultSessionTTL
	}

	return config
}

func normalizeClient(client string) string {
	switch strings.ToLower(strings.TrimSpace(client)) {
	case "desktop":
		return "desktop"
	default:
		return "web"
	}
}

func readBearerToken(header string) (string, bool) {
	scheme, token, ok := strings.Cut(strings.TrimSpace(header), " ")
	if !ok || !strings.EqualFold(scheme, "Bearer") || strings.TrimSpace(token) == "" {
		return "", false
	}

	return strings.TrimSpace(token), true
}

func mustRandomToken(byteCount int) string {
	buffer := make([]byte, byteCount)
	if _, err := rand.Read(buffer); err != nil {
		panic(err)
	}

	return base64.RawURLEncoding.EncodeToString(buffer)
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if trimmed := strings.TrimSpace(value); trimmed != "" {
			return trimmed
		}
	}

	return ""
}
