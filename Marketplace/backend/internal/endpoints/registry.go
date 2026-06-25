package endpoints

import "net/http"

const (
	Health             = "health"
	Search             = "search"
	Tags               = "tags"
	CompileCheck       = "compile-check"
	Upload             = "upload"
	SourceCode         = "source-code"
	AuthProviders      = "auth-providers"
	GoogleAuthStart    = "google-auth-start"
	GoogleAuthCallback = "google-auth-callback"
	AuthMe             = "auth-me"
	DesktopOpen        = "desktop-open"
	AdminClear         = "admin-clear"
	AdminSeed          = "admin-seed"
)

type AuthPolicy struct {
	Required        bool   `json:"required"`
	RequiredPlan    string `json:"requiredPlan,omitempty"`
	CreatorRequired bool   `json:"creatorRequired,omitempty"`
}

type Endpoint struct {
	Name              string     `json:"name"`
	Method            string     `json:"method"`
	Path              string     `json:"path"`
	Auth              AuthPolicy `json:"auth"`
	CloudFunctionName string     `json:"cloudFunctionName"`
	Serverless        bool       `json:"serverless"`
	LocalAdminOnly    bool       `json:"localAdminOnly,omitempty"`
}

func Registry() []Endpoint {
	return []Endpoint{
		{Name: Health, Method: http.MethodGet, Path: "/healthz", CloudFunctionName: "health", Serverless: true},
		{Name: Search, Method: http.MethodGet, Path: "/api/search", CloudFunctionName: "search", Serverless: true},
		{Name: Tags, Method: http.MethodGet, Path: "/api/tags", CloudFunctionName: "tags", Serverless: true},
		{Name: CompileCheck, Method: http.MethodPost, Path: "/api/source/compile-check", CloudFunctionName: "compile-check", Serverless: true},
		{Name: Upload, Method: http.MethodPost, Path: "/api/uploads", Auth: AuthPolicy{Required: true}, CloudFunctionName: "upload", Serverless: true},
		{Name: SourceCode, Method: http.MethodGet, Path: "/api/uploads/{entryId}/source", CloudFunctionName: "source-code", Serverless: true},
		{Name: AuthProviders, Method: http.MethodGet, Path: "/api/auth/providers", CloudFunctionName: "auth-providers", Serverless: true},
		{Name: GoogleAuthStart, Method: http.MethodGet, Path: "/api/auth/google/start", CloudFunctionName: "google-auth-start", Serverless: true},
		{Name: GoogleAuthCallback, Method: http.MethodGet, Path: "/api/auth/google/callback", CloudFunctionName: "google-auth-callback", Serverless: true},
		{Name: AuthMe, Method: http.MethodGet, Path: "/api/auth/me", Auth: AuthPolicy{Required: true}, CloudFunctionName: "auth-me", Serverless: true},
		{Name: DesktopOpen, Method: http.MethodGet, Path: "/api/desktop/open", CloudFunctionName: "desktop-open", Serverless: true},
		{Name: AdminClear, Method: http.MethodPost, Path: "/api/admin/clear", LocalAdminOnly: true},
		{Name: AdminSeed, Method: http.MethodPost, Path: "/api/admin/seed", LocalAdminOnly: true},
	}
}

func ServerEndpoints(localAdminEnabled bool) []Endpoint {
	return filter(func(endpoint Endpoint) bool {
		return !endpoint.LocalAdminOnly || localAdminEnabled
	})
}

func ServerlessEndpoints() []Endpoint {
	return filter(func(endpoint Endpoint) bool {
		return endpoint.Serverless
	})
}

func Find(name string) (Endpoint, bool) {
	for _, endpoint := range Registry() {
		if endpoint.Name == name {
			return endpoint, true
		}
	}

	return Endpoint{}, false
}

func ServerlessNames() []string {
	endpoints := ServerlessEndpoints()
	names := make([]string, 0, len(endpoints))
	for _, endpoint := range endpoints {
		names = append(names, endpoint.Name)
	}

	return names
}

func filter(include func(Endpoint) bool) []Endpoint {
	all := Registry()
	filtered := make([]Endpoint, 0, len(all))
	for _, endpoint := range all {
		if include(endpoint) {
			filtered = append(filtered, endpoint)
		}
	}

	return filtered
}
