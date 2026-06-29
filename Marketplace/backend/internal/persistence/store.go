package persistence

import (
	"context"
	"errors"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

var ErrNotFound = errors.New("marketplace entry not found")

type Store interface {
	Search(ctx context.Context, query domain.SearchQuery) (domain.SearchResult, error)
	ListTags(ctx context.Context) ([]string, error)
	SaveUpload(ctx context.Context, upload domain.UploadRequest) (domain.ScriptEntry, error)
	GetByID(ctx context.Context, id string) (domain.ScriptEntry, error)
	CountUploadsByAuthor(ctx context.Context, authorID string) (int, error)
	UpsertOAuthUser(ctx context.Context, identity domain.OAuthIdentity) (domain.AuthenticatedUser, error)
	CreateSession(ctx context.Context, token domain.SessionToken) error
	FindSessionByTokenHash(ctx context.Context, tokenHash string, now time.Time) (domain.AuthenticatedUser, error)
}

type AdminStore interface {
	ClearAll(ctx context.Context) error
	RunSeedScripts(ctx context.Context) ([]domain.SeedScriptResult, error)
}
