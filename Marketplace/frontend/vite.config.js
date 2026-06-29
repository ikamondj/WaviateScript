import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';

export default defineConfig(({ mode }) => {
  const localModes = new Set(['loc', 'local']);
  const config = {
    plugins: [vue()],
  };

  if (localModes.has(mode)) {
    // Configure proxy to connect to the default ports for the golang server and DB (if needed)
    config.server = {
      proxy: {
        '/api': {
          target: 'http://localhost:8080',
          changeOrigin: true,
        },
        // In a real scenario, the DB port 5432 is used by the backend,
        // but wiring it here per requirements in case of some direct db introspection proxy
        '/db': {
          target: 'http://localhost:5432',
          changeOrigin: true,
        },
      }
    };
  }

  return config;
});
