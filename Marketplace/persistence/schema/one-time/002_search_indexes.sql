CREATE INDEX IF NOT EXISTS idx_script_entries_published_rating
    ON script_entries (is_published, rating_average DESC, download_count DESC);

CREATE INDEX IF NOT EXISTS idx_script_entries_author
    ON script_entries (author_id, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_script_versions_script_created
    ON script_versions (script_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_tags_name
    ON tags (name);

CREATE INDEX IF NOT EXISTS idx_script_entries_text_search
    ON script_entries
    USING gin (to_tsvector('english', title || ' ' || summary));
