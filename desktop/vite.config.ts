import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Two pages: the settings window and the tool panel the input method opens.
export default defineConfig({
  base: './',
  plugins: [react()],
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    sourcemap: false,
    rollupOptions: { input: { index: 'index.html', panel: 'panel.html' } },
  },
});
