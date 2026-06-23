package persistence

import (
	"context"
	"errors"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

var ErrNotFound = errors.New("marketplace entry not found")

type Store interface {
	Search(ctx context.Context, query domain.SearchQuery) (domain.SearchResult, error)
	SaveUpload(ctx context.Context, upload domain.UploadRequest) (domain.ScriptEntry, error)
	GetByID(ctx context.Context, id string) (domain.ScriptEntry, error)
}
