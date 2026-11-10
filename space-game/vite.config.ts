/// <reference types="vitest/config" />
import { defineConfig } from 'vite';

export default defineConfig({
  server: { port: 5173 },
  // Rapier-compat bettet sein WASM als Base64 ein (~4,3 MB) und wird lazy im Flug-Chunk geladen.
  build: { target: 'es2022', chunkSizeWarningLimit: 5000 },
  test: {
    include: ['tests/unit/**/*.test.ts'],
    environment: 'node',
  },
});
