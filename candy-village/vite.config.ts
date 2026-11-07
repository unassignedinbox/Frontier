import { defineConfig } from 'vite';

// Bound to 0.0.0.0 so the sandbox preview proxy can reach the dev server, and
// allow any host header because the preview is served from a generated domain.
export default defineConfig({
  server: {
    host: '0.0.0.0',
    port: 5173,
    strictPort: true,
    allowedHosts: true,
  },
  preview: {
    host: '0.0.0.0',
    port: 4173,
    strictPort: true,
    allowedHosts: true,
  },
  build: {
    target: 'es2022',
    chunkSizeWarningLimit: 1500,
  },
});
