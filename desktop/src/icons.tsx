import type { CSSProperties, ReactNode } from 'react';
import brandMark from '../../assets/branding/icon/LocalPinyinIME.svg';
import brandSmall from '../../assets/branding/icon/small/LocalPinyinIME-40.png';

export type IconName = 'appearance' | 'input' | 'dictionary' | 'learning' | 'about' | 'sun' | 'moon' | 'monitor' | 'chevron' | 'search' | 'plus' | 'edit' | 'trash' | 'folder' | 'check' | 'close' | 'minimize' | 'maximize' | 'shield' | 'arrow' | 'keyboard' | 'spark' | 'reset' | 'warning' | 'text' | 'context' | 'switch' | 'database' | 'info';

const paths: Record<IconName, ReactNode> = {
  appearance: <><circle cx="12" cy="12" r="8.5" /><path d="M12 3.5v17" /><path d="M12 3.5a8.5 8.5 0 0 1 0 17Z" fill="currentColor" stroke="none" /></>,
  input: <><rect x="2.5" y="5" width="19" height="14" rx="3" /><path d="M6.5 9.5h.01M10 9.5h.01M14 9.5h.01M17.5 9.5h.01M7.5 14.5h9" /></>,
  dictionary: <><path d="M5 4.5A1.5 1.5 0 0 1 6.5 3H19v15H6.5A1.5 1.5 0 0 0 5 19.5Z" /><path d="M5 19.5A1.5 1.5 0 0 0 6.5 21H19v-3" /><path d="M9 7.5h6M9 11h4" /></>,
  learning: <><path d="M12 3.5 14.2 9l5.8.5-4.4 3.8 1.4 5.7L12 16l-5 3 1.4-5.7L4 9.5 9.8 9Z" /></>,
  about: <><circle cx="12" cy="12" r="8.5" /><path d="M12 11v5.5" /><circle cx="12" cy="7.75" r="1" fill="currentColor" stroke="none" /></>,
  sun: <><circle cx="12" cy="12" r="4" /><path d="M12 2.5v2M12 19.5v2M2.5 12h2M19.5 12h2M5.3 5.3l1.4 1.4M17.3 17.3l1.4 1.4M5.3 18.7l1.4-1.4M17.3 6.7l1.4-1.4" /></>,
  moon: <path d="M19.5 14.5A8 8 0 0 1 9.5 4.5a8 8 0 1 0 10 10Z" />,
  monitor: <><rect x="3" y="4" width="18" height="12.5" rx="2" /><path d="M9 20h6M12 16.5V20" /></>,
  chevron: <path d="m9.5 6 6 6-6 6" />,
  search: <><circle cx="10.5" cy="10.5" r="6.5" /><path d="m15.5 15.5 5 5" /></>,
  plus: <path d="M12 5v14M5 12h14" />,
  edit: <><path d="M14.5 5.5 18.5 9.5 9 19H5v-4Z" /><path d="m13 7 4 4" /></>,
  trash: <><path d="M4.5 6.5h15M9.5 6.5V4h5v2.5M6.5 6.5l1 13.5h9l1-13.5M10 10.5v6M14 10.5v6" /></>,
  folder: <path d="M3 7.5A2 2 0 0 1 5 5.5h4.5l2 2H19a2 2 0 0 1 2 2V18a1.5 1.5 0 0 1-1.5 1.5h-15A1.5 1.5 0 0 1 3 18Z" />,
  check: <path d="m5 12.5 4.5 4.5L19 7.5" />,
  close: <path d="m6.5 6.5 11 11M6.5 17.5l11-11" />,
  minimize: <path d="M6 12h12" />,
  maximize: <rect x="6" y="6" width="12" height="12" rx="1.5" />,
  shield: <><path d="M12 3 19.5 6v5.5c0 4.2-3 7.6-7.5 9.5-4.5-1.9-7.5-5.3-7.5-9.5V6Z" /><path d="m8.8 12 2.3 2.3 4.2-4.6" /></>,
  arrow: <path d="M5 12h14m-5-5 5 5-5 5" />,
  keyboard: <><rect x="2.5" y="5" width="19" height="14" rx="3" /><path d="M6.5 9.5h.01M10 9.5h.01M14 9.5h.01M17.5 9.5h.01M6.5 13h.01M10 13h.01M14 13h.01M17.5 13h.01M8 16.5h8" /></>,
  spark: <path d="M12 3c.6 4.2 2.8 6.4 7 7-4.2.6-6.4 2.8-7 7-.6-4.2-2.8-6.4-7-7 4.2-.6 6.4-2.8 7-7Z" />,
  reset: <><path d="M4.5 12a7.5 7.5 0 1 0 2.2-5.3L4.5 9" /><path d="M4.5 4.5V9H9" /></>,
  warning: <><path d="M10.3 4.3 2.8 17.5A2 2 0 0 0 4.5 20.5h15a2 2 0 0 0 1.7-3L13.7 4.3a2 2 0 0 0-3.4 0Z" /><path d="M12 9.5v4.5M12 17h.01" /></>,
  text: <><path d="M5 6.5V5h14v1.5M12 5v14M9 19h6" /></>,
  context: <><path d="M4 6h10M4 10h16M4 14h7" /><path d="M14.5 14.5h5v5h-5z" /><path d="M17 16v2" /></>,
  switch: <><path d="M4 8h13l-3-3M20 16H7l3 3" /></>,
  database: <><ellipse cx="12" cy="6" rx="7.5" ry="3" /><path d="M4.5 6v12c0 1.7 3.4 3 7.5 3s7.5-1.3 7.5-3V6M4.5 12c0 1.7 3.4 3 7.5 3s7.5-1.3 7.5-3" /></>,
  info: <><circle cx="12" cy="12" r="8.5" /><path d="M12 11v5.5" /><circle cx="12" cy="7.75" r="1" fill="currentColor" stroke="none" /></>,
};

export function Icon({ name, size = 20, style, className }: { name: IconName; size?: number; style?: CSSProperties; className?: string }) {
  return <svg className={className} style={style} width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">{paths[name]}</svg>;
}

export function BrandMark({ size = 32 }: { size?: number }) {
  // Small marks use the simplified render (no keyboard), like the small ICO entries.
  return <img className="brand-mark" src={size < 32 ? brandSmall : brandMark} width={size} height={size} alt="" aria-hidden="true" draggable={false} />;
}
