WITH users AS (
    INSERT INTO marketplace_users (handle, display_name, email)
    VALUES
        ('phasefold', 'Phase Fold', 'phasefold@example.local'),
        ('modmatrix', 'Mod Matrix', 'modmatrix@example.local')
    ON CONFLICT (handle) DO UPDATE
        SET display_name = EXCLUDED.display_name,
            updated_at = now()
    RETURNING id, handle
),
all_users AS (
    SELECT id, handle FROM users
    UNION
    SELECT id, handle FROM marketplace_users WHERE handle IN ('phasefold', 'modmatrix')
),
scripts AS (
    INSERT INTO script_entries (author_id, slug, title, summary, license, rating_average, rating_count, download_count, is_published)
    SELECT id, 'glacial-padfield', 'Glacial Padfield',
           'Slow evolving sample shader for crystalline pads and wide stereo drift.',
           'MIT', 4.90, 42, 18420, true
    FROM all_users
    WHERE handle = 'phasefold'
    ON CONFLICT (slug) DO UPDATE
        SET summary = EXCLUDED.summary,
            rating_average = EXCLUDED.rating_average,
            rating_count = EXCLUDED.rating_count,
            download_count = EXCLUDED.download_count,
            is_published = EXCLUDED.is_published,
            updated_at = now()
    RETURNING id, slug
)
INSERT INTO script_versions (script_id, version_label, payload_format, source_text, validation_status)
SELECT id, '0.1.0', 'unknown', '// TODO: Demo Waviate script source goes here.', 'pending'
FROM scripts
ON CONFLICT (script_id, version_label) DO NOTHING;
