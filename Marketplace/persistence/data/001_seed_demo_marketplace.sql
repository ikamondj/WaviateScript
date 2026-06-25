INSERT INTO marketplace_users (display_name, email)
SELECT display_name, email
FROM (
    VALUES
        ('Phase Fold', 'phasefold@example.local'),
        ('Mod Matrix', 'modmatrix@example.local'),
        ('Null Carrier', 'nullcarrier@example.local'),
        ('Signal Weld', 'signalweld@example.local'),
        ('Noct Shape', 'noctshape@example.local')
) AS seed_users(display_name, email)
ON CONFLICT (email) DO UPDATE
    SET display_name = EXCLUDED.display_name,
        updated_at = now();

UPDATE marketplace_users
SET plan = 'premium',
    subscription_expires_at = '2027-06-25T00:00:00Z'::timestamptz,
    creator = false,
    updated_at = now()
WHERE email = 'modmatrix@example.local';

UPDATE marketplace_users
SET plan = 'standard',
    subscription_expires_at = NULL,
    creator = true,
    updated_at = now()
WHERE email = 'phasefold@example.local';

UPDATE marketplace_users
SET plan = 'standard',
    subscription_expires_at = NULL,
    creator = false,
    updated_at = now()
WHERE email IN (
    'nullcarrier@example.local',
    'signalweld@example.local',
    'noctshape@example.local'
);

INSERT INTO user_credentials (
    user_id,
    provider,
    provider_subject,
    provider_username,
    provider_email,
    last_login_at
)
SELECT
    marketplace_users.id,
    'google',
    'demo:' || marketplace_users.email,
    split_part(marketplace_users.email, '@', 1),
    marketplace_users.email,
    now()
FROM marketplace_users
WHERE marketplace_users.email IN (
    'phasefold@example.local',
    'modmatrix@example.local',
    'nullcarrier@example.local',
    'signalweld@example.local',
    'noctshape@example.local'
)
ON CONFLICT (provider, provider_subject) DO UPDATE
    SET user_id = EXCLUDED.user_id,
        provider_username = EXCLUDED.provider_username,
        provider_email = EXCLUDED.provider_email,
        last_login_at = EXCLUDED.last_login_at,
        updated_at = now();

INSERT INTO authors (user_id, name, slug)
SELECT marketplace_users.id, seed_authors.name, seed_authors.slug
FROM (
    VALUES
        ('phasefold@example.local', 'Phase Fold', 'phasefold'),
        ('modmatrix@example.local', 'Mod Matrix', 'modmatrix'),
        ('nullcarrier@example.local', 'Null Carrier', 'nullcarrier'),
        ('signalweld@example.local', 'Signal Weld', 'signalweld'),
        ('noctshape@example.local', 'Noct Shape', 'noctshape')
) AS seed_authors(email, name, slug)
JOIN marketplace_users ON marketplace_users.email = seed_authors.email
ON CONFLICT (slug) DO UPDATE
    SET user_id = EXCLUDED.user_id,
        name = EXCLUDED.name,
        updated_at = now();

INSERT INTO waviate_scripts (
    author_id,
    slug,
    name,
    description,
    rating_score,
    rating_count,
    download_count,
    requires_premium,
    content,
    created_at,
    updated_at
)
SELECT
    authors.id,
    seed_scripts.slug,
    seed_scripts.name,
    seed_scripts.description,
    seed_scripts.rating_score,
    seed_scripts.rating_count,
    seed_scripts.download_count,
    seed_scripts.requires_premium,
    seed_scripts.content,
    seed_scripts.created_at,
    seed_scripts.updated_at
FROM (
    VALUES
        (
            'phasefold',
            'glacial-padfield',
            'Glacial Padfield',
            'A slow evolving sample shader for crystalline pads and wide stereo drift.',
            4.90::numeric,
            42,
            18420,
            false,
            'wsc1:K07MLchJjS8oyk9OLS62TSvNSy7JzM/TSC6p0KwuSi0pLcpTSC6p0MvMKygtsa4FAA',
            '2026-06-01T00:00:00Z'::timestamptz,
            '2026-06-10T00:00:00Z'::timestamptz
        ),
        (
            'modmatrix',
            'granular-fm-bloom',
            'Granular FM Bloom',
            'Frequency shader sketch for glassy FM clusters with tight macro control.',
            4.70::numeric,
            27,
            9310,
            false,
            'wsc1:SytKLSxNzUuujC8oyk9OLS62TSvNSy7JzM/TSC6p0KwuSi0pLcpTSC6p0EvKzLOuBQA',
            '2026-05-02T00:00:00Z'::timestamptz,
            '2026-05-21T00:00:00Z'::timestamptz
        ),
        (
            'nullcarrier',
            'clean-sub-driver',
            'Clean Sub Driver',
            'A focused low-end utility script for gentle saturation and phase-safe weight.',
            4.80::numeric,
            35,
            12102,
            false,
            'wsc1:K07MLchJjS8oyk9OLS62TSvNSy7JzM/TSC6p0KwuSi0pLcpTSC6p0MvMKygtsa4FAA',
            '2026-04-05T00:00:00Z'::timestamptz,
            '2026-04-28T00:00:00Z'::timestamptz
        ),
        (
            'signalweld',
            'osc-reactor-grid',
            'OSC Reactor Grid',
            'Prototype control-reactive script intended for premium OSC workflows later.',
            4.40::numeric,
            9,
            3702,
            true,
            'wsc1:S87PKynKz4kvKMpPTi0utk0rzUsuyczP00guqdCsLkotKS3KU0guqdArS8wpTbWuBQA',
            '2026-03-01T00:00:00Z'::timestamptz,
            '2026-03-19T00:00:00Z'::timestamptz
        ),
        (
            'phasefold',
            'midi-strum-smear',
            'MIDI Strum Smear',
            'MIDI-aware sample shader for soft timing spread and chord-motion textures.',
            4.60::numeric,
            18,
            7118,
            false,
            'wsc1:y81MyYwvKMpPTi0utk0rzUsuyczP00guqdCsLkotKS3KU0guqdDLyy9Jta4FAA',
            '2026-01-29T00:00:00Z'::timestamptz,
            '2026-02-07T00:00:00Z'::timestamptz
        ),
        (
            'noctshape',
            'spectral-lantern',
            'Spectral Lantern',
            'Frequency-domain tone shaper with glowing resonant peaks and animated tilt.',
            4.50::numeric,
            14,
            6024,
            false,
            'wsc1:SytKLSxNzUuujC8oyk9OLS62TSvNSy7JzM/TSC6p0KwuSi0pLcpTSC6p0MtNTM/LLClNSbWuBQA',
            '2025-12-18T00:00:00Z'::timestamptz,
            '2026-01-14T00:00:00Z'::timestamptz
        )
) AS seed_scripts(
    author_slug,
    slug,
    name,
    description,
    rating_score,
    rating_count,
    download_count,
    requires_premium,
    content,
    created_at,
    updated_at
)
JOIN authors ON authors.slug = seed_scripts.author_slug
ON CONFLICT (slug) DO UPDATE
    SET author_id = EXCLUDED.author_id,
        name = EXCLUDED.name,
        description = EXCLUDED.description,
        rating_score = EXCLUDED.rating_score,
        rating_count = EXCLUDED.rating_count,
        download_count = EXCLUDED.download_count,
        requires_premium = EXCLUDED.requires_premium,
        content = EXCLUDED.content,
        updated_at = EXCLUDED.updated_at;
