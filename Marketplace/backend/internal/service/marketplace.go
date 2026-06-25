package service

import (
	"context"
	"errors"
	"strings"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/sourcecode"
	"github.com/waviate-script/marketplace/backend/internal/validation"
)

var ErrAdminUnavailable = errors.New("marketplace admin actions are unavailable for this store")
var ErrUploadLimitExceeded = errors.New("upload limit exceeded for this account")

type Service struct {
	store persistence.Store
	clock func() time.Time
}

func NewService(store persistence.Store) *Service {
	return &Service{
		store: store,
		clock: func() time.Time { return time.Now().UTC() },
	}
}

func (service *Service) Search(ctx context.Context, query domain.SearchQuery) (domain.SearchResult, error) {
	if query.Page <= 0 {
		query.Page = 1
	}
	if query.PageSize <= 0 || query.PageSize > 100 {
		query.PageSize = 20
	}
	if query.Sort == "" {
		query.Sort = "rating"
	}
	query.QueryMode = normalizeMatchMode(query.QueryMode)
	query.UserMode = normalizeMatchMode(query.UserMode)
	query.IncludedTags = normalizeTags(query.IncludedTags)
	query.ExcludedTags = normalizeTags(query.ExcludedTags)

	result, err := service.store.Search(ctx, query)
	if err != nil {
		return domain.SearchResult{}, err
	}

	for index := range result.Entries {
		source, err := sourcecode.RecoverFormatted(result.Entries[index].Content)
		if err != nil {
			return domain.SearchResult{}, err
		}
		result.Entries[index].Content = source
	}

	return result, nil
}

func (service *Service) ListTags(ctx context.Context) ([]string, error) {
	return service.store.ListTags(ctx)
}

func (service *Service) ClearMarketplaceData(ctx context.Context) (domain.AdminActionResult, error) {
	adminStore, ok := service.store.(persistence.AdminStore)
	if !ok {
		return domain.AdminActionResult{}, ErrAdminUnavailable
	}

	if err := adminStore.ClearAll(ctx); err != nil {
		return domain.AdminActionResult{}, err
	}

	return domain.AdminActionResult{
		Message: "Marketplace data tables cleared.",
	}, nil
}

func (service *Service) RunSeedData(ctx context.Context) (domain.AdminActionResult, error) {
	adminStore, ok := service.store.(persistence.AdminStore)
	if !ok {
		return domain.AdminActionResult{}, ErrAdminUnavailable
	}

	scripts, err := adminStore.RunSeedScripts(ctx)
	if err != nil {
		return domain.AdminActionResult{}, err
	}

	return domain.AdminActionResult{
		Message: "Marketplace seed scripts applied.",
		Scripts: scripts,
	}, nil
}

func (service *Service) CompileCheck(ctx context.Context, request domain.CompileCheckRequest) (domain.CompileCheckResult, error) {
	if err := sourcecode.ValidateSource(request.Content); err != nil {
		return domain.CompileCheckResult{}, err
	}
	if err := sourcecode.RunStaticRoutine(ctx, request.Content); err != nil {
		return domain.CompileCheckResult{}, err
	}

	return domain.CompileCheckResult{
		Passed:         true,
		CompilerStatus: compilerStatus(),
	}, nil
}

func (service *Service) Upload(ctx context.Context, request domain.UploadRequest) (domain.ScriptEntry, error) {
	user, ok := auth.UserFromContext(ctx)
	if !ok {
		return domain.ScriptEntry{}, auth.ErrUnauthenticated
	}

	request = normalizeUpload(request)
	request.AuthorID = user.AuthorID

	if err := validation.ValidateUpload(request); err != nil {
		return domain.ScriptEntry{}, err
	}

	uploadCount, err := service.store.CountUploadsByAuthor(ctx, user.AuthorID)
	if err != nil {
		return domain.ScriptEntry{}, err
	}
	if !userCanUpload(user, uploadCount, service.clock()) {
		return domain.ScriptEntry{}, ErrUploadLimitExceeded
	}

	preparedContent, err := sourcecode.PrepareForStorage(ctx, request.Content)
	if err != nil {
		return domain.ScriptEntry{}, err
	}

	request.Content = preparedContent
	return service.store.SaveUpload(ctx, request)
}

func (service *Service) Me(ctx context.Context) (domain.AuthenticatedUser, error) {
	user, ok := auth.UserFromContext(ctx)
	if !ok {
		return domain.AuthenticatedUser{}, auth.ErrUnauthenticated
	}

	uploadCount, err := service.store.CountUploadsByAuthor(ctx, user.AuthorID)
	if err != nil {
		return domain.AuthenticatedUser{}, err
	}

	return withUploadStatus(user, uploadCount, service.clock()), nil
}

func (service *Service) SourceCode(ctx context.Context, entryID string) (domain.SourceCodeResponse, error) {
	entryID = strings.TrimSpace(entryID)
	if entryID == "" {
		return domain.SourceCodeResponse{}, errors.New("entry id is required")
	}

	entry, err := service.store.GetByID(ctx, entryID)
	if err != nil {
		return domain.SourceCodeResponse{}, err
	}

	source, err := sourcecode.RecoverFormatted(entry.Content)
	if err != nil {
		return domain.SourceCodeResponse{}, err
	}

	return domain.SourceCodeResponse{
		EntryID: entry.ID,
		Source:  source,
	}, nil
}

func normalizeUpload(request domain.UploadRequest) domain.UploadRequest {
	request.Name = strings.TrimSpace(request.Name)
	request.AuthorID = strings.TrimSpace(request.AuthorID)
	request.Description = strings.TrimSpace(request.Description)

	seenTags := make(map[string]struct{}, len(request.Tags))
	normalizedTags := make([]string, 0, len(request.Tags))
	for _, tag := range request.Tags {
		normalized := strings.TrimSpace(strings.ToLower(tag))
		if normalized == "" {
			continue
		}
		if _, ok := seenTags[normalized]; ok {
			continue
		}

		seenTags[normalized] = struct{}{}
		normalizedTags = append(normalizedTags, normalized)
	}
	request.Tags = normalizedTags

	return request
}

func normalizeMatchMode(mode string) string {
	switch strings.ToLower(strings.TrimSpace(mode)) {
	case "exact":
		return domain.MatchModeExact
	case "begins", "begin", "starts", "starts_with", "starts-with", "prefix":
		return domain.MatchModeStartsWith
	default:
		return domain.MatchModeContains
	}
}

func normalizeTags(tags []string) []string {
	seenTags := make(map[string]struct{}, len(tags))
	normalizedTags := make([]string, 0, len(tags))
	for _, tag := range tags {
		normalized := strings.TrimSpace(strings.ToLower(tag))
		if normalized == "" {
			continue
		}
		if _, ok := seenTags[normalized]; ok {
			continue
		}

		seenTags[normalized] = struct{}{}
		normalizedTags = append(normalizedTags, normalized)
	}

	return normalizedTags
}

func userCanUpload(user domain.AuthenticatedUser, uploadCount int, now time.Time) bool {
	user = withUploadStatus(user, uploadCount, now)
	return user.UploadLimitUnlimited || uploadCount < user.UploadLimit
}

func withUploadStatus(user domain.AuthenticatedUser, uploadCount int, now time.Time) domain.AuthenticatedUser {
	user.Plan = normalizePlan(user.Plan)
	user.SubscriptionActive = user.SubscriptionExpiresAt != nil && user.SubscriptionExpiresAt.After(now)
	user.UploadCount = uploadCount

	switch {
	case user.Creator:
		user.UploadLimit = 0
		user.UploadLimitUnlimited = true
	case user.Plan == domain.PlanPremium && user.SubscriptionActive:
		user.UploadLimit = 200
		user.UploadLimitUnlimited = false
	default:
		user.UploadLimit = 5
		user.UploadLimitUnlimited = false
	}

	return user
}

func normalizePlan(plan string) string {
	switch strings.ToLower(strings.TrimSpace(plan)) {
	case domain.PlanPremium:
		return domain.PlanPremium
	default:
		return domain.PlanStandard
	}
}

func compilerStatus() domain.CompilerStatus {
	status := sourcecode.NativeCompilerStatus()
	return domain.CompilerStatus{
		Available: status.Available,
		Required:  status.Required,
		Mode:      status.Mode,
		Detail:    status.Detail,
	}
}
