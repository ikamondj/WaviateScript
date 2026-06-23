package domain

import "time"

type ScriptEntry struct {
	ID            string    `json:"id"`
	Title         string    `json:"title"`
	AuthorID      string    `json:"authorId"`
	AuthorHandle  string    `json:"authorHandle"`
	Summary       string    `json:"summary"`
	Tags          []string  `json:"tags"`
	Rating        float64   `json:"rating"`
	Downloads     int       `json:"downloads"`
	PayloadFormat string    `json:"payloadFormat"`
	License       string    `json:"license"`
	UpdatedAt     time.Time `json:"updatedAt"`
}

type UploadRequest struct {
	Title         string   `json:"title"`
	AuthorID      string   `json:"authorId"`
	Summary       string   `json:"summary"`
	Tags          []string `json:"tags"`
	PayloadFormat string   `json:"payloadFormat"`
	SourceText    string   `json:"sourceText"`
	License       string   `json:"license"`
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

type PageInfo struct {
	Page       int `json:"page"`
	PageSize   int `json:"pageSize"`
	Total      int `json:"total"`
	TotalPages int `json:"totalPages"`
}
