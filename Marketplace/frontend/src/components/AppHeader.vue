<script setup>
import { defineEmits, defineProps } from 'vue';

defineProps({
  page: {
    type: String,
    required: true,
  },
  navItems: {
    type: Array,
    required: true,
  },
});

const emit = defineEmits(['navigate']);

function navigate(nextPage) {
  emit('navigate', nextPage);
}
</script>

<template>
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
</template>
