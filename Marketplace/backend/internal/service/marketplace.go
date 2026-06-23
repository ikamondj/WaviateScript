package service

import (
	"context"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/validation"
)

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

	return service.store.Search(ctx, query)
}

func (service *Service) Upload(ctx context.Context, request domain.UploadRequest) (domain.ScriptEntry, error) {
	if request.PayloadFormat == "" {
		request.PayloadFormat = "unknown"
	}

	if err := validation.ValidateUpload(request); err != nil {
		return domain.ScriptEntry{}, err
	}

	// TODO: Queue deeper static analysis and sandbox verification before publishing.
	return service.store.SaveUpload(ctx, request)
}
