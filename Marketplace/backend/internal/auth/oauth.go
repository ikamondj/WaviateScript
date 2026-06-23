package auth

import "net/http"

type Provider struct {
	Name         string
	AuthURL      string
	TokenURL     string
	ProfileURL   string
	ClientIDEnv  string
	ClientSecret string
	Scopes       []string
}

type Middleware struct{}

func DefaultProviders() []Provider {
	return []Provider{
		{
			Name:         "google",
			AuthURL:      "https://accounts.google.com/o/oauth2/v2/auth",
			TokenURL:     "https://oauth2.googleapis.com/token",
			ProfileURL:   "https://openidconnect.googleapis.com/v1/userinfo",
			ClientIDEnv:  "GOOGLE_CLIENT_ID",
			ClientSecret: "GOOGLE_CLIENT_SECRET",
			Scopes:       []string{"openid", "email", "profile"},
		},
		{
			Name:         "microsoft",
			AuthURL:      "https://login.microsoftonline.com/common/oauth2/v2.0/authorize",
			TokenURL:     "https://login.microsoftonline.com/common/oauth2/v2.0/token",
			ProfileURL:   "https://graph.microsoft.com/oidc/userinfo",
			ClientIDEnv:  "MICROSOFT_CLIENT_ID",
			ClientSecret: "MICROSOFT_CLIENT_SECRET",
			Scopes:       []string{"openid", "email", "profile"},
		},
		{
			Name:         "github",
			AuthURL:      "https://github.com/login/oauth/authorize",
			TokenURL:     "https://github.com/login/oauth/access_token",
			ProfileURL:   "https://api.github.com/user",
			ClientIDEnv:  "GITHUB_CLIENT_ID",
			ClientSecret: "GITHUB_CLIENT_SECRET",
			Scopes:       []string{"read:user", "user:email"},
		},
	}
}

func (Middleware) RequireUser(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// TODO: Validate session/JWT and attach the authenticated marketplace user to context.
		next.ServeHTTP(w, r)
	})
}
