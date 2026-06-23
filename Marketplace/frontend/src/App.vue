<script setup>
import { computed, onMounted, ref } from 'vue';
import { mockEntries, sortOptions } from './data/mockMarketplace';

const page = ref('home');
const query = ref('');
const author = ref('');
const tag = ref('');
const sortBy = ref('rating');
const pageNumber = ref(1);
const pageSize = 4;

const navItems = [
  { key: 'home', label: 'Home' },
  { key: 'search', label: 'Search' },
  { key: 'docs', label: 'Docs' },
  { key: 'upload', label: 'Upload' },
];

const allTags = computed(() => {
  const tags = new Set(mockEntries.flatMap((entry) => entry.tags));
  return [...tags].sort();
});

const filteredEntries = computed(() => {
  const loweredQuery = query.value.trim().toLowerCase();
  const loweredAuthor = author.value.trim().toLowerCase();

  return mockEntries
    .filter((entry) => {
      const matchesQuery =
        !loweredQuery ||
        entry.title.toLowerCase().includes(loweredQuery) ||
        entry.summary.toLowerCase().includes(loweredQuery) ||
        entry.tags.some((entryTag) => entryTag.toLowerCase().includes(loweredQuery));
      const matchesAuthor = !loweredAuthor || entry.author.toLowerCase().includes(loweredAuthor);
      const matchesTag = !tag.value || entry.tags.includes(tag.value);

      return matchesQuery && matchesAuthor && matchesTag;
    })
    .sort((left, right) => {
      if (sortBy.value === 'downloads') {
        return right.downloads - left.downloads;
      }

      if (sortBy.value === 'updated') {
        return new Date(right.updated) - new Date(left.updated);
      }

      return right.rating - left.rating;
    });
});

const totalPages = computed(() => Math.max(1, Math.ceil(filteredEntries.value.length / pageSize)));

const pagedEntries = computed(() => {
  const safePage = Math.min(pageNumber.value, totalPages.value);
  const offset = (safePage - 1) * pageSize;
  return filteredEntries.value.slice(offset, offset + pageSize);
});

function navigate(nextPage) {
  page.value = nextPage;
  window.location.hash = nextPage;
}

function resetSearch() {
  query.value = '';
  author.value = '';
  tag.value = '';
  sortBy.value = 'rating';
  pageNumber.value = 1;
}

function installEntry(entry) {
  // TODO: Replace this with the backend-provided desktop handoff URL.
  window.alert(`Desktop install handoff is not wired yet for "${entry.title}".`);
}

onMounted(() => {
  const initialPage = window.location.hash.replace('#', '');
  if (navItems.some((item) => item.key === initialPage)) {
    page.value = initialPage;
  }
});
</script>

<template>
  <div class="shell">
    <header class="topbar">
      <button class="brand" type="button" @click="navigate('home')">
        <span class="brand-mark">WS</span>
        <span>
          <strong>WaviateScript</strong>
          <small>Marketplace</small>
        </span>
      </button>

      <nav class="nav" aria-label="Primary">
        <button
          v-for="item in navItems"
          :key="item.key"
          type="button"
          :class="{ active: page === item.key }"
          @click="navigate(item.key)"
        >
          {{ item.label }}
        </button>
      </nav>
    </header>

    <main>
      <section v-if="page === 'home'" class="hero">
        <div class="hero-copy">
          <p class="eyebrow">Shader-like audio code, ready to share</p>
          <h1>Discover Waviate scripts for sample and frequency workflows.</h1>
          <p class="lede">
            Browse community-built kernels, study patterns, and prepare scripts for the
            future sandboxed sharing flow.
          </p>
          <div class="hero-actions">
            <button type="button" class="primary" @click="navigate('search')">Search Scripts</button>
            <button type="button" class="secondary" @click="navigate('docs')">Read Docs</button>
          </div>
        </div>

        <div class="signal-panel" aria-label="Marketplace highlights">
          <div class="signal-ring"></div>
          <div class="metric">
            <span>Entries</span>
            <strong>{{ mockEntries.length }}</strong>
          </div>
          <div class="metric">
            <span>Top Rating</span>
            <strong>4.9</strong>
          </div>
          <div class="metric">
            <span>Formats</span>
            <strong>Open</strong>
          </div>
        </div>
      </section>

      <section v-else-if="page === 'search'" class="workspace">
        <div class="page-heading">
          <div>
            <p class="eyebrow">Marketplace Search</p>
            <h1>Find scripts by name, author, tag, or rating.</h1>
          </div>
          <button type="button" class="secondary" @click="resetSearch">Reset</button>
        </div>

        <div class="filters">
          <label>
            Search
            <input v-model="query" type="search" placeholder="pads, spectral, bass..." @input="pageNumber = 1" />
          </label>
          <label>
            User
            <input v-model="author" type="search" placeholder="creator handle" @input="pageNumber = 1" />
          </label>
          <label>
            Tag
            <select v-model="tag" @change="pageNumber = 1">
              <option value="">Any tag</option>
              <option v-for="entryTag in allTags" :key="entryTag" :value="entryTag">
                {{ entryTag }}
              </option>
            </select>
          </label>
          <label>
            Sort
            <select v-model="sortBy">
              <option v-for="option in sortOptions" :key="option.value" :value="option.value">
                {{ option.label }}
              </option>
            </select>
          </label>
        </div>

        <div class="results-meta">
          <span>{{ filteredEntries.length }} results</span>
          <span>Page {{ pageNumber }} of {{ totalPages }}</span>
        </div>

        <div class="results">
          <article v-for="entry in pagedEntries" :key="entry.id" class="entry-card">
            <div>
              <p class="entry-author">@{{ entry.author }}</p>
              <h2>{{ entry.title }}</h2>
              <p>{{ entry.summary }}</p>
            </div>
            <div class="tag-list">
              <span v-for="entryTag in entry.tags" :key="entryTag">{{ entryTag }}</span>
            </div>
            <div class="entry-footer">
              <span>{{ entry.rating.toFixed(1) }} rating</span>
              <span>{{ entry.downloads.toLocaleString() }} downloads</span>
              <span>Updated {{ entry.updated }}</span>
              <button type="button" @click="installEntry(entry)">Open in App</button>
            </div>
          </article>

          <div v-if="pagedEntries.length === 0" class="empty-state">
            No scripts matched those filters yet.
          </div>
        </div>

        <div class="pagination" aria-label="Pagination">
          <button type="button" :disabled="pageNumber === 1" @click="pageNumber -= 1">Previous</button>
          <button type="button" :disabled="pageNumber === totalPages" @click="pageNumber += 1">Next</button>
        </div>
      </section>

      <section v-else-if="page === 'docs'" class="workspace narrow">
        <p class="eyebrow">Docs</p>
        <h1>Build, validate, and share Waviate scripts.</h1>
        <div class="doc-grid">
          <article>
            <h2>Sample Shaders</h2>
            <p>TODO: Link to language docs, helper functions, and audio-thread rules.</p>
          </article>
          <article>
            <h2>Frequency Shaders</h2>
            <p>TODO: Link to spectral context, bin processing examples, and safety guidance.</p>
          </article>
          <article>
            <h2>Publishing</h2>
            <p>TODO: Document validation, sandbox guarantees, payload format, and review flow.</p>
          </article>
        </div>
      </section>

      <section v-else class="workspace narrow">
        <p class="eyebrow">Upload</p>
        <h1>Script publishing is coming later.</h1>
        <p class="lede">
          This scaffold reserves space for auth, validation, and payload packaging without choosing
          the final raw, compressed, or encrypted storage model yet.
        </p>
        <button type="button" class="primary" @click="navigate('search')">Back to Search</button>
      </section>
    </main>
  </div>
</template>
