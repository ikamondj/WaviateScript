<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue';
import {
  loadMarketplaceEntries,
  marketplaceError,
  marketplaceLoading,
  marketplacePage,
  marketplaceTags,
  mockEntries,
  sortOptions,
} from '../data/mockMarketplace';
import { highlightCpp } from '../data/cppHighlight';

const query = ref('');
const author = ref('');
const tagSelections = ref({});
const tagDropdownOpen = ref(false);
const tagDropdown = ref(null);
const sortBy = ref('rating');
const pageNumber = ref(1);
const pageSize = 4;
const appliedSearch = ref(emptySearchCriteria());
const sourcePreviewEntry = ref(null);

const allTags = computed(() => marketplaceTags.value);

const includedTags = computed(() =>
  allTags.value.filter((entryTag) => tagSelections.value[entryTag] === 'include'),
);

const excludedTags = computed(() =>
  allTags.value.filter((entryTag) => tagSelections.value[entryTag] === 'exclude'),
);

const tagFilterLabel = computed(() => {
  const includedCount = includedTags.value.length;
  const excludedCount = excludedTags.value.length;

  if (!includedCount && !excludedCount) {
    return 'Any tag';
  }

  const parts = [];
  if (includedCount) {
    parts.push(`${includedCount} included`);
  }
  if (excludedCount) {
    parts.push(`${excludedCount} excluded`);
  }

  return parts.join(', ');
});

const totalPages = computed(() => Math.max(1, marketplacePage.value.totalPages));
const resultCount = computed(() => marketplacePage.value.total);
const pagedEntries = computed(() => mockEntries.value);
const pendingChanges = computed(() => !sameCriteria(readDraftCriteria(), appliedSearch.value));
const highlightedPreviewSource = computed(() => highlightCpp(sourcePreviewEntry.value?.content || ''));

function resetSearch() {
  query.value = '';
  author.value = '';
  tagSelections.value = {};
  tagDropdownOpen.value = false;
  sortBy.value = 'rating';
  appliedSearch.value = emptySearchCriteria();
  pageNumber.value = 1;
  refreshSearch();
}

function toggleTagSelection(entryTag, nextSelection) {
  const selections = { ...tagSelections.value };

  if (selections[entryTag] === nextSelection) {
    delete selections[entryTag];
  } else {
    selections[entryTag] = nextSelection;
  }

  tagSelections.value = selections;
}

async function refreshSearch() {
  const result = await loadMarketplaceEntries({
    ...appliedSearch.value,
    page: pageNumber.value,
    pageSize,
  });

  if (result?.page?.page && result.page.page !== pageNumber.value) {
    pageNumber.value = result.page.page;
  }
}

function applySearch() {
  appliedSearch.value = readDraftCriteria();
  tagDropdownOpen.value = false;
  pageNumber.value = 1;
  refreshSearch();
}

function goToPage(nextPage) {
  pageNumber.value = nextPage;
  refreshSearch();
}

function closeTagDropdownOnOutsideClick(event) {
  if (!tagDropdownOpen.value || tagDropdown.value?.contains(event.target)) {
    return;
  }

  tagDropdownOpen.value = false;
}

function installEntry(entry) {
  window.alert(`Desktop install handoff is not wired yet for "${entry.title}".`);
}

function previewSource(entry) {
  sourcePreviewEntry.value = entry;
}

function closeSourcePreview() {
  sourcePreviewEntry.value = null;
}

function emptySearchCriteria() {
  return {
    query: '',
    user: '',
    includedTags: [],
    excludedTags: [],
    sort: 'rating',
  };
}

function readDraftCriteria() {
  return {
    query: query.value.trim(),
    user: author.value.trim(),
    includedTags: [...includedTags.value],
    excludedTags: [...excludedTags.value],
    sort: sortBy.value,
  };
}

function sameCriteria(left, right) {
  return (
    left.query === right.query &&
    left.user === right.user &&
    left.sort === right.sort &&
    sameArray(left.includedTags, right.includedTags) &&
    sameArray(left.excludedTags, right.excludedTags)
  );
}

function sameArray(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

onMounted(() => {
  document.addEventListener('click', closeTagDropdownOnOutsideClick);
  document.addEventListener('keydown', closePreviewOnEscape);
  refreshSearch();
});

onBeforeUnmount(() => {
  document.removeEventListener('click', closeTagDropdownOnOutsideClick);
  document.removeEventListener('keydown', closePreviewOnEscape);
});

function closePreviewOnEscape(event) {
  if (event.key === 'Escape') {
    closeSourcePreview();
  }
}
</script>

<template>
  <section class="workspace">
    <div class="page-heading">
      <div>
        <p class="eyebrow">Marketplace Search</p>
        <h1>Find scripts by name, author, tag, or rating.</h1>
      </div>
      <div class="page-actions">
        <button type="button" class="secondary" @click="resetSearch">Reset</button>
        <button type="button" class="primary" :disabled="marketplaceLoading || !pendingChanges" @click="applySearch">
          Apply
        </button>
      </div>
    </div>

    <div class="filters">
      <label>
        Search
        <input v-model="query" type="search" placeholder="pads, spectral, bass..." />
      </label>
      <label>
        User
        <input v-model="author" type="search" placeholder="creator handle" />
      </label>
      <div class="field">
        <span class="field-label">Tag</span>
        <div
          ref="tagDropdown"
          class="tag-dropdown"
          :class="{ open: tagDropdownOpen }"
          @keydown.escape.stop="tagDropdownOpen = false"
        >
          <button
            type="button"
            class="tag-dropdown-toggle"
            aria-haspopup="menu"
            :aria-expanded="tagDropdownOpen.toString()"
            @click="tagDropdownOpen = !tagDropdownOpen"
          >
            <span>{{ tagFilterLabel }}</span>
          </button>

          <div v-if="tagDropdownOpen" class="tag-dropdown-menu" role="menu" aria-label="Tag filters">
            <div v-for="entryTag in allTags" :key="entryTag" class="tag-option">
              <span class="tag-option-name">{{ entryTag }}</span>
              <div class="tag-option-actions">
                <button
                  type="button"
                  class="tag-mode include"
                  :class="{ active: tagSelections[entryTag] === 'include' }"
                  :aria-pressed="tagSelections[entryTag] === 'include'"
                  :aria-label="`${entryTag}: include`"
                  @click="toggleTagSelection(entryTag, 'include')"
                >
                  &#10003;
                </button>
                <button
                  type="button"
                  class="tag-mode exclude"
                  :class="{ active: tagSelections[entryTag] === 'exclude' }"
                  :aria-pressed="tagSelections[entryTag] === 'exclude'"
                  :aria-label="`${entryTag}: exclude`"
                  @click="toggleTagSelection(entryTag, 'exclude')"
                >
                  X
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>
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
      <span>{{ resultCount }} results</span>
      <span>Page {{ marketplacePage.page }} of {{ totalPages }}</span>
      <span v-if="pendingChanges">Filters changed, apply to update results</span>
    </div>

    <div v-if="marketplaceError" class="data-note">
      {{ marketplaceError }}
    </div>

    <div class="results">
      <div v-if="marketplaceLoading && pagedEntries.length === 0" class="empty-state">
        Loading marketplace scripts...
      </div>

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
          <div class="entry-stats">
            <span>{{ entry.rating.toFixed(1) }} rating</span>
            <span>{{ entry.downloads.toLocaleString() }} downloads</span>
            <span>Updated {{ entry.updated }}</span>
          </div>
          <div class="entry-actions">
            <button type="button" class="source-preview-button" @click="previewSource(entry)">
              Preview Source
            </button>
            <button type="button" @click="installEntry(entry)">Open in App</button>
          </div>
        </div>
      </article>

      <div v-if="!marketplaceLoading && pagedEntries.length === 0" class="empty-state">
        No scripts matched those filters yet.
      </div>
    </div>

    <div class="pagination" aria-label="Pagination">
      <button
        type="button"
        :disabled="marketplaceLoading || marketplacePage.page === 1"
        @click="goToPage(marketplacePage.page - 1)"
      >
        Previous
      </button>
      <button
        type="button"
        :disabled="marketplaceLoading || marketplacePage.page === totalPages"
        @click="goToPage(marketplacePage.page + 1)"
      >
        Next
      </button>
    </div>

    <div
      v-if="sourcePreviewEntry"
      class="source-preview-overlay"
      role="dialog"
      aria-modal="true"
      :aria-label="`${sourcePreviewEntry.title} source preview`"
      @click.self="closeSourcePreview"
    >
      <section class="source-preview-panel">
        <div class="source-preview-header">
          <div>
            <p class="eyebrow">Source Preview</p>
            <h2>{{ sourcePreviewEntry.title }}</h2>
            <p class="entry-author">@{{ sourcePreviewEntry.author }}</p>
          </div>
          <button type="button" class="secondary" @click="closeSourcePreview">Close</button>
        </div>

        <pre class="source-code" tabindex="0"><code v-html="highlightedPreviewSource"></code></pre>
      </section>
    </div>
  </section>
</template>
