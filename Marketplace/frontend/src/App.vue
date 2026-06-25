<script setup>
import { computed, onMounted, ref } from 'vue';
import AppHeader from './components/AppHeader.vue';
import HomeView from './components/HomeView.vue';
import SearchView from './components/SearchView.vue';
import DocsView from './components/DocsView.vue';
import AdminView from './components/AdminView.vue';
import { loadMarketplaceEntries, usesMarketplaceBackend } from './data/mockMarketplace';

const page = ref('home');
const baseNavItems = [
  { key: 'home', label: 'Home' },
  { key: 'search', label: 'Search' },
  { key: 'docs', label: 'Docs' },
];
const localAdminEnabled = computed(() => usesMarketplaceBackend());
const navItems = computed(() => {
  if (!localAdminEnabled.value) {
    return baseNavItems;
  }

  return [...baseNavItems, { key: 'admin', label: 'Admin' }];
});

function navigate(nextPage) {
  if (!navItems.value.some((item) => item.key === nextPage)) {
    return;
  }

  page.value = nextPage;
  window.location.hash = nextPage;
}

onMounted(() => {
  loadMarketplaceEntries();
  const initialPage = window.location.hash.replace('#', '');
  if (navItems.value.some((item) => item.key === initialPage)) {
    page.value = initialPage;
  }
});
</script>

<template>
  <div class="shell">
    <AppHeader :page="page" :nav-items="navItems" @navigate="navigate" />

    <main>
      <HomeView v-if="page === 'home'" @navigate="navigate" />
      <SearchView v-else-if="page === 'search'" />
      <DocsView v-else-if="page === 'docs'" />
      <AdminView v-else-if="page === 'admin' && localAdminEnabled" />
    </main>
  </div>
</template>
