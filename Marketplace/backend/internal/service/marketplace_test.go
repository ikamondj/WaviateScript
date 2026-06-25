package service

import (
	"context"
	"testing"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/sourcecode"
)

func TestSearchReturnsFormattedSourceContent(t *testing.T) {
	stored, err := sourcecode.Compress("int add(int left,int right){return left+right;}")
	if err != nil {
		t.Fatalf("compress source: %v", err)
	}

	store := fakeStore{
		entry: domain.ScriptEntry{
			ID:        "entry-1",
			Name:      "Adder",
			AuthorID:  "author-1",
			Content:   stored,
			CreatedAt: time.Now().UTC(),
			UpdatedAt: time.Now().UTC(),
		},
	}

	result, err := NewService(store).Search(context.Background(), domain.SearchQuery{Page: 1, PageSize: 1})
	if err != nil {
		t.Fatalf("search: %v", err)
	}
	if len(result.Entries) != 1 {
		t.Fatalf("expected one result, got %d", len(result.Entries))
	}

	want := "int add(int left, int right) {\n    return left + right;\n}\n"
	if result.Entries[0].Content != want {
		t.Fatalf("unexpected source content:\nwant:\n%s\ngot:\n%s", want, result.Entries[0].Content)
	}
}

func TestCompileCheckRunsSourceValidationAndCompilerHook(t *testing.T) {
	t.Setenv("MARKETPLACE_REQUIRE_NATIVE_COMPILER", "")

	result, err := NewService(fakeStore{}).CompileCheck(context.Background(), domain.CompileCheckRequest{
		Content: "float SampleProcess(WaviateSample& wav) { return 0.0f; }",
	})
	if err != nil {
		t.Fatalf("compile check: %v", err)
	}
	if !result.Passed {
		t.Fatalf("expected compile check to pass")
	}
	if result.CompilerStatus.Mode == "" {
		t.Fatalf("expected compiler status in response")
	}
}

type fakeStore struct {
	entry domain.ScriptEntry
}

func (store fakeStore) Search(context.Context, domain.SearchQuery) (domain.SearchResult, error) {
	return domain.SearchResult{
		Entries: []domain.ScriptEntry{store.entry},
		Page: domain.PageInfo{
			Page:       1,
			PageSize:   1,
			Total:      1,
			TotalPages: 1,
		},
	}, nil
}

func (store fakeStore) SaveUpload(context.Context, domain.UploadRequest) (domain.ScriptEntry, error) {
	return domain.ScriptEntry{}, nil
}

func (store fakeStore) GetByID(context.Context, string) (domain.ScriptEntry, error) {
	if store.entry.ID == "" {
		return domain.ScriptEntry{}, persistence.ErrNotFound
	}

	return store.entry, nil
}
