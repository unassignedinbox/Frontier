import { defineConfig } from 'vite';

// The dev server is exposed through a proxy host, so we must not restrict
// allowed hosts / origins and we must listen on all interfaces.
export default defineConfig({
  base: './',
  server: {
    host: '0.0.0.0',
    port: 5173,
    strictPort: true,
    allowedHosts: true,
    hmr: { clientPort: 443, protocol: 'wss' }
  },
  preview: {
    host: '0.0.0.0',
    port: 5173,
    allowedHosts: true
  },
  build: {
    target: 'es2020',
    outDir: 'dist',
    chunkSizeWarningLimit: 1600
  }
});
