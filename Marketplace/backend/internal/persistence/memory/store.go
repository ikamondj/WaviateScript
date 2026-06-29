package memory

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
)

type Store struct {
	mu       sync.Mutex
	entries  []domain.ScriptEntry
	users    map[string]domain.AuthenticatedUser
	sessions map[string]domain.SessionToken
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
				Content:         "float SampleProcess(const WaviateSample& wav) { return wav.getIncomingSample(); }",
				Tags:            []string{"pads", "ambient", "sample-shader"},
				CreatedAt:       time.Date(2026, 6, 1, 0, 0, 0, 0, time.UTC),
				UpdatedAt:       time.Date(2026, 6, 10, 0, 0, 0, 0, time.UTC),
			},
		},
		users:    make(map[string]domain.AuthenticatedUser),
		sessions: make(map[string]domain.SessionToken),
	}
}

var _ persistence.Store = (*Store)(nil)

func (store *Store) Search(_ context.Context, query domain.SearchQuery) (domain.SearchResult, error) {
	store.mu.Lock()
	entries := append([]domain.ScriptEntry{}, store.entries...)
	store.mu.Unlock()

	filtered := make([]domain.ScriptEntry, 0, len(entries))
	nameNeedle := strings.ToLower(strings.TrimSpace(query.Query))
	userNeedle := strings.ToLower(strings.TrimSpace(query.User))

	for _, entry := range entries {
		if nameNeedle != "" && !matchText(entry.Name, nameNeedle, query.QueryMode) {
			continue
		}

		if userNeedle != "" && !matchText(entry.AuthorName, userNeedle, query.UserMode) {
			continue
		}

		if !hasIncludedTags(entry, query.IncludedTags) || hasExcludedTags(entry, query.ExcludedTags) {
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

func (store *Store) ListTags(_ context.Context) ([]string, error) {
	store.mu.Lock()
	defer store.mu.Unlock()

	tagSet := map[string]struct{}{}
	for _, entry := range store.entries {
		for _, tag := range entry.Tags {
			tagSet[tag] = struct{}{}
		}
	}

	tags := make([]string, 0, len(tagSet))
	for tag := range tagSet {
		tags = append(tags, tag)
	}
	sort.Strings(tags)

	return tags, nil
}

func (store *Store) SaveUpload(_ context.Context, upload domain.UploadRequest) (domain.ScriptEntry, error) {
	store.mu.Lock()
	defer store.mu.Unlock()

	now := time.Now().UTC()
	authorName := upload.AuthorID
	if user := store.findUserByAuthorID(upload.AuthorID); user.AuthorName != "" {
		authorName = user.AuthorName
	}

	entry := domain.ScriptEntry{
		ID:              fmt.Sprintf("draft-%d", len(store.entries)+1),
		Name:            upload.Name,
		AuthorID:        upload.AuthorID,
		AuthorName:      authorName,
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
	store.mu.Lock()
	defer store.mu.Unlock()

	for _, entry := range store.entries {
		if entry.ID == id {
			return entry, nil
		}
	}

	return domain.ScriptEntry{}, persistence.ErrNotFound
}

func (store *Store) CountUploadsByAuthor(_ context.Context, authorID string) (int, error) {
	store.mu.Lock()
	defer store.mu.Unlock()

	count := 0
	for _, entry := range store.entries {
		if entry.AuthorID == authorID {
			count++
		}
	}

	return count, nil
}

func (store *Store) UpsertOAuthUser(_ context.Context, identity domain.OAuthIdentity) (domain.AuthenticatedUser, error) {
	store.mu.Lock()
	defer store.mu.Unlock()

	subject := firstNonEmpty(identity.Subject, identity.Email, identity.Username)
	if strings.TrimSpace(subject) == "" {
		return domain.AuthenticatedUser{}, fmt.Errorf("oauth subject is required")
	}

	userID := "user-" + slugify(identity.Provider+"-"+subject)
	displayName := firstNonEmpty(identity.DisplayName, identity.Username, identity.Email, "Waviate Creator")
	authorSlug := slugify(displayName)
	user := domain.AuthenticatedUser{
		ID:          userID,
		DisplayName: displayName,
		Email:       identity.Email,
		AuthorID:    "author-" + authorSlug,
		AuthorName:  displayName,
		Plan:        domain.PlanStandard,
	}

	if existing, ok := store.users[userID]; ok {
		user.Plan = existing.Plan
		user.Creator = existing.Creator
		user.SubscriptionExpiresAt = existing.SubscriptionExpiresAt
	}

	store.users[userID] = user
	return user, nil
}

func (store *Store) CreateSession(_ context.Context, token domain.SessionToken) error {
	store.mu.Lock()
	defer store.mu.Unlock()

	store.sessions[token.TokenHash] = token
	return nil
}

func (store *Store) FindSessionByTokenHash(_ context.Context, tokenHash string, now time.Time) (domain.AuthenticatedUser, error) {
	store.mu.Lock()
	defer store.mu.Unlock()

	session, ok := store.sessions[tokenHash]
	if !ok || !session.ExpiresAt.After(now) {
		return domain.AuthenticatedUser{}, persistence.ErrNotFound
	}

	user, ok := store.users[session.UserID]
	if !ok {
		return domain.AuthenticatedUser{}, persistence.ErrNotFound
	}

	return user, nil
}

func (store *Store) findUserByAuthorID(authorID string) domain.AuthenticatedUser {
	for _, user := range store.users {
		if user.AuthorID == authorID {
			return user
		}
	}

	return domain.AuthenticatedUser{}
}

func matchText(value string, needle string, mode string) bool {
	value = strings.ToLower(value)
	switch mode {
	case domain.MatchModeExact:
		return value == needle
	case domain.MatchModeStartsWith:
		return strings.HasPrefix(value, needle)
	default:
		return strings.Contains(value, needle)
	}
}

func hasIncludedTags(entry domain.ScriptEntry, tags []string) bool {
	for _, tag := range tags {
		if !hasTag(entry, tag) {
			return false
		}
	}

	return true
}

func hasExcludedTags(entry domain.ScriptEntry, tags []string) bool {
	for _, tag := range tags {
		if hasTag(entry, tag) {
			return true
		}
	}

	return false
}

func hasTag(entry domain.ScriptEntry, tag string) bool {
	for _, entryTag := range entry.Tags {
		if strings.EqualFold(entryTag, tag) {
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

func slugify(value string) string {
	value = strings.ToLower(strings.TrimSpace(value))
	var out strings.Builder
	previousDash := false
	for _, r := range value {
		isAllowed := (r >= 'a' && r <= 'z') || (r >= '0' && r <= '9')
		if isAllowed {
			out.WriteRune(r)
			previousDash = false
			continue
		}
		if !previousDash && out.Len() > 0 {
			out.WriteByte('-')
			previousDash = true
		}
	}

	slug := strings.Trim(out.String(), "-")
	if slug == "" {
		return "waviate-creator"
	}

	return slug
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if trimmed := strings.TrimSpace(value); trimmed != "" {
			return trimmed
		}
	}

	return ""
}
