INSERT INTO tags (name)
VALUES
    ('ambient'),
    ('bass'),
    ('control'),
    ('experimental'),
    ('fm'),
    ('frequency-shader'),
    ('midi'),
    ('osc'),
    ('pads'),
    ('premium-idea'),
    ('sample-shader'),
    ('spectral'),
    ('texture'),
    ('tone'),
    ('utility')
ON CONFLICT (name) DO NOTHING;

DELETE FROM waviate_script_tags
USING waviate_scripts
WHERE waviate_script_tags.script_id = waviate_scripts.id
  AND waviate_scripts.slug IN (
      'glacial-padfield',
      'granular-fm-bloom',
      'clean-sub-driver',
      'osc-reactor-grid',
      'midi-strum-smear',
      'spectral-lantern'
  );

INSERT INTO waviate_script_tags (script_id, tag_id)
SELECT waviate_scripts.id, tags.id
FROM (
    VALUES
        ('glacial-padfield', 'pads'),
        ('glacial-padfield', 'ambient'),
        ('glacial-padfield', 'sample-shader'),
        ('granular-fm-bloom', 'fm'),
        ('granular-fm-bloom', 'frequency-shader'),
        ('granular-fm-bloom', 'experimental'),
        ('clean-sub-driver', 'bass'),
        ('clean-sub-driver', 'utility'),
        ('clean-sub-driver', 'sample-shader'),
        ('osc-reactor-grid', 'osc'),
        ('osc-reactor-grid', 'control'),
        ('osc-reactor-grid', 'premium-idea'),
        ('midi-strum-smear', 'midi'),
        ('midi-strum-smear', 'texture'),
        ('midi-strum-smear', 'sample-shader'),
        ('spectral-lantern', 'spectral'),
        ('spectral-lantern', 'frequency-shader'),
        ('spectral-lantern', 'tone')
) AS seed_script_tags(script_slug, tag_name)
JOIN waviate_scripts ON waviate_scripts.slug = seed_script_tags.script_slug
JOIN tags ON tags.name = seed_script_tags.tag_name
ON CONFLICT DO NOTHING;
