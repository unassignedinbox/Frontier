import { defineConfig } from 'vite';

export default defineConfig({
  base: './',
  server: { host: '0.0.0.0', allowedHosts: true, strictPort: true, port: 5173 },
  preview: { host: '0.0.0.0', allowedHosts: true },
  worker: { format: 'es' },
  build: { target: 'es2022' },
});
