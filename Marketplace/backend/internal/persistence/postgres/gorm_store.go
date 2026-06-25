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

type MarketplaceUser struct {
	ID                    string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	DisplayName           string `gorm:"not null"`
	Email                 *string
	Plan                  string `gorm:"not null;default:standard"`
	Creator               bool
	SubscriptionExpiresAt *time.Time
	CreatedAt             time.Time
	UpdatedAt             time.Time
}

type UserCredential struct {
	ID                     string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	UserID                 string `gorm:"type:uuid;not null"`
	Provider               string `gorm:"not null"`
	ProviderSubject        string `gorm:"not null"`
	ProviderUsername       string
	ProviderEmail          string
	AccessTokenCiphertext  string
	RefreshTokenCiphertext string
	ExpiresAt              *time.Time
	LastLoginAt            *time.Time
	CreatedAt              time.Time
	UpdatedAt              time.Time
}

type AuthSession struct {
	TokenHash  string `gorm:"primaryKey"`
	UserID     string `gorm:"type:uuid;not null"`
	ExpiresAt  time.Time
	CreatedAt  time.Time
	LastSeenAt *time.Time
}

type Tag struct {
	ID   string `gorm:"type:uuid;primaryKey;default:gen_random_uuid()"`
	Name string `gorm:"unique;not null"`
}

func (s *GormStore) Search(ctx context.Context, query domain.SearchQuery) (domain.SearchResult, error) {
	db := s.db.WithContext(ctx).Model(&WaviateScript{})

	if q := strings.TrimSpace(query.Query); q != "" {
		db = applyTextMatch(db, "waviate_scripts.name", q, query.QueryMode)
	}
	if user := strings.TrimSpace(query.User); user != "" {
		db = db.Joins("JOIN authors ON authors.id = waviate_scripts.author_id").
			Scopes(func(joined *gorm.DB) *gorm.DB {
				return applyTextMatch(joined, "authors.name", user, query.UserMode)
			})
	}
	for _, tag := range query.IncludedTags {
		db = db.Where("EXISTS (SELECT 1 FROM waviate_script_tags wst JOIN tags t ON t.id = wst.tag_id WHERE wst.script_id = waviate_scripts.id AND t.name = ?)", strings.ToLower(strings.TrimSpace(tag)))
	}
	for _, tag := range query.ExcludedTags {
		db = db.Where("NOT EXISTS (SELECT 1 FROM waviate_script_tags wst JOIN tags t ON t.id = wst.tag_id WHERE wst.script_id = waviate_scripts.id AND t.name = ?)", strings.ToLower(strings.TrimSpace(tag)))
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

func (s *GormStore) ListTags(ctx context.Context) ([]string, error) {
	var tags []string
	if err := s.db.WithContext(ctx).Model(&Tag{}).Order("name ASC").Pluck("name", &tags).Error; err != nil {
		return nil, err
	}

	return tags, nil
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

func (s *GormStore) CountUploadsByAuthor(ctx context.Context, authorID string) (int, error) {
	var count int64
	if err := s.db.WithContext(ctx).Model(&WaviateScript{}).Where("author_id = ?", authorID).Count(&count).Error; err != nil {
		return 0, err
	}

	return int(count), nil
}

func (s *GormStore) UpsertOAuthUser(ctx context.Context, identity domain.OAuthIdentity) (domain.AuthenticatedUser, error) {
	var result domain.AuthenticatedUser
	err := s.db.WithContext(ctx).Transaction(func(tx *gorm.DB) error {
		now := time.Now().UTC()
		user, err := findOrCreateOAuthUser(tx, identity)
		if err != nil {
			return err
		}

		user.DisplayName = firstNonEmpty(identity.DisplayName, user.DisplayName, "Waviate Creator")
		if strings.TrimSpace(identity.Email) != "" {
			email := strings.TrimSpace(strings.ToLower(identity.Email))
			user.Email = &email
		}
		if strings.TrimSpace(user.Plan) == "" {
			user.Plan = domain.PlanStandard
		}
		if err := tx.Save(&user).Error; err != nil {
			return err
		}

		lastLogin := now
		credential := UserCredential{}
		if err := tx.Where("provider = ? AND provider_subject = ?", identity.Provider, identity.Subject).
			Assign(map[string]any{
				"user_id":                  user.ID,
				"provider_username":        identity.Username,
				"provider_email":           identity.Email,
				"access_token_ciphertext":  "",
				"refresh_token_ciphertext": "",
				"expires_at":               identity.ExpiresAt,
				"last_login_at":            &lastLogin,
			}).
			FirstOrCreate(&credential, UserCredential{
				UserID:          user.ID,
				Provider:        identity.Provider,
				ProviderSubject: identity.Subject,
			}).Error; err != nil {
			return err
		}

		author, err := ensureAuthor(tx, user)
		if err != nil {
			return err
		}

		result = mapAuthenticatedUser(user, author, now)
		return nil
	})
	if err != nil {
		return domain.AuthenticatedUser{}, err
	}

	return result, nil
}

func (s *GormStore) CreateSession(ctx context.Context, token domain.SessionToken) error {
	session := AuthSession{
		TokenHash: token.TokenHash,
		UserID:    token.UserID,
		ExpiresAt: token.ExpiresAt,
	}

	return s.db.WithContext(ctx).Create(&session).Error
}

func (s *GormStore) FindSessionByTokenHash(ctx context.Context, tokenHash string, now time.Time) (domain.AuthenticatedUser, error) {
	var session AuthSession
	err := s.db.WithContext(ctx).First(&session, "token_hash = ? AND expires_at > ?", tokenHash, now).Error
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return domain.AuthenticatedUser{}, persistence.ErrNotFound
	}
	if err != nil {
		return domain.AuthenticatedUser{}, err
	}

	lastSeen := now
	_ = s.db.WithContext(ctx).Model(&session).Update("last_seen_at", lastSeen).Error

	var user MarketplaceUser
	if err := s.db.WithContext(ctx).First(&user, "id = ?", session.UserID).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return domain.AuthenticatedUser{}, persistence.ErrNotFound
		}
		return domain.AuthenticatedUser{}, err
	}

	var author Author
	if err := s.db.WithContext(ctx).First(&author, "user_id = ?", user.ID).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return domain.AuthenticatedUser{}, persistence.ErrNotFound
		}
		return domain.AuthenticatedUser{}, err
	}

	return mapAuthenticatedUser(user, author, now), nil
}

func (s *GormStore) ClearAll(ctx context.Context) error {
	return s.db.WithContext(ctx).Exec(`
TRUNCATE TABLE
    auth_sessions,
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

func findOrCreateOAuthUser(tx *gorm.DB, identity domain.OAuthIdentity) (MarketplaceUser, error) {
	var credential UserCredential
	err := tx.First(&credential, "provider = ? AND provider_subject = ?", identity.Provider, identity.Subject).Error
	if err == nil {
		var user MarketplaceUser
		if err := tx.First(&user, "id = ?", credential.UserID).Error; err != nil {
			return MarketplaceUser{}, err
		}
		return user, nil
	}
	if !errors.Is(err, gorm.ErrRecordNotFound) {
		return MarketplaceUser{}, err
	}

	if email := strings.TrimSpace(strings.ToLower(identity.Email)); email != "" {
		var user MarketplaceUser
		err := tx.First(&user, "email = ?", email).Error
		if err == nil {
			return user, nil
		}
		if !errors.Is(err, gorm.ErrRecordNotFound) {
			return MarketplaceUser{}, err
		}
	}

	user := MarketplaceUser{
		DisplayName: firstNonEmpty(identity.DisplayName, identity.Username, identity.Email, "Waviate Creator"),
		Plan:        domain.PlanStandard,
	}
	if email := strings.TrimSpace(strings.ToLower(identity.Email)); email != "" {
		user.Email = &email
	}
	if err := tx.Create(&user).Error; err != nil {
		return MarketplaceUser{}, err
	}

	return user, nil
}

func ensureAuthor(tx *gorm.DB, user MarketplaceUser) (Author, error) {
	var author Author
	err := tx.First(&author, "user_id = ?", user.ID).Error
	if err == nil {
		if strings.TrimSpace(author.Name) != strings.TrimSpace(user.DisplayName) && strings.TrimSpace(user.DisplayName) != "" {
			author.Name = user.DisplayName
			if err := tx.Save(&author).Error; err != nil {
				return Author{}, err
			}
		}
		return author, nil
	}
	if !errors.Is(err, gorm.ErrRecordNotFound) {
		return Author{}, err
	}

	baseSlug := slugify(user.DisplayName)
	slug := baseSlug
	var existing Author
	if err := tx.First(&existing, "slug = ?", slug).Error; err == nil && existing.UserID != user.ID {
		slug = fmt.Sprintf("%s-%s", baseSlug, shortID(user.ID))
	} else if err != nil && !errors.Is(err, gorm.ErrRecordNotFound) {
		return Author{}, err
	}

	author = Author{
		UserID: user.ID,
		Name:   firstNonEmpty(user.DisplayName, "Waviate Creator"),
		Slug:   slug,
	}
	if err := tx.Create(&author).Error; err != nil {
		return Author{}, err
	}

	return author, nil
}

func mapAuthenticatedUser(user MarketplaceUser, author Author, now time.Time) domain.AuthenticatedUser {
	email := ""
	if user.Email != nil {
		email = *user.Email
	}
	plan := strings.TrimSpace(strings.ToLower(user.Plan))
	if plan == "" {
		plan = domain.PlanStandard
	}

	return domain.AuthenticatedUser{
		ID:                    user.ID,
		DisplayName:           user.DisplayName,
		Email:                 email,
		AuthorID:              author.ID,
		AuthorName:            author.Name,
		Plan:                  plan,
		Creator:               user.Creator,
		SubscriptionExpiresAt: user.SubscriptionExpiresAt,
		SubscriptionActive:    user.SubscriptionExpiresAt != nil && user.SubscriptionExpiresAt.After(now),
	}
}

func applyTextMatch(db *gorm.DB, column string, value string, mode string) *gorm.DB {
	switch mode {
	case domain.MatchModeExact:
		return db.Where("LOWER("+column+") = LOWER(?)", strings.TrimSpace(value))
	case domain.MatchModeStartsWith:
		return db.Where(column+" ILIKE ? ESCAPE '\\'", escapeLike(value)+"%")
	default:
		return db.Where(column+" ILIKE ? ESCAPE '\\'", "%"+escapeLike(value)+"%")
	}
}

func escapeLike(value string) string {
	replacer := strings.NewReplacer(`\`, `\\`, `%`, `\%`, `_`, `\_`)
	return replacer.Replace(strings.TrimSpace(value))
}

func shortID(id string) string {
	id = strings.ReplaceAll(id, "-", "")
	if len(id) > 8 {
		return id[:8]
	}
	if id == "" {
		return "user"
	}

	return id
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if trimmed := strings.TrimSpace(value); trimmed != "" {
			return trimmed
		}
	}

	return ""
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
