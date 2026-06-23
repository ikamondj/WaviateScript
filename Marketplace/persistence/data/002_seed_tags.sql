INSERT INTO tags (name)
VALUES
    ('ambient'),
    ('bass'),
    ('control'),
    ('experimental'),
    ('frequency-shader'),
    ('midi'),
    ('pads'),
    ('sample-shader'),
    ('spectral'),
    ('utility')
ON CONFLICT (name) DO NOTHING;

INSERT INTO script_tags (script_id, tag_id)
SELECT script_entries.id, tags.id
FROM script_entries
JOIN tags ON tags.name IN ('ambient', 'pads', 'sample-shader')
WHERE script_entries.slug = 'glacial-padfield'
ON CONFLICT DO NOTHING;
