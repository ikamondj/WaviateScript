package domain

import "time"

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
	Query    string
	User     string
	Tag      string
	Sort     string
	Page     int
	PageSize int
}

type SearchResult struct {
	Entries []ScriptEntry `json:"entries"`
	Page    PageInfo      `json:"page"`
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
