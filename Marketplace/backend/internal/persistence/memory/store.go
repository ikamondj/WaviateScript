package memory

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
)

type Store struct {
	entries []domain.ScriptEntry
}

func NewStore() *Store {
	return &Store{
		entries: []domain.ScriptEntry{
			{
				ID:            "glacial-padfield",
				Title:         "Glacial Padfield",
				AuthorID:      "user_phasefold",
				AuthorHandle:  "phasefold",
				Summary:       "Slow evolving sample shader for crystalline pads.",
				Tags:          []string{"pads", "ambient", "sample-shader"},
				Rating:        4.9,
				Downloads:     18420,
				PayloadFormat: "unknown",
				License:       "MIT",
				UpdatedAt:     time.Date(2026, 6, 10, 0, 0, 0, 0, time.UTC),
			},
		},
	}
}

var _ persistence.Store = (*Store)(nil)

func (store *Store) Search(_ context.Context, query domain.SearchQuery) (domain.SearchResult, error) {
	filtered := make([]domain.ScriptEntry, 0, len(store.entries))
	needle := strings.ToLower(strings.TrimSpace(query.Query))
	user := strings.ToLower(strings.TrimSpace(query.User))

	for _, entry := range store.entries {
		if needle != "" && !matchesText(entry, needle) {
			continue
		}

		if user != "" && !strings.Contains(strings.ToLower(entry.AuthorHandle), user) {
			continue
		}

		if query.Tag != "" && !hasTag(entry, query.Tag) {
			continue
		}

		filtered = append(filtered, entry)
	}

	sortEntries(filtered, query.Sort)

	total := len(filtered)
	pageSize := query.PageSize
	if pageSize <= 0 {
		pageSize = 20
	}
	page := query.Page
	if page <= 0 {
		page = 1
	}

	totalPages := max(1, (total+pageSize-1)/pageSize)
	if page > totalPages {
		page = totalPages
	}

	start := (page - 1) * pageSize
	end := min(start+pageSize, total)
	if start > end {
		start = end
	}

	return domain.SearchResult{
		Entries: filtered[start:end],
		Page: domain.PageInfo{
			Page:       page,
			PageSize:   pageSize,
			Total:      total,
			TotalPages: totalPages,
		},
	}, nil
}

func (store *Store) SaveUpload(_ context.Context, upload domain.UploadRequest) (domain.ScriptEntry, error) {
	entry := domain.ScriptEntry{
		ID:            fmt.Sprintf("draft-%d", len(store.entries)+1),
		Title:         upload.Title,
		AuthorID:      upload.AuthorID,
		AuthorHandle:  upload.AuthorID,
		Summary:       upload.Summary,
		Tags:          upload.Tags,
		Rating:        0,
		Downloads:     0,
		PayloadFormat: upload.PayloadFormat,
		License:       upload.License,
		UpdatedAt:     time.Now().UTC(),
	}

	store.entries = append(store.entries, entry)
	return entry, nil
}

func (store *Store) GetByID(_ context.Context, id string) (domain.ScriptEntry, error) {
	for _, entry := range store.entries {
		if entry.ID == id {
			return entry, nil
		}
	}

	return domain.ScriptEntry{}, persistence.ErrNotFound
}

func matchesText(entry domain.ScriptEntry, needle string) bool {
	if strings.Contains(strings.ToLower(entry.Title), needle) {
		return true
	}
	if strings.Contains(strings.ToLower(entry.Summary), needle) {
		return true
	}

	for _, tag := range entry.Tags {
		if strings.Contains(strings.ToLower(tag), needle) {
			return true
		}
	}

	return false
}

func hasTag(entry domain.ScriptEntry, tag string) bool {
	for _, entryTag := range entry.Tags {
		if entryTag == tag {
			return true
		}
	}

	return false
}

func sortEntries(entries []domain.ScriptEntry, sortBy string) {
	sort.Slice(entries, func(i, j int) bool {
		switch sortBy {
		case "downloads":
			return entries[i].Downloads > entries[j].Downloads
		case "updated":
			return entries[i].UpdatedAt.After(entries[j].UpdatedAt)
		default:
			return entries[i].Rating > entries[j].Rating
		}
	})
}
