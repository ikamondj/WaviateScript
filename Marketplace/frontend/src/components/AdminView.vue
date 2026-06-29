<script setup>
import { ref } from 'vue';
import { clearMarketplaceTables, runMarketplaceSeeds } from '../data/adminApi';
import { loadMarketplaceEntries, usesMarketplaceBackend } from '../data/mockMarketplace';

const busyAction = ref('');
const message = ref('');
const error = ref('');
const scripts = ref([]);

async function clearTables() {
  await runAction('clear', clearMarketplaceTables);
}

async function seedData() {
  await runAction('seed', runMarketplaceSeeds);
}

async function runAction(action, request) {
  busyAction.value = action;
  message.value = '';
  error.value = '';
  scripts.value = [];

  try {
    const result = await request();
    message.value = result.message || 'Admin action completed.';
    scripts.value = result.scripts || [];
    await loadMarketplaceEntries({ pageSize: 4 });
  } catch (err) {
    error.value = err.message || 'Admin action failed.';
  } finally {
    busyAction.value = '';
  }
}
</script>

<template>
  <section v-if="usesMarketplaceBackend()" class="workspace narrow">
    <div class="page-heading">
      <div>
        <p class="eyebrow">Local Admin</p>
        <h1>Reset and reseed the marketplace database.</h1>
      </div>
    </div>

    <div class="admin-actions">
      <button
        type="button"
        class="secondary danger"
        :disabled="!!busyAction"
        @click="clearTables"
      >
        {{ busyAction === 'clear' ? 'Clearing...' : 'Clear All Tables' }}
      </button>
      <button
        type="button"
        class="primary"
        :disabled="!!busyAction"
        @click="seedData"
      >
        {{ busyAction === 'seed' ? 'Running...' : 'Run Seed Scripts' }}
      </button>
    </div>

    <div v-if="message" class="data-note">
      <strong>{{ message }}</strong>
      <ul v-if="scripts.length" class="script-list">
        <li v-for="script in scripts" :key="script.name">{{ script.name }}</li>
      </ul>
    </div>

    <div v-if="error" class="data-note error-note">
      {{ error }}
    </div>
  </section>

  <section v-else class="workspace narrow">
    <p class="eyebrow">Local Admin</p>
    <h1>Admin tools are only available in local mode.</h1>
  </section>
</template>
