package domain

import "time"

const (
	MatchModeContains   = "contains"
	MatchModeStartsWith = "starts_with"
	MatchModeExact      = "exact"

	PlanStandard = "standard"
	PlanPremium  = "premium"
)

type ScriptEntry struct {
	ID              string    `json:"id"`
	Name            string    `json:"name"`
	AuthorID        string    `json:"authorId"`
	AuthorName      string    `json:"authorName"`
	Description     string    `json:"description"`
	RatingScore     float64   `json:"ratingScore"`
	RatingCount     int       `json:"ratingCount"`
	DownloadCount   int       `json:"downloadCount"`
	RequiresPremium bool      `json:"requiresPremium"`
	Content         string    `json:"content"`
	Tags            []string  `json:"tags"`
	CreatedAt       time.Time `json:"createdAt"`
	UpdatedAt       time.Time `json:"updatedAt"`
}

type UploadRequest struct {
	Name            string   `json:"name"`
	AuthorID        string   `json:"authorId"`
	Description     string   `json:"description"`
	RequiresPremium bool     `json:"requiresPremium"`
	Content         string   `json:"content"`
	Tags            []string `json:"tags"`
}

type CompileCheckRequest struct {
	Content string `json:"content"`
}

type CompileCheckResult struct {
	Passed         bool           `json:"passed"`
	CompilerStatus CompilerStatus `json:"compilerStatus"`
}

type CompilerStatus struct {
	Available bool   `json:"available"`
	Required  bool   `json:"required"`
	Mode      string `json:"mode"`
	Detail    string `json:"detail,omitempty"`
}

type SearchQuery struct {
	Query        string
	QueryMode    string
	User         string
	UserMode     string
	IncludedTags []string
	ExcludedTags []string
	Sort         string
	Page         int
	PageSize     int
}

type SearchResult struct {
	Entries []ScriptEntry `json:"entries"`
	Page    PageInfo      `json:"page"`
}

type TagsResponse struct {
	Tags []string `json:"tags"`
}

type SourceCodeResponse struct {
	EntryID string `json:"entryId"`
	Source  string `json:"source"`
}

type PageInfo struct {
	Page       int `json:"page"`
	PageSize   int `json:"pageSize"`
	Total      int `json:"total"`
	TotalPages int `json:"totalPages"`
}

type AdminActionResult struct {
	Message string             `json:"message"`
	Scripts []SeedScriptResult `json:"scripts,omitempty"`
}

type SeedScriptResult struct {
	Name string `json:"name"`
}

type OAuthIdentity struct {
	Provider     string
	Subject      string
	Username     string
	Email        string
	DisplayName  string
	AccessToken  string
	RefreshToken string
	ExpiresAt    *time.Time
}

type SessionToken struct {
	TokenHash string
	UserID    string
	ExpiresAt time.Time
}

type AuthenticatedUser struct {
	ID                    string     `json:"id"`
	DisplayName           string     `json:"displayName"`
	Email                 string     `json:"email,omitempty"`
	AuthorID              string     `json:"authorId"`
	AuthorName            string     `json:"authorName"`
	Plan                  string     `json:"plan"`
	Creator               bool       `json:"creator"`
	SubscriptionExpiresAt *time.Time `json:"subscriptionExpiresAt,omitempty"`
	SubscriptionActive    bool       `json:"subscriptionActive"`
	UploadLimit           int        `json:"uploadLimit"`
	UploadLimitUnlimited  bool       `json:"uploadLimitUnlimited"`
	UploadCount           int        `json:"uploadCount"`
}

type AuthSession struct {
	Token     string            `json:"token"`
	ExpiresAt time.Time         `json:"expiresAt"`
	User      AuthenticatedUser `json:"user"`
}
