<script setup>
import { ref, onMounted, computed, shallowRef } from 'vue';
import { marked } from 'marked';

// Import all markdown files as raw strings
const mdModules = import.meta.glob('../docs/*.md', { query: '?raw', import: 'default' });

const docs = ref([]);
const currentDoc = shallowRef('');
const currentDocName = ref('');

onMounted(async () => {
  const loadedDocs = [];
  for (const path in mdModules) {
    const filename = path.split('/').pop();
    if (filename.startsWith('_') || filename === 'index.html') continue;
    
    const name = filename.replace('.md', '');
    loadedDocs.push({
      name,
      path,
      load: mdModules[path]
    });
  }
  
  // Custom sorting or just alphabetical
  docs.value = loadedDocs.sort((a, b) => a.name.localeCompare(b.name));

  // Try to load home first
  const initial = docs.value.find(d => d.name === 'home') || docs.value[0];
  if (initial) {
    selectDoc(initial);
  }
});

const renderedMarkdown = computed(() => {
  if (!currentDoc.value) return '';
  return marked(currentDoc.value);
});

async function selectDoc(doc) {
  currentDocName.value = doc.name;
  const content = await doc.load();
  currentDoc.value = content;
}
</script>

<template>
  <section class="docs-layout workspace">
    <aside class="docs-sidebar">
      <p class="eyebrow">Documentation</p>
      <nav>
        <button 
          v-for="doc in docs" 
          :key="doc.name"
          @click="selectDoc(doc)"
          :class="{ active: currentDocName === doc.name }"
        >
          {{ doc.name.replace(/_/g, ' ').replace(/-/g, ' ') }}
        </button>
      </nav>
    </aside>
    <main class="docs-content entry-card">
      <div v-html="renderedMarkdown"></div>
    </main>
  </section>
</template>

<style scoped>
.docs-layout {
  display: grid;
  grid-template-columns: 240px minmax(0, 1fr);
  gap: 42px;
  align-items: start;
}

@media (max-width: 860px) {
  .docs-layout {
    grid-template-columns: 1fr;
  }
}

.docs-sidebar {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.docs-sidebar nav {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.docs-sidebar button {
  text-align: left;
  background: transparent;
  padding: 10px 14px;
  border-radius: 8px;
  text-transform: capitalize;
  color: #b7c6d8;
  transition: all 0.2s ease;
  font-size: 0.95rem;
  border: 1px solid transparent;
}

.docs-sidebar button:hover {
  background: rgba(157, 196, 255, 0.08);
  color: #ddecff;
}

.docs-sidebar button.active {
  background: rgba(69, 240, 255, 0.1);
  color: #45f0ff;
  border: 1px solid rgba(69, 240, 255, 0.3);
  font-weight: 600;
}

.docs-content {
  color: #b7c6d8;
  line-height: 1.65;
  font-size: 1.05rem;
}

/* Rendered markdown styling to match the rest of the site */
.docs-content :deep(h1) {
  color: #ffffff;
  font-size: 2.5rem;
  margin-top: 0;
  margin-bottom: 24px;
  line-height: 1.1;
}

.docs-content :deep(h2) {
  color: #edf7ff;
  font-size: 1.8rem;
  margin-top: 40px;
  margin-bottom: 16px;
}

.docs-content :deep(h3) {
  color: #ddecff;
  font-size: 1.4rem;
  margin-top: 32px;
  margin-bottom: 12px;
}

.docs-content :deep(a) {
  color: #45f0ff;
  text-decoration: none;
  border-bottom: 1px solid rgba(69, 240, 255, 0.3);
  transition: border-color 0.2s;
}

.docs-content :deep(a:hover) {
  border-color: #45f0ff;
}

.docs-content :deep(p) {
  margin-bottom: 16px;
}

.docs-content :deep(ul),
.docs-content :deep(ol) {
  margin-bottom: 16px;
  padding-left: 24px;
}

.docs-content :deep(li) {
  margin-bottom: 8px;
}

.docs-content :deep(pre) {
  background: rgba(5, 8, 18, 0.9);
  border: 1px solid rgba(157, 196, 255, 0.18);
  border-radius: 8px;
  padding: 16px;
  overflow-x: auto;
  margin: 24px 0;
  font-family: monospace;
  font-size: 0.9rem;
  color: #ddecff;
}

.docs-content :deep(code) {
  font-family: monospace;
  background: rgba(157, 196, 255, 0.1);
  padding: 3px 6px;
  border-radius: 4px;
  color: #45f0ff;
  font-size: 0.9em;
}

.docs-content :deep(pre code) {
  background: transparent;
  padding: 0;
  color: inherit;
}

.docs-content :deep(blockquote) {
  border-left: 4px solid rgba(69, 240, 255, 0.5);
  background: rgba(69, 240, 255, 0.05);
  margin: 24px 0;
  padding: 16px 20px;
  border-radius: 0 8px 8px 0;
  font-style: italic;
}

.docs-content :deep(table) {
  width: 100%;
  border-collapse: collapse;
  margin: 24px 0;
}

.docs-content :deep(th),
.docs-content :deep(td) {
  border: 1px solid rgba(157, 196, 255, 0.18);
  padding: 12px;
  text-align: left;
}

.docs-content :deep(th) {
  background: rgba(157, 196, 255, 0.08);
  color: #edf7ff;
  font-weight: 600;
}
</style>
