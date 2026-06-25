package postgres

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"

	"gorm.io/gorm"

	"github.com/waviate-script/marketplace/backend/internal/domain"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
)

type GormStore struct {
	db      *gorm.DB
	dataDir string
}

type Options struct {
	DataDir string
}

func NewGormStore(db *gorm.DB, options ...Options) *GormStore {
	store := &GormStore{
		db:      db,
		dataDir: "../persistence/data",
	}

	if len(options) > 0 && strings.TrimSpace(options[0].DataDir) != "" {
		store.dataDir = options[0].DataDir
	}

	return store
}

var _ persistence.Store = (*GormStore)(nil)
var _ persistence.AdminStore = (*GormStore)(nil)

type WaviateScript struct {
	ID              string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	AuthorID        string `gorm:"type:uuid;not null"`
	Author          Author `gorm:"foreignKey:AuthorID"`
	Slug            string `gorm:"unique;not null"`
	Name            string `gorm:"not null"`
	Description     string `gorm:"default:''"`
	RatingScore     float64
	RatingCount     int
	DownloadCount   int
	RequiresPremium bool
	Content         string
	CreatedAt       time.Time
	UpdatedAt       time.Time
	Tags            []Tag `gorm:"many2many:waviate_script_tags;joinForeignKey:script_id;joinReferences:tag_id"`
}

type Author struct {
	ID        string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	UserID    string `gorm:"type:uuid;not null"`
	Name      string `gorm:"not null"`
	Slug      string `gorm:"unique;not null"`
	CreatedAt time.Time
	UpdatedAt time.Time
}

type Tag struct {
	ID   string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	Name string `gorm:"unique;not null"`
}

func (s *GormStore) Search(ctx context.Context, query domain.SearchQuery) (domain.SearchResult, error) {
	db := s.db.WithContext(ctx).Model(&WaviateScript{})

	if q := strings.TrimSpace(query.Query); q != "" {
		db = db.Where("name ILIKE ? OR description ILIKE ?", "%"+q+"%", "%"+q+"%")
	}
	if user := strings.TrimSpace(query.User); user != "" {
		db = db.Joins("JOIN authors ON authors.id = waviate_scripts.author_id").
			Where("authors.name ILIKE ?", "%"+user+"%")
	}
	if tag := strings.TrimSpace(query.Tag); tag != "" {
		db = db.Where("EXISTS (SELECT 1 FROM waviate_script_tags wst JOIN tags t ON t.id = wst.tag_id WHERE wst.script_id = waviate_scripts.id AND t.name = ?)", strings.ToLower(tag))
	}

	var total int64
	if err := db.Count(&total).Error; err != nil {
		return domain.SearchResult{}, err
	}

	pageSize := query.PageSize
	if pageSize <= 0 {
		pageSize = 20
	}
	page := query.Page
	if page <= 0 {
		page = 1
	}
	totalPages := int(max(1, (total+int64(pageSize)-1)/int64(pageSize)))
	if page > totalPages {
		page = totalPages
	}

	switch query.Sort {
	case "downloads":
		db = db.Order("download_count DESC, rating_score DESC, updated_at DESC")
	case "updated":
		db = db.Order("updated_at DESC, rating_score DESC")
	default:
		db = db.Order("rating_score DESC, rating_count DESC, download_count DESC")
	}

	var scripts []WaviateScript
	if err := db.Preload("Author").Preload("Tags").
		Limit(pageSize).Offset((page - 1) * pageSize).
		Find(&scripts).Error; err != nil {
		return domain.SearchResult{}, err
	}

	entries := make([]domain.ScriptEntry, 0, len(scripts))
	for _, script := range scripts {
		entries = append(entries, mapToDomain(script))
	}

	return domain.SearchResult{
		Entries: entries,
		Page: domain.PageInfo{
			Page:       page,
			PageSize:   pageSize,
			Total:      int(total),
			TotalPages: totalPages,
		},
	}, nil
}

func (s *GormStore) SaveUpload(ctx context.Context, upload domain.UploadRequest) (domain.ScriptEntry, error) {
	var entry domain.ScriptEntry
	err := s.db.WithContext(ctx).Transaction(func(tx *gorm.DB) error {
		slug := fmt.Sprintf("%s-%d", slugify(upload.Name), time.Now().UTC().Unix())

		script := WaviateScript{
			AuthorID:        upload.AuthorID,
			Slug:            slug,
			Name:            upload.Name,
			Description:     upload.Description,
			RequiresPremium: upload.RequiresPremium,
			Content:         upload.Content,
		}

		// Ensure tags exist
		for _, tagName := range upload.Tags {
			normalized := strings.TrimSpace(strings.ToLower(tagName))
			if normalized == "" {
				continue
			}
			var tag Tag
			if err := tx.Where(Tag{Name: normalized}).FirstOrCreate(&tag).Error; err != nil {
				return err
			}
			script.Tags = append(script.Tags, tag)
		}

		if err := tx.Create(&script).Error; err != nil {
			return err
		}

		// reload with author
		if err := tx.Preload("Author").Preload("Tags").First(&script, "id = ?", script.ID).Error; err != nil {
			return err
		}

		entry = mapToDomain(script)
		return nil
	})

	return entry, err
}

func (s *GormStore) GetByID(ctx context.Context, id string) (domain.ScriptEntry, error) {
	var script WaviateScript
	err := s.db.WithContext(ctx).Preload("Author").Preload("Tags").First(&script, "id = ?", id).Error
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return domain.ScriptEntry{}, persistence.ErrNotFound
	}
	if err != nil {
		return domain.ScriptEntry{}, err
	}

	return mapToDomain(script), nil
}

func (s *GormStore) ClearAll(ctx context.Context) error {
	return s.db.WithContext(ctx).Exec(`
TRUNCATE TABLE
    waviate_script_downloads,
    waviate_script_ratings,
    waviate_script_tags,
    waviate_scripts,
    tags,
    authors,
    user_credentials,
    marketplace_users
RESTART IDENTITY CASCADE;
`).Error
}

func (s *GormStore) RunSeedScripts(ctx context.Context) ([]domain.SeedScriptResult, error) {
	files, err := os.ReadDir(s.dataDir)
	if err != nil {
		return nil, fmt.Errorf("read seed data directory %q: %w", s.dataDir, err)
	}

	sort.Slice(files, func(i, j int) bool {
		return files[i].Name() < files[j].Name()
	})

	results := make([]domain.SeedScriptResult, 0, len(files))
	err = s.db.WithContext(ctx).Transaction(func(tx *gorm.DB) error {
		for _, file := range files {
			if file.IsDir() || !strings.HasSuffix(file.Name(), ".sql") {
				continue
			}

			scriptPath := filepath.Join(s.dataDir, file.Name())
			contents, err := os.ReadFile(scriptPath)
			if err != nil {
				return fmt.Errorf("read seed data script %q: %w", file.Name(), err)
			}

			if err := tx.Exec(string(contents)).Error; err != nil {
				return fmt.Errorf("apply seed data script %q: %w", file.Name(), err)
			}

			results = append(results, domain.SeedScriptResult{Name: file.Name()})
		}

		if len(results) == 0 {
			return fmt.Errorf("no seed data scripts found in %q", s.dataDir)
		}

		return nil
	})
	if err != nil {
		return nil, err
	}

	return results, nil
}

func mapToDomain(script WaviateScript) domain.ScriptEntry {
	tags := make([]string, 0, len(script.Tags))
	for _, t := range script.Tags {
		tags = append(tags, t.Name)
	}

	return domain.ScriptEntry{
		ID:              script.ID,
		Name:            script.Name,
		AuthorID:        script.AuthorID,
		AuthorName:      script.Author.Name,
		Description:     script.Description,
		RatingScore:     script.RatingScore,
		RatingCount:     script.RatingCount,
		DownloadCount:   script.DownloadCount,
		RequiresPremium: script.RequiresPremium,
		Content:         script.Content,
		Tags:            tags,
		CreatedAt:       script.CreatedAt,
		UpdatedAt:       script.UpdatedAt,
	}
}

func max(a, b int64) int64 {
	if a > b {
		return a
	}
	return b
}

var slugInvalidChars = regexp.MustCompile(`[^a-z0-9]+`)

func slugify(value string) string {
	slug := strings.Trim(slugInvalidChars.ReplaceAllString(strings.ToLower(value), "-"), "-")
	if slug == "" {
		return "waviate-script"
	}
	return slug
}
