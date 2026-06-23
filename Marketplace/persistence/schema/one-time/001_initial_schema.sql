CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS marketplace_users (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    handle text NOT NULL UNIQUE,
    display_name text NOT NULL,
    email text UNIQUE,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS auth_identities (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    provider text NOT NULL,
    provider_subject text NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (provider, provider_subject)
);

CREATE TABLE IF NOT EXISTS script_entries (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    author_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE RESTRICT,
    slug text NOT NULL UNIQUE,
    title text NOT NULL,
    summary text NOT NULL,
    license text NOT NULL DEFAULT 'unspecified',
    rating_average numeric(3, 2) NOT NULL DEFAULT 0,
    rating_count integer NOT NULL DEFAULT 0,
    download_count integer NOT NULL DEFAULT 0,
    is_published boolean NOT NULL DEFAULT false,
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS script_versions (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    script_id uuid NOT NULL REFERENCES script_entries(id) ON DELETE CASCADE,
    version_label text NOT NULL,
    payload_format text NOT NULL DEFAULT 'unknown',
    payload bytea,
    source_text text,
    validation_status text NOT NULL DEFAULT 'pending',
    validation_notes text,
    created_at timestamptz NOT NULL DEFAULT now(),
    UNIQUE (script_id, version_label),
    CHECK (payload_format IN ('unknown', 'raw', 'compressed', 'encrypted')),
    CHECK (validation_status IN ('pending', 'accepted', 'rejected'))
);

CREATE TABLE IF NOT EXISTS tags (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    name text NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS script_tags (
    script_id uuid NOT NULL REFERENCES script_entries(id) ON DELETE CASCADE,
    tag_id uuid NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
    PRIMARY KEY (script_id, tag_id)
);

CREATE TABLE IF NOT EXISTS script_ratings (
    script_id uuid NOT NULL REFERENCES script_entries(id) ON DELETE CASCADE,
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    rating integer NOT NULL CHECK (rating BETWEEN 1 AND 5),
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (script_id, user_id)
);

CREATE TABLE IF NOT EXISTS script_downloads (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    script_id uuid NOT NULL REFERENCES script_entries(id) ON DELETE CASCADE,
    user_id uuid REFERENCES marketplace_users(id) ON DELETE SET NULL,
    client_kind text NOT NULL DEFAULT 'web',
    created_at timestamptz NOT NULL DEFAULT now()
);

COMMENT ON COLUMN script_versions.payload_format IS 'TODO: Finalize raw/compressed/encrypted marketplace package format.';
COMMENT ON COLUMN script_versions.payload IS 'TODO: Store packaged script bytes after upload validation is defined.';
COMMENT ON COLUMN script_versions.source_text IS 'TODO: Keep or remove depending on marketplace packaging and encryption requirements.';
