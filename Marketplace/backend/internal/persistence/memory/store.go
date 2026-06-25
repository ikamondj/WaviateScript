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
				ID:              "glacial-padfield",
				Name:            "Glacial Padfield",
				AuthorID:        "author_phasefold",
				AuthorName:      "Phase Fold",
				Description:     "Slow evolving sample shader for crystalline pads.",
				RatingScore:     4.9,
				RatingCount:     42,
				DownloadCount:   18420,
				RequiresPremium: false,
				Content:         "sample_process = function(ctx) { return ctx.input; }",
				Tags:            []string{"pads", "ambient", "sample-shader"},
				CreatedAt:       time.Date(2026, 6, 1, 0, 0, 0, 0, time.UTC),
				UpdatedAt:       time.Date(2026, 6, 10, 0, 0, 0, 0, time.UTC),
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

		if user != "" && !strings.Contains(strings.ToLower(entry.AuthorName), user) {
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
	now := time.Now().UTC()
	entry := domain.ScriptEntry{
		ID:              fmt.Sprintf("draft-%d", len(store.entries)+1),
		Name:            upload.Name,
		AuthorID:        upload.AuthorID,
		AuthorName:      upload.AuthorID,
		Description:     upload.Description,
		RatingScore:     0,
		RatingCount:     0,
		DownloadCount:   0,
		RequiresPremium: upload.RequiresPremium,
		Content:         upload.Content,
		Tags:            upload.Tags,
		CreatedAt:       now,
		UpdatedAt:       now,
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
	if strings.Contains(strings.ToLower(entry.Name), needle) {
		return true
	}
	if strings.Contains(strings.ToLower(entry.Description), needle) {
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
			return entries[i].DownloadCount > entries[j].DownloadCount
		case "updated":
			return entries[i].UpdatedAt.After(entries[j].UpdatedAt)
		default:
			return entries[i].RatingScore > entries[j].RatingScore
		}
	})
}
