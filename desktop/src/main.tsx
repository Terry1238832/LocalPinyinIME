import React from 'react';
import { createRoot } from 'react-dom/client';
import App from './App';
import './styles.css';
import './candidate-styles.css';
import './settings.css';

const storedTheme = window.localPinyin?.initialState?.settings.theme ?? 'system';
document.documentElement.dataset.theme = storedTheme === 'system'
  ? (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light')
  : storedTheme;
createRoot(document.getElementById('root')!).render(<App />);
