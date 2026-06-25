package service

import (
	"context"
	"errors"
	"strings"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/sourcecode"
	"github.com/waviate-script/marketplace/backend/internal/validation"
)

var ErrAdminUnavailable = errors.New("marketplace admin actions are unavailable for this store")

type Service struct {
	store persistence.Store
}

func NewService(store persistence.Store) *Service {
	return &Service{store: store}
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
	request = normalizeUpload(request)

	if err := validation.ValidateUpload(request); err != nil {
		return domain.ScriptEntry{}, err
	}

	preparedContent, err := sourcecode.PrepareForStorage(ctx, request.Content)
	if err != nil {
		return domain.ScriptEntry{}, err
	}

	request.Content = preparedContent
	return service.store.SaveUpload(ctx, request)
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

func compilerStatus() domain.CompilerStatus {
	status := sourcecode.NativeCompilerStatus()
	return domain.CompilerStatus{
		Available: status.Available,
		Required:  status.Required,
		Mode:      status.Mode,
		Detail:    status.Detail,
	}
}
