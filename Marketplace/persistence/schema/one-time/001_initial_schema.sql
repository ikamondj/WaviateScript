CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS marketplace_users (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    display_name text NOT NULL,
    email text UNIQUE,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS user_credentials (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    provider text NOT NULL,
    provider_subject text NOT NULL,
    provider_username text,
    provider_email text,
    access_token_ciphertext text,
    refresh_token_ciphertext text,
    expires_at timestamptz,
    last_login_at timestamptz,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (provider, provider_subject)
);

CREATE TABLE IF NOT EXISTS authors (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    name text NOT NULL,
    slug text NOT NULL UNIQUE,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (user_id, slug)
);

CREATE TABLE IF NOT EXISTS waviate_scripts (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    author_id uuid NOT NULL REFERENCES authors(id) ON DELETE RESTRICT,
    slug text NOT NULL UNIQUE,
    name text NOT NULL,
    description text NOT NULL DEFAULT '',
    rating_score numeric(4, 2) NOT NULL DEFAULT 0,
    rating_count integer NOT NULL DEFAULT 0,
    download_count integer NOT NULL DEFAULT 0,
    requires_premium boolean NOT NULL DEFAULT false,
    content text NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    CHECK (rating_score >= 0 AND rating_score <= 5),
    CHECK (rating_count >= 0),
    CHECK (download_count >= 0)
);

CREATE TABLE IF NOT EXISTS tags (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    name text NOT NULL UNIQUE,
    CHECK (name ~ '^[a-z0-9][a-z0-9-]{0,63}$')
);

CREATE TABLE IF NOT EXISTS waviate_script_tags (
    script_id uuid NOT NULL REFERENCES waviate_scripts(id) ON DELETE CASCADE,
    tag_id uuid NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
    created_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (script_id, tag_id)
);

CREATE TABLE IF NOT EXISTS waviate_script_ratings (
    script_id uuid NOT NULL REFERENCES waviate_scripts(id) ON DELETE CASCADE,
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    rating integer NOT NULL CHECK (rating BETWEEN 1 AND 5),
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (script_id, user_id)
);

CREATE TABLE IF NOT EXISTS waviate_script_downloads (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    script_id uuid NOT NULL REFERENCES waviate_scripts(id) ON DELETE CASCADE,
    user_id uuid REFERENCES marketplace_users(id) ON DELETE SET NULL,
    client_kind text NOT NULL DEFAULT 'web',
    created_at timestamptz NOT NULL DEFAULT now()
);

COMMENT ON TABLE marketplace_users IS 'Login/user account records. OAuth identities live in user_credentials.';
COMMENT ON TABLE authors IS 'Public marketplace author profile linked to an authenticated marketplace user.';
COMMENT ON TABLE waviate_scripts IS 'Searchable marketplace entries for user-authored Waviate scripts.';
COMMENT ON TABLE waviate_script_tags IS 'Many-to-many tag assignments for efficient tag search, tag pages, and tag maintenance.';
COMMENT ON COLUMN waviate_scripts.content IS 'Current Waviate script content. This can later become packaged or encrypted content if distribution rules require it.';
COMMENT ON COLUMN user_credentials.access_token_ciphertext IS 'Optional encrypted OAuth access token storage. Prefer not storing provider tokens unless a workflow requires it.';
COMMENT ON COLUMN user_credentials.refresh_token_ciphertext IS 'Optional encrypted OAuth refresh token storage. Prefer not storing provider tokens unless a workflow requires it.';
