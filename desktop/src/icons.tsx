import type { CSSProperties } from 'react';

export type IconName = 'appearance' | 'input' | 'dictionary' | 'learning' | 'about' | 'sun' | 'moon' | 'monitor' | 'chevron' | 'search' | 'plus' | 'edit' | 'trash' | 'folder' | 'check' | 'close' | 'minimize' | 'maximize' | 'shield' | 'arrow' | 'keyboard' | 'spark' | 'reset' | 'warning';

const paths: Record<IconName, React.ReactNode> = {
  appearance: <><circle cx="12" cy="12" r="8.5" /><path d="M12 3.5v17a8.5 8.5 0 0 0 0-17Z" fill="currentColor" stroke="none" /></>,
  input: <><rect x="3" y="5" width="18" height="14" rx="3" /><path d="M7 9h.01M11 9h.01M15 9h.01M17 13h.01M7 13h.01M11 13h.01M8 16h8" /></>,
  dictionary: <><path d="M12 6c-2.4-2-5.6-2.5-9-1v14c3.4-1.5 6.6-1 9 1 2.4-2 5.6-2.5 9-1V5c-3.4-1.5-6.6-1-9 1Zm0 0v14" /><path d="m6 9 3 .5M6 13l3 .5M15 9.5l3-.5M15 13.5l3-.5" /></>,
  learning: <><path d="M8 4.5A3.5 3.5 0 0 0 4.5 8a3.5 3.5 0 0 0-1 6.4A3.5 3.5 0 0 0 8 19.5M8 4.5a3.5 3.5 0 0 1 4 1 3.5 3.5 0 0 1 4-1A3.5 3.5 0 0 1 19.5 8a3.5 3.5 0 0 1 1 6.4A3.5 3.5 0 0 1 16 19.5a3.5 3.5 0 0 1-4-1 3.5 3.5 0 0 1-4 1" /><path d="M12 5.5v13M7 9c2 0 2.5 1 2.5 2.5M17 14c-2 0-2.5-1-2.5-2.5" /></>,
  about: <><circle cx="12" cy="12" r="8.5" /><path d="M12 11v6M12 7.3h.01" /></>,
  sun: <><circle cx="12" cy="12" r="4" /><path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4m11.4 11.4 1.4 1.4M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" /></>,
  moon: <path d="M20 14.5A8.5 8.5 0 0 1 9.5 4a8.5 8.5 0 1 0 10.5 10.5Z" />,
  monitor: <><rect x="3" y="4" width="18" height="13" rx="2" /><path d="M12 17v4M8 21h8" /></>,
  chevron: <path d="m9 5 7 7-7 7" />,
  search: <><circle cx="10.5" cy="10.5" r="6.5" /><path d="m15.5 15.5 4.5 4.5" /></>,
  plus: <path d="M12 5v14M5 12h14" />,
  edit: <><path d="m14.5 4.5 5 5-10 10-6 1 1-6Z" /><path d="m12 7 5 5" /></>,
  trash: <><path d="M4 6h16M9 6V3h6v3M6 6l1 15h10l1-15M10 10v7M14 10v7" /></>,
  folder: <path d="M3 7a2 2 0 0 1 2-2h5l2 2h7a2 2 0 0 1 2 2v10H3Z" />,
  check: <path d="m5 12 4.5 4.5L19 7" />,
  close: <path d="m6 6 12 12M6 18 18 6" />,
  minimize: <path d="M5 12h14" />,
  maximize: <rect x="5" y="5" width="14" height="14" rx="1" />,
  shield: <><path d="m12 3 8 3v6c0 4-3 7-8 9-5-2-8-5-8-9V6Z" /><path d="m8 12 3 3 5-6" /></>,
  arrow: <path d="M5 12h14m-5-5 5 5-5 5" />,
  keyboard: <><rect x="2.5" y="5" width="19" height="14" rx="2.5" /><path d="M6.5 9h.01M10 9h.01M14 9h.01M17.5 9h.01M6.5 12.5h.01M10 12.5h.01M14 12.5h.01M17.5 12.5h.01M7.5 16h9" /></>,
  spark: <><path d="m12 3 2.5 6.5L21 12l-6.5 2.5L12 21l-2.5-6.5L3 12l6.5-2.5Z" /><path d="m20 2 .5 1.5L22 4l-1.5.5L20 6l-.5-1.5L18 4l1.5-.5Z" /></>,
  reset: <><path d="M4 10a8 8 0 1 1 2 8M4 10V4M4 10h6" /><path d="M12 7v5l3 2" /></>,
  warning: <><path d="m12 3 10 18H2Z" /><path d="M12 9v5M12 17h.01" /></>,
};

export function Icon({ name, size = 20, style, className }: { name: IconName; size?: number; style?: CSSProperties; className?: string }) {
  return <svg className={className} style={style} width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">{paths[name]}</svg>;
}

export function BrandMark({ size = 40 }: { size?: number }) {
  return <svg className="brand-mark" width={size} height={size} viewBox="0 0 48 48" fill="none" aria-hidden="true"><rect width="48" height="48" rx="14" fill="currentColor" /><path d="M13 14v20h8M27 14v20M27 14h6c7 0 7 11 0 11h-6" stroke="white" strokeWidth="3.1" strokeLinecap="round" strokeLinejoin="round" /><circle cx="35.5" cy="33.5" r="2" fill="white" /></svg>;
}
