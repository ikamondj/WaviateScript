CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE INDEX IF NOT EXISTS idx_user_credentials_user
    ON user_credentials (user_id);

CREATE INDEX IF NOT EXISTS idx_authors_user
    ON authors (user_id);

CREATE INDEX IF NOT EXISTS idx_authors_name_trgm
    ON authors
    USING gin (name gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_authors_lower_name_prefix
    ON authors (lower(name) text_pattern_ops);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_rating
    ON waviate_scripts (rating_score DESC, rating_count DESC, download_count DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_downloads
    ON waviate_scripts (download_count DESC, rating_score DESC, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_updated
    ON waviate_scripts (updated_at DESC, rating_score DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_author_updated
    ON waviate_scripts (author_id, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_requires_premium
    ON waviate_scripts (requires_premium, rating_score DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_name_trgm
    ON waviate_scripts
    USING gin (name gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_lower_name_prefix
    ON waviate_scripts (lower(name) text_pattern_ops);

CREATE INDEX IF NOT EXISTS idx_tags_name
    ON tags (name);

CREATE INDEX IF NOT EXISTS idx_waviate_script_tags_tag
    ON waviate_script_tags (tag_id, script_id);

CREATE INDEX IF NOT EXISTS idx_waviate_script_downloads_script_created
    ON waviate_script_downloads (script_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_waviate_scripts_text_search
    ON waviate_scripts
    USING gin (to_tsvector('english', name || ' ' || description));
