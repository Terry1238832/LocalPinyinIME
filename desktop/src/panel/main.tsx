import React from 'react';
import { createRoot } from 'react-dom/client';
import Panel from './Panel';
import '../tokens.css';
import './panel.css';

// The main process follows the settings' theme (nativeTheme), which is what
// prefers-color-scheme reports here.
const scheme = window.matchMedia('(prefers-color-scheme: dark)');
const applyTheme = () => { document.documentElement.dataset.theme = scheme.matches ? 'dark' : 'light'; };
applyTheme();
scheme.addEventListener('change', applyTheme);
createRoot(document.getElementById('root')!).render(<Panel />);
