<script setup>
import { computed, defineEmits } from 'vue';
import { marketplacePage, mockEntries } from '../data/mockMarketplace';

const emit = defineEmits(['navigate']);
const entryCount = computed(() => marketplacePage.value.total || mockEntries.value.length);

function navigate(nextPage) {
  emit('navigate', nextPage);
}
</script>

<template>
  <section class="hero">
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
        <strong>{{ entryCount }}</strong>
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
</template>
