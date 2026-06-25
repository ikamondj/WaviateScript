ALTER TABLE marketplace_users
    ADD COLUMN IF NOT EXISTS plan text NOT NULL DEFAULT 'standard',
    ADD COLUMN IF NOT EXISTS creator boolean NOT NULL DEFAULT false,
    ADD COLUMN IF NOT EXISTS subscription_expires_at timestamptz;

ALTER TABLE marketplace_users
    DROP CONSTRAINT IF EXISTS marketplace_users_plan_check;

ALTER TABLE marketplace_users
    ADD CONSTRAINT marketplace_users_plan_check
    CHECK (plan IN ('standard', 'premium'));

CREATE TABLE IF NOT EXISTS auth_sessions (
    token_hash text PRIMARY KEY,
    user_id uuid NOT NULL REFERENCES marketplace_users(id) ON DELETE CASCADE,
    expires_at timestamptz NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now(),
    last_seen_at timestamptz
);

CREATE INDEX IF NOT EXISTS idx_auth_sessions_user_expires
    ON auth_sessions (user_id, expires_at DESC);

CREATE INDEX IF NOT EXISTS idx_auth_sessions_expires
    ON auth_sessions (expires_at);

CREATE INDEX IF NOT EXISTS idx_marketplace_users_plan_subscription
    ON marketplace_users (plan, subscription_expires_at DESC);

CREATE INDEX IF NOT EXISTS idx_marketplace_users_creator
    ON marketplace_users (creator)
    WHERE creator = true;

COMMENT ON COLUMN marketplace_users.plan IS 'Subscription plan marker. Premium access still requires an active subscription_expires_at.';
COMMENT ON COLUMN marketplace_users.creator IS 'Creator role flag for unlimited marketplace uploads. This is separate from premium.';
COMMENT ON TABLE auth_sessions IS 'Opaque marketplace session tokens stored by hash; raw tokens are returned only to clients.';
