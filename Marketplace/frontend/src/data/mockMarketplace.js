import { ref } from 'vue';

const fallbackEntries = [
  {
    id: 'glacial-padfield',
    title: 'Glacial Padfield',
    author: 'phasefold',
    summary: 'A slow evolving sample shader for crystalline pads and wide stereo drift.',
    tags: ['pads', 'ambient', 'sample-shader'],
    rating: 4.9,
    downloads: 18420,
    updated: '2026-06-10',
    format: 'unknown',
  },
  {
    id: 'granular-fm-bloom',
    title: 'Granular FM Bloom',
    author: 'modmatrix',
    summary: 'Frequency shader sketch for glassy FM clusters with tight macro control.',
    tags: ['fm', 'frequency-shader', 'experimental'],
    rating: 4.7,
    downloads: 9310,
    updated: '2026-05-21',
    format: 'unknown',
  },
  {
    id: 'clean-sub-driver',
    title: 'Clean Sub Driver',
    author: 'nullcarrier',
    summary: 'A focused low-end utility script for gentle saturation and phase-safe weight.',
    tags: ['bass', 'utility', 'sample-shader'],
    rating: 4.8,
    downloads: 12102,
    updated: '2026-04-28',
    format: 'unknown',
  },
  {
    id: 'osc-reactor-grid',
    title: 'OSC Reactor Grid',
    author: 'signalweld',
    summary: 'Prototype control-reactive script intended for premium OSC workflows later.',
    tags: ['osc', 'control', 'premium-idea'],
    rating: 4.4,
    downloads: 3702,
    updated: '2026-03-19',
    format: 'unknown',
  },
  {
    id: 'midi-strum-smear',
    title: 'MIDI Strum Smear',
    author: 'phasefold',
    summary: 'MIDI-aware sample shader for soft timing spread and chord-motion textures.',
    tags: ['midi', 'texture', 'sample-shader'],
    rating: 4.6,
    downloads: 7118,
    updated: '2026-02-07',
    format: 'unknown',
  },
  {
    id: 'spectral-lantern',
    title: 'Spectral Lantern',
    author: 'noctshape',
    summary: 'Frequency-domain tone shaper with glowing resonant peaks and animated tilt.',
    tags: ['spectral', 'frequency-shader', 'tone'],
    rating: 4.5,
    downloads: 6024,
    updated: '2026-01-14',
    format: 'unknown',
  },
];

const fallbackSource = `float SampleProcess(const WaviateSample& wav)
{
    return wav.getIncomingSample();
}
`;

const backendModes = new Set(['loc', 'local']);
const defaultPageSize = 20;
let activeLoad = 0;
let activeTagLoad = 0;

export const mockEntries = ref([]);
export const marketplacePage = ref(createPageInfo(1, defaultPageSize, 0));
export const marketplaceLoading = ref(false);
export const marketplaceError = ref('');
export const marketplaceDataSource = ref('fallback');
export const marketplaceTags = ref(usesMarketplaceBackend() ? [] : collectTags(fallbackEntries));

export const sortOptions = [
  { value: 'rating', label: 'Top Rated' },
  { value: 'downloads', label: 'Most Downloaded' },
  { value: 'updated', label: 'Recently Updated' },
];

export const matchModeOptions = [
  { value: 'contains', label: 'Contains' },
  { value: 'starts_with', label: 'Starts with' },
  { value: 'exact', label: 'Exact match' },
];

export function usesMarketplaceBackend() {
  return backendModes.has(import.meta.env.MODE);
}

export async function loadMarketplaceTags() {
  const loadId = ++activeTagLoad;

  if (!usesMarketplaceBackend()) {
    marketplaceTags.value = collectTags(fallbackEntries);
    return marketplaceTags.value;
  }

  try {
    const response = await fetch('/api/tags', {
      headers: {
        Accept: 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error(`Marketplace tags failed with HTTP ${response.status}`);
    }

    const data = await response.json();
    const tags = normalizeTags(Array.isArray(data.tags) ? data.tags : []);
    if (loadId === activeTagLoad) {
      marketplaceTags.value = tags;
    }

    return tags;
  } catch (err) {
    if (loadId === activeTagLoad) {
      console.warn('Failed to fetch marketplace tags from backend.', err);
      if (marketplaceTags.value.length === 0) {
        marketplaceTags.value = collectTags(fallbackEntries);
      }
    }

    return marketplaceTags.value;
  }
}

export async function downloadEntrySource(entry) {
  if (!usesMarketplaceBackend()) {
    return entry.content || fallbackSource;
  }

  const response = await fetch(`/api/uploads/${encodeURIComponent(entry.id)}/source`, {
    headers: {
      Accept: 'application/json',
    },
  });

  if (!response.ok) {
    throw new Error(`Source download failed with HTTP ${response.status}`);
  }

  const data = await response.json();
  return data.source || '';
}

export async function loadMarketplaceEntries(options = {}) {
  const loadId = ++activeLoad;
  const searchOptions = normalizeOptions(options);

  marketplaceLoading.value = true;
  marketplaceError.value = '';

  try {
    if (usesMarketplaceBackend()) {
      const result = await loadBackendEntries(searchOptions);
      if (loadId !== activeLoad) {
        return null;
      }

      applyResult(result, 'backend', loadId);
      return result;
    }
  } catch (err) {
    if (loadId === activeLoad) {
      console.warn('Failed to fetch marketplace entries from backend, falling back to bundled data.', err);
      marketplaceError.value = 'Could not reach the marketplace backend. Showing bundled sample data.';
    }
  } finally {
    if (loadId === activeLoad) {
      marketplaceLoading.value = false;
    }
  }

  const fallbackResult = searchFallbackEntries(searchOptions);
  if (loadId !== activeLoad) {
    return null;
  }

  applyResult(fallbackResult, 'fallback', loadId);
  return fallbackResult;
}

async function loadBackendEntries(options) {
  const params = new URLSearchParams();
  params.set('page', String(options.page));
  params.set('pageSize', String(options.pageSize));
  params.set('sort', options.sort);
  params.set('qMode', options.queryMode);
  params.set('userMode', options.userMode);

  if (options.query) {
    params.set('q', options.query);
  }

  if (options.user) {
    params.set('user', options.user);
  }

  options.includedTags.forEach((tag) => params.append('includeTag', tag));
  options.excludedTags.forEach((tag) => params.append('excludeTag', tag));

  const response = await fetch(`/api/search?${params.toString()}`, {
    headers: {
      Accept: 'application/json',
    },
  });

  if (!response.ok) {
    throw new Error(`Marketplace search failed with HTTP ${response.status}`);
  }

  const data = await response.json();
  const entries = (data.entries || []).map(mapBackendEntry);

  return {
    entries,
    page: normalizePage(data.page, options, entries.length),
  };
}

function searchFallbackEntries(options) {
  const loweredQuery = options.query.toLowerCase();
  const loweredUser = options.user.toLowerCase();
  const filteredEntries = fallbackEntries.filter((entry) => {
    const matchesQuery = !loweredQuery || matchesText(entry.title, loweredQuery, options.queryMode);
    const matchesUser = !loweredUser || matchesText(entry.author, loweredUser, options.userMode);

    return (
      matchesQuery &&
      matchesUser &&
      filterEntriesByTags([entry], options.includedTags, options.excludedTags).length === 1
    );
  });

  return paginateEntries(
    sortEntries(filteredEntries, options.sort).map((entry) => ({
      ...entry,
      content: entry.content || fallbackSource,
    })),
    options,
  );
}

function applyResult(result, source, loadId) {
  if (loadId !== activeLoad) {
    return;
  }

  mockEntries.value = result.entries;
  marketplacePage.value = result.page;
  marketplaceDataSource.value = source;
  if (source === 'fallback') {
    marketplaceTags.value = collectTags(fallbackEntries);
  } else if (marketplaceTags.value.length === 0) {
    marketplaceTags.value = mergeTags(result.entries);
  }
}

function normalizeOptions(options) {
  return {
    query: String(options.query || '').trim(),
    queryMode: normalizeMatchMode(options.queryMode),
    user: String(options.user || '').trim(),
    userMode: normalizeMatchMode(options.userMode),
    includedTags: normalizeTags(options.includedTags || options.tags || (options.tag ? [options.tag] : [])),
    excludedTags: normalizeTags(options.excludedTags || []),
    sort: options.sort || 'rating',
    page: normalizePositiveInt(options.page, 1),
    pageSize: normalizePositiveInt(options.pageSize, defaultPageSize),
  };
}

function normalizeTags(tags) {
  return tags.map((tag) => String(tag).trim()).filter(Boolean);
}

function normalizeMatchMode(mode) {
  if (mode === 'exact') {
    return 'exact';
  }
  if (mode === 'starts_with' || mode === 'starts' || mode === 'begins') {
    return 'starts_with';
  }

  return 'contains';
}

function normalizePositiveInt(value, fallback) {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed < 1) {
    return fallback;
  }

  return parsed;
}

function mapBackendEntry(entry) {
  return {
    id: entry.id,
    title: entry.name || 'Untitled Script',
    author: entry.authorName || entry.authorId || 'unknown',
    summary: entry.description || '',
    tags: Array.isArray(entry.tags) ? entry.tags : [],
    rating: Number(entry.ratingScore || 0),
    ratingCount: Number(entry.ratingCount || 0),
    downloads: Number(entry.downloadCount || 0),
    updated: formatDate(entry.updatedAt || entry.createdAt),
    format: 'waviate',
    requiresPremium: Boolean(entry.requiresPremium),
    content: entry.content || '',
  };
}

function formatDate(value) {
  if (!value) {
    return 'unknown';
  }

  const parsed = new Date(value);
  if (!Number.isNaN(parsed.getTime())) {
    return parsed.toISOString().slice(0, 10);
  }

  return String(value).split('T')[0] || 'unknown';
}

function filterEntriesByTags(entries, includedTags, excludedTags) {
  return entries.filter((entry) => {
    const entryTags = new Set(entry.tags);
    return (
      includedTags.every((entryTag) => entryTags.has(entryTag)) &&
      excludedTags.every((entryTag) => !entryTags.has(entryTag))
    );
  });
}

function matchesText(value, loweredNeedle, mode) {
  const loweredValue = String(value || '').toLowerCase();
  if (mode === 'exact') {
    return loweredValue === loweredNeedle;
  }
  if (mode === 'starts_with') {
    return loweredValue.startsWith(loweredNeedle);
  }

  return loweredValue.includes(loweredNeedle);
}

function sortEntries(entries, sortBy) {
  return [...entries].sort((left, right) => {
    if (sortBy === 'downloads') {
      return right.downloads - left.downloads;
    }

    if (sortBy === 'updated') {
      return new Date(right.updated) - new Date(left.updated);
    }

    return right.rating - left.rating;
  });
}

function paginateEntries(entries, options) {
  const pageSize = normalizePositiveInt(options.pageSize, defaultPageSize);
  const total = entries.length;
  const totalPages = Math.max(1, Math.ceil(total / pageSize));
  const page = Math.min(normalizePositiveInt(options.page, 1), totalPages);
  const offset = (page - 1) * pageSize;

  return {
    entries: entries.slice(offset, offset + pageSize),
    page: {
      page,
      pageSize,
      total,
      totalPages,
    },
  };
}

function normalizePage(page, options, entryCount) {
  return {
    page: normalizePositiveInt(page?.page, options.page),
    pageSize: normalizePositiveInt(page?.pageSize, options.pageSize),
    total: normalizePositiveInt(page?.total, entryCount),
    totalPages: normalizePositiveInt(page?.totalPages, 1),
  };
}

function createPageInfo(page, pageSize, total) {
  return {
    page,
    pageSize,
    total,
    totalPages: Math.max(1, Math.ceil(total / pageSize)),
  };
}

function collectTags(entries) {
  return [...new Set(entries.flatMap((entry) => entry.tags))].sort();
}

function mergeTags(entries) {
  return [...new Set([...marketplaceTags.value, ...entries.flatMap((entry) => entry.tags)])].sort();
}
