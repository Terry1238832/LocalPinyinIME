import type { CSSProperties, ReactNode } from 'react';
import brandMark from '../../assets/branding/icon/LocalPinyinIME.svg';
import brandSmall from '../../assets/branding/icon/small/LocalPinyinIME-40.png';

// One family: 24-unit grid, 1.6 stroke, round caps and joins, no fills except
// small dots. Glyphs are drawn inside a 3–21 safe area so they share an
// optical size in the sidebar and in rows.
export type IconName = 'appearance' | 'input' | 'dictionary' | 'learning' | 'about' | 'sun' | 'moon' | 'monitor'
  | 'chevron' | 'search' | 'plus' | 'edit' | 'trash' | 'folder' | 'check' | 'close' | 'minimize' | 'maximize'
  | 'shield' | 'arrow' | 'keyboard' | 'spark' | 'reset' | 'warning' | 'text' | 'context' | 'switch' | 'database'
  | 'info' | 'command' | 'layers' | 'user' | 'gear' | 'enter' | 'history' | 'cursor'
  | 'tools' | 'clipboard' | 'omega' | 'kaomoji' | 'smile' | 'sticker' | 'star' | 'pin' | 'image' | 'calendar' | 'upload'
  | 'download' | 'panel' | 'lock' | 'english';

const paths: Record<IconName, ReactNode> = {
  appearance: <><circle cx="12" cy="12" r="8.25" /><path d="M12 3.75a8.25 8.25 0 0 1 0 16.5Z" fill="currentColor" stroke="none" /></>,
  input: <><rect x="3" y="5.5" width="18" height="13" rx="2.75" /><path d="M7 9.5h.01M10.33 9.5h.01M13.67 9.5h.01M17 9.5h.01M7 12.5h.01M17 12.5h.01M9 15h6" /></>,
  dictionary: <><path d="M4.75 5.75A2.75 2.75 0 0 1 7.5 3h11.75v14.5H7.5a2.75 2.75 0 0 0-2.75 2.75Z" /><path d="M4.75 20.25A2.75 2.75 0 0 0 7.5 21h11.75v-3.5" /><path d="M9 7.5h6" /></>,
  learning: <><path d="M3.75 12a8.25 8.25 0 1 0 2.5-5.9" /><path d="M3.75 4.5v3.75H7.5" /><path d="M12 8v4.25l2.75 1.75" /></>,
  about: <><circle cx="12" cy="12" r="8.25" /><path d="M12 11v5" /><circle cx="12" cy="7.9" r=".95" fill="currentColor" stroke="none" /></>,
  sun: <><circle cx="12" cy="12" r="3.75" /><path d="M12 3v1.75M12 19.25V21M3 12h1.75M19.25 12H21M5.64 5.64l1.24 1.24M17.12 17.12l1.24 1.24M5.64 18.36l1.24-1.24M17.12 6.88l1.24-1.24" /></>,
  moon: <path d="M19.25 14.25A7.75 7.75 0 0 1 9.75 4.75a7.75 7.75 0 1 0 9.5 9.5Z" />,
  monitor: <><rect x="3" y="4" width="18" height="12.5" rx="2.25" /><path d="M9 20h6M12 16.5V20" /></>,
  chevron: <path d="m9.5 6 6 6-6 6" />,
  search: <><circle cx="10.75" cy="10.75" r="6.25" /><path d="m15.5 15.5 4.75 4.75" /></>,
  plus: <path d="M12 5v14M5 12h14" />,
  edit: <><path d="M15.25 4.75a2.12 2.12 0 0 1 3 3L8 18l-4 1 1-4Z" /><path d="m13.5 6.5 3 3" /></>,
  trash: <><path d="M4.5 6.75h15M9.75 6.75V4.5h4.5v2.25M6.5 6.75l.9 12.5a1.5 1.5 0 0 0 1.5 1.25h6.2a1.5 1.5 0 0 0 1.5-1.25l.9-12.5M10 10.75v5.5M14 10.75v5.5" /></>,
  folder: <path d="M3.25 7.25A2.25 2.25 0 0 1 5.5 5h3.88a1.5 1.5 0 0 1 1.06.44L12 7h6.5a2.25 2.25 0 0 1 2.25 2.25v8.5A2.25 2.25 0 0 1 18.5 20h-13a2.25 2.25 0 0 1-2.25-2.25Z" />,
  check: <path d="m5 12.5 4.5 4.5L19 7.5" />,
  close: <path d="m6.5 6.5 11 11M6.5 17.5l11-11" />,
  minimize: <path d="M6 12h12" />,
  maximize: <rect x="6" y="6" width="12" height="12" rx="1.75" />,
  shield: <><path d="M12 3.25 19 6v5.5c0 4.1-2.9 7.4-7 9.25-4.1-1.85-7-5.15-7-9.25V6Z" /><path d="m9 12 2.1 2.1L15.25 10" /></>,
  arrow: <path d="M5 12h14m-5-5 5 5-5 5" />,
  keyboard: <><rect x="3" y="5.5" width="18" height="13" rx="2.75" /><path d="M7 9.5h.01M10.33 9.5h.01M13.67 9.5h.01M17 9.5h.01M7 12.5h.01M10.33 12.5h.01M13.67 12.5h.01M17 12.5h.01M9 15.25h6" /></>,
  spark: <><path d="M12 3.5c.5 3.9 2.6 6 6.5 6.5-3.9.5-6 2.6-6.5 6.5-.5-3.9-2.6-6-6.5-6.5 3.9-.5 6-2.6 6.5-6.5Z" /><path d="M18.5 15.5c.2 1.4.9 2.1 2.25 2.25-1.35.2-2.05.9-2.25 2.25-.2-1.35-.9-2.05-2.25-2.25 1.35-.15 2.05-.85 2.25-2.25Z" /></>,
  reset: <><path d="M4.5 12a7.5 7.5 0 1 0 2.2-5.3L4.5 9" /><path d="M4.5 4.5V9H9" /></>,
  warning: <><path d="M10.3 4.3 2.8 17.5A2 2 0 0 0 4.5 20.5h15a2 2 0 0 0 1.7-3L13.7 4.3a2 2 0 0 0-3.4 0Z" /><path d="M12 9.5v4.25M12 17h.01" /></>,
  text: <><path d="M4 18 8.5 6h1L14 18M5.6 14h6.8" /><path d="M16 11.5c.5-1 1.4-1.5 2.4-1.5 1.5 0 2.1.9 2.1 2.3V18M20.5 14.5c-2.9 0-4.75.6-4.75 2 0 .95.75 1.6 1.9 1.6 1.4 0 2.85-.95 2.85-2.8" /></>,
  context: <><path d="M4 7h11M4 11.5h8" /><path d="M16.5 10v8M14.75 10h3.5M14.75 18h3.5" /><path d="M4 16h6" /></>,
  switch: <><path d="M4 8.5h13.5L14.5 5.5M20 15.5H6.5l3 3" /></>,
  database: <><ellipse cx="12" cy="6" rx="7.25" ry="2.75" /><path d="M4.75 6v12c0 1.5 3.25 2.75 7.25 2.75s7.25-1.25 7.25-2.75V6M4.75 12c0 1.5 3.25 2.75 7.25 2.75s7.25-1.25 7.25-2.75" /></>,
  info: <><circle cx="12" cy="12" r="8.25" /><path d="M12 11v5" /><circle cx="12" cy="7.9" r=".95" fill="currentColor" stroke="none" /></>,
  command: <path d="M9 9V6.5A2.5 2.5 0 1 0 6.5 9H9Zm0 0h6M9 9v6m6-6V6.5A2.5 2.5 0 1 1 17.5 9H15Zm0 0v6m0 0h2.5A2.5 2.5 0 1 1 15 17.5V15Zm0 0H9m0 0v2.5A2.5 2.5 0 1 1 6.5 15H9Z" />,
  layers: <><path d="m12 3.75 8.25 4.25L12 12.25 3.75 8Z" /><path d="m3.75 12 8.25 4.25L20.25 12M3.75 16l8.25 4.25L20.25 16" /></>,
  user: <><circle cx="12" cy="8.25" r="3.75" /><path d="M4.75 20c.9-3.4 3.75-5.25 7.25-5.25s6.35 1.85 7.25 5.25" /></>,
  gear: <><circle cx="12" cy="12" r="2.75" /><path d="M12 3.25v2.1M12 18.65v2.1M20.75 12h-2.1M5.35 12h-2.1M18.19 5.81l-1.49 1.49M7.3 16.7l-1.49 1.49M18.19 18.19l-1.49-1.49M7.3 7.3 5.81 5.81" /><circle cx="12" cy="12" r="6.25" /></>,
  enter: <><path d="M19.5 5.5v6a3 3 0 0 1-3 3H5" /><path d="m9 10.5-4 4 4 4" /></>,
  history: <><path d="M3.75 12a8.25 8.25 0 1 0 2.5-5.9" /><path d="M3.75 4.5v3.75H7.5" /><path d="M12 8v4.25l2.75 1.75" /></>,
  cursor: <><path d="M12 5v14M9 5h6M9 19h6" /></>,
  tools: <><rect x="4.5" y="3.5" width="15" height="17" rx="2.5" /><path d="M8 7.5h8M8 11.5h.01M12 11.5h.01M16 11.5h.01M8 15h.01M12 15h.01M8 17.5h.01M12 17.5h.01M16 15v2.5" /></>,
  clipboard: <><path d="M9 4.5H7.25A2.25 2.25 0 0 0 5 6.75v11.5a2.25 2.25 0 0 0 2.25 2.25h9.5A2.25 2.25 0 0 0 19 18.25V6.75a2.25 2.25 0 0 0-2.25-2.25H15" /><rect x="9" y="3" width="6" height="3.25" rx="1.1" /><path d="M8.75 11h6.5M8.75 14.5h4.5" /></>,
  omega: <path d="M5 19.25h4.25v-2.1A6.25 6.25 0 0 1 12 5.25a6.25 6.25 0 0 1 2.75 11.9v2.1H19" />,
  kaomoji: <><path d="M6.25 5.5C4.5 7.25 3.75 9.5 3.75 12s.75 4.75 2.5 6.5M17.75 5.5c1.75 1.75 2.5 4 2.5 6.5s-.75 4.75-2.5 6.5" /><path d="m8.25 10 1.5 1.25-1.5 1.25M15.75 10l-1.5 1.25 1.5 1.25M10.5 15.25c1 .75 2 .75 3 0" /></>,
  smile: <><circle cx="12" cy="12" r="8.25" /><path d="M8.5 14c.9 1.4 2.1 2.1 3.5 2.1s2.6-.7 3.5-2.1" /><circle cx="9" cy="10" r=".95" fill="currentColor" stroke="none" /><circle cx="15" cy="10" r=".95" fill="currentColor" stroke="none" /></>,
  sticker: <><path d="M20.25 12.5V6.25a2.5 2.5 0 0 0-2.5-2.5H6.25a2.5 2.5 0 0 0-2.5 2.5v11.5a2.5 2.5 0 0 0 2.5 2.5h6.25Z" /><path d="M20.25 12.5h-5.25a2.5 2.5 0 0 0-2.5 2.5v5.25" /><circle cx="9" cy="9.5" r=".95" fill="currentColor" stroke="none" /><circle cx="15" cy="9.5" r=".95" fill="currentColor" stroke="none" /></>,
  star: <path d="m12 3.75 2.5 5.1 5.6.8-4.05 3.95.95 5.6L12 16.55 7 19.2l.95-5.6L3.9 9.65l5.6-.8Z" />,
  pin: <><path d="M9.25 3.75h5.5M10 3.75v5.5L7 13h10l-3-3.75v-5.5" /><path d="M12 13v7.25" /></>,
  image: <><rect x="3.75" y="4.75" width="16.5" height="14.5" rx="2.5" /><circle cx="9" cy="9.75" r="1.5" /><path d="m4.5 17 4.75-4.25 3.5 3 2.75-2.25 4.25 3.5" /></>,
  calendar: <><rect x="3.75" y="5" width="16.5" height="15" rx="2.5" /><path d="M3.75 9.75h16.5M8 3v4M16 3v4" /></>,
  upload: <><path d="M12 15.5V4.5m-4.25 4.25L12 4.5l4.25 4.25" /><path d="M4.5 14.5v2.75a2.25 2.25 0 0 0 2.25 2.25h10.5a2.25 2.25 0 0 0 2.25-2.25V14.5" /></>,
  download: <><path d="M12 4.5v11m-4.25-4.25L12 15.5l4.25-4.25" /><path d="M4.5 14.5v2.75a2.25 2.25 0 0 0 2.25 2.25h10.5a2.25 2.25 0 0 0 2.25-2.25V14.5" /></>,
  panel: <><rect x="3" y="4.5" width="18" height="15" rx="2.75" /><path d="M8.5 4.5v15M11.5 9h6M11.5 12.5h6M11.5 16h3.5" /></>,
  lock: <><rect x="4.5" y="10.5" width="15" height="10" rx="2.5" /><path d="M8 10.5V8a4 4 0 0 1 8 0v2.5M12 14.5v2.5" /></>,
  english: <><rect x="3" y="5" width="18" height="14" rx="2.75" /><path d="M10.25 8.75H7v6.5h3.25M7 12h2.75" /><path d="M13 15.25v-4.5M13 12.6c.35-1.2 1.1-1.85 2.15-1.85 1.2 0 1.85.8 1.85 2.1v2.4" /></>,
};

export function Icon({ name, size = 20, style, className }: { name: IconName; size?: number; style?: CSSProperties; className?: string }) {
  return <svg className={className} style={style} width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">{paths[name]}</svg>;
}

export function BrandMark({ size = 32 }: { size?: number }) {
  // Small marks use the simplified render (no keyboard), like the small ICO entries.
  return <img className="brand-mark" src={size < 32 ? brandSmall : brandMark} width={size} height={size} alt="" aria-hidden="true" draggable={false} />;
}
