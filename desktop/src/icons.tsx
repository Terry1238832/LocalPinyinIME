import type { CSSProperties } from 'react';
import brandSmall from '../../assets/branding/icon/small/LocalPinyinIME-40.png';

export type IconName = 'appearance' | 'input' | 'dictionary' | 'learning' | 'about' | 'sun' | 'moon' | 'monitor' | 'chevron' | 'search' | 'plus' | 'edit' | 'trash' | 'folder' | 'check' | 'close' | 'minimize' | 'maximize' | 'shield' | 'arrow' | 'keyboard' | 'spark' | 'reset' | 'warning';

const paths: Record<IconName, React.ReactNode> = {
  appearance: <><rect x="3" y="3" width="18" height="18" rx="4" /><path d="M4 9h16M9 10v10" /></>,
  input: <><rect x="2" y="4" width="20" height="15" rx="4" /><path d="M6 9h12M6 14h7M6 19v3" /></>,
  dictionary: <><rect x="4" y="3" width="17" height="18" rx="3" /><path d="M9 4v16M12 9h5M12 14h5" /></>,
  learning: <path d="m12 2 3 7 7 3-7 3-3 7-3-7-7-3 7-3Z" />,
  about: <><circle cx="12" cy="12" r="9" /><path d="M12 11v6" /><circle cx="12" cy="7" r="1" fill="currentColor" stroke="none" /></>,
  sun: <><circle cx="12" cy="12" r="4" /><path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4m11.4 11.4 1.4 1.4M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" /></>,
  moon: <path d="M20 14.5A8.5 8.5 0 0 1 9.5 4a8.5 8.5 0 1 0 10.5 10.5Z" />,
  monitor: <><rect x="3" y="4" width="18" height="13" rx="2" /><path d="M12 17v4M8 21h8" /></>,
  chevron: <path d="m9 5 7 7-7 7" />,
  search: <><circle cx="10" cy="10" r="7" /><path d="m16 16 6 6" /></>,
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
  spark: <path d="m12 2 3 7 7 3-7 3-3 7-3-7-7-3 7-3Z" />,
  reset: <><path d="M4 10a8 8 0 1 1 2 8M4 10V4M4 10h6" /><path d="M12 7v5l3 2" /></>,
  warning: <><path d="m12 3 10 18H2Z" /><path d="M12 9v5M12 17h.01" /></>,
};

export function Icon({ name, size = 20, style, className }: { name: IconName; size?: number; style?: CSSProperties; className?: string }) {
  return <svg className={className} style={style} width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">{paths[name]}</svg>;
}

export function BrandMark({ size = 40 }: { size?: number }) {
  return <img className="brand-mark" src={brandSmall} width={size} height={size} alt="" aria-hidden="true" draggable={false} />;
}
