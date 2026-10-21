import { createContext, Fragment, useContext, useEffect, useId, useRef, useState, type CSSProperties, type ReactNode } from 'react';
import { createPortal } from 'react-dom';
import { Icon, type IconName } from './icons';
import type { CandidateStyle } from './types';

export function ErrorNotice({ message }: { message: string | null }) {
  return message ? <div className="error-notice" role="alert"><Icon name="warning" size={16} /><span>{message}</span></div> : null;
}

// The pinyin "kicker" above every title types itself in, like the candidate
// window's pinyin row, and ends in the brand caret.
export function Kicker({ pinyin }: { pinyin: string }) {
  const parts = pinyin.split("'");
  return <span className="kicker" aria-hidden="true" style={{ '--chars': pinyin.length } as CSSProperties}>
    <span className="kicker-text">{parts.map((part, index) => <Fragment key={index}>{index > 0 && <span className="kicker-separator">'</span>}{part}</Fragment>)}</span>
    <span className="kicker-caret" />
  </span>;
}

export function PageHeader({ kicker, title, description, action }: { kicker: string; title: string; description: string; action?: ReactNode }) {
  return <header className="page-header">
    <div className="page-heading"><Kicker pinyin={kicker} /><h1>{title}</h1><p>{description}</p></div>
    {action && <div className="page-actions">{action}</div>}
  </header>;
}

// The candidate style the settings are drawn in (the sidebar's current page).
// A switch that is on takes that style's color where it has one of its own:
// the highlighter's yellow, the split-flap's amber (styles.css).
export const CandidateStyleContext = createContext<CandidateStyle | undefined>(undefined);

export function Switch({ checked, disabled, label, onChange }: { checked: boolean; disabled?: boolean; label: string; onChange: (checked: boolean) => void }) {
  const style = useContext(CandidateStyleContext);
  return <button type="button" className="switch" role="switch" aria-label={label} aria-checked={checked} disabled={disabled}
    data-cand-style={style} onClick={() => onChange(!checked)}><span className="switch-thumb" /></button>;
}

// The details button beside a setting's name. Explanations stay out of the
// way until asked for; the button opens them (and any demonstration) below.
export function InfoButton({ label, open, controls, onToggle }: { label: string; open: boolean; controls: string; onToggle: (open: boolean) => void }) {
  return <button type="button" className={`info-button${open ? ' open' : ''}`} aria-label={`${label}详情`} aria-expanded={open}
    aria-controls={controls} title={open ? '收起说明' : '查看说明'} onClick={() => onToggle(!open)}>
    <Icon name="info" size={15} />
  </button>;
}

// The opened explanation: an optional demonstration, then the text.
function SettingDetail({ id, title, description, demo, open }: { id: string; title: string; description?: ReactNode; demo?: ReactNode; open: boolean }) {
  return <div className="setting-detail" id={id} role="region" aria-label={`${title}说明`} hidden={!open}>
    {open && demo}{description && <p>{description}</p>}
  </div>;
}

// Settings are lists of rows in titled groups. A row is the setting's name
// with its control on the right: a switch for on/off, a selector for a choice
// among a few, a status for what is fixed. What it does is behind the
// details button.
export function SettingsGroup({ id, title, description, demo, children }: {
  id: string; title: string; description?: ReactNode; demo?: ReactNode; children: ReactNode;
}) {
  const [open, setOpen] = useState(false);
  const detailId = useId();
  return <section className="settings-group" aria-labelledby={id}>
    <div className="settings-group-head"><h2 id={id}>{title}</h2>
      {(description || demo) && <InfoButton label={title} open={open} controls={detailId} onToggle={setOpen} />}</div>
    {(description || demo) && <div className="settings-group-detail"><SettingDetail id={detailId} title={title} description={description} demo={demo} open={open} /></div>}
    <div className="settings-list">{children}</div>
  </section>;
}

// `tour`: where the first-run guide points (data-tour).
export function SettingRow({ title, description, demo, tour, children }: { title: string; description?: ReactNode; demo?: ReactNode; tour?: string; children?: ReactNode }) {
  const [open, setOpen] = useState(false);
  const detailId = useId();
  const details = description || demo;
  return <div className={`setting-item${open ? ' open' : ''}`} data-tour={tour}>
    <div className="setting-row">
      <div className="setting-text"><span className="setting-title">{title}
        {details && <InfoButton label={title} open={open} controls={detailId} onToggle={setOpen} />}</span></div>
      {children && <div className="setting-control">{children}</div>}
    </div>
    {details && <SettingDetail id={detailId} title={title} description={description} demo={demo} open={open} />}
  </div>;
}

// For a setting that is fixed (always on), in place of a control.
export function SettingStatus({ children }: { children: ReactNode }) {
  return <span className="setting-status">{children}</span>;
}

// A setting with more to it than one control: the row shows what is set
// now (`summary`) and opens to the explanation and detailed options below it.
export function SettingDisclosure({ title, description, summary, open, onToggle, children }: {
  title: string; description?: ReactNode; summary: ReactNode; open: boolean; onToggle: (open: boolean) => void; children: ReactNode;
}) {
  const id = useId();
  return <div className={`setting-disclosure${open ? ' open' : ''}`}>
    <button type="button" className="setting-row setting-disclosure-toggle" aria-expanded={open} aria-controls={id} onClick={() => onToggle(!open)}>
      <span className="setting-text"><span className="setting-title">{title}</span></span>
      <span className="setting-control"><span className="setting-summary">{summary}</span><Icon name="chevron" size={14} className="setting-chevron" /></span>
    </button>
    <div className="setting-disclosure-panel" id={id} role="region" aria-label={title} hidden={!open}>
      {description && <p className="setting-disclosure-note">{description}</p>}
      {children}
    </div>
  </div>;
}

// Keyboard keys, e.g. <Keys keys={['Ctrl', 'Space']} />.
export function Keys({ keys, joiner = '+', large = false }: { keys: string[]; joiner?: string; large?: boolean }) {
  return <span className={`keys${large ? ' keys-large' : ''}`}>{keys.map((key, index) => <Fragment key={`${key}-${index}`}>{index > 0 && joiner && <span className="keys-joiner">{joiner}</span>}<kbd>{key}</kbd></Fragment>)}</span>;
}

export function Badge({ tone = 'neutral', icon, children }: { tone?: 'neutral' | 'accent' | 'success'; icon?: IconName; children: ReactNode }) {
  return <span className={`badge badge-${tone}`}>{icon && <Icon name={icon} size={13} />}{children}</span>;
}

// A single ratio against a limit: accent fill on a lighter track of the same hue.
export function Meter({ value, max, label }: { value: number; max: number; label: string }) {
  const ratio = max > 0 ? Math.min(1, Math.max(0, value / max)) : 0;
  return <div className="meter" role="meter" aria-label={label} aria-valuemin={0} aria-valuemax={max} aria-valuenow={value}
    aria-valuetext={`${formatNumber(value)} / ${formatNumber(max)}`}>
    <span className="meter-fill" style={{ width: `${ratio * 100}%` }} />
  </div>;
}

// A miniature of the candidate window, for illustrations: the query bar, the
// focus row (the first candidate) and chips. `selected` marks a chip the
// user picked by number.
export function MiniCandidates({ pinyin, words, selected = 0, className = '' }: { pinyin?: string; words: string[]; selected?: number; className?: string }) {
  return <span className={`mini-candidates${className ? ` ${className}` : ''}`} aria-hidden="true">
    {pinyin !== undefined && <span className="mini-query"><span className="mini-mark" /><span className="mini-pinyin">{pinyin}<i /></span></span>}
    <span className="mini-focus"><b>1</b>{words[0]}</span>
    {words.length > 1 && <span className="mini-chips">{words.slice(1).map((word, index) =>
      <span key={`${word}-${index}`} className={`mini-chip${index + 1 === selected ? ' chosen' : ''}`}><b>{index + 2}</b>{word}</span>)}</span>}
  </span>;
}

export function SaveStatus({ saving }: { saving: boolean }) {
  return <span className={`save-status${saving ? ' saving' : ''}`} role="status">
    {saving ? <span className="save-spinner" aria-hidden="true" /> : <Icon name="check" size={13} />}{saving ? '正在保存' : '已自动保存'}
  </span>;
}

export function Modal({ title, description, children, onClose, busy = false, wide = false, className }: { title: string; description?: string; children: ReactNode; onClose: () => void; busy?: boolean; wide?: boolean; className?: string }) {
  const dialog = useRef<HTMLDivElement>(null);
  const closeRef = useRef(onClose);
  const busyRef = useRef(busy);
  closeRef.current = onClose;
  busyRef.current = busy;
  useEffect(() => {
    const previous = document.activeElement as HTMLElement | null;
    const element = dialog.current;
    const focusable = () => Array.from(element?.querySelectorAll<HTMLElement>('button:not(:disabled), input:not(:disabled), textarea:not(:disabled), select:not(:disabled), [tabindex="0"]') ?? []).filter(item => item.getClientRects().length > 0);
    const first = element?.querySelector<HTMLElement>('[data-autofocus]') ?? focusable()[0];
    first?.focus();
    const keydown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && !busyRef.current) { event.preventDefault(); closeRef.current(); }
      if (event.key === 'Tab') {
        const items = focusable();
        if (!items.length) { event.preventDefault(); return; }
        const firstItem = items[0];
        const lastItem = items[items.length - 1];
        if (event.shiftKey && (document.activeElement === firstItem || !element?.contains(document.activeElement))) { event.preventDefault(); lastItem.focus(); }
        else if (!event.shiftKey && (document.activeElement === lastItem || !element?.contains(document.activeElement))) { event.preventDefault(); firstItem.focus(); }
      }
    };
    document.addEventListener('keydown', keydown);
    return () => { document.removeEventListener('keydown', keydown); previous?.focus(); };
  }, []);
  // Rendered at <body>: pages animate with a transform, which would otherwise
  // capture the fixed backdrop.
  return createPortal(<div className="modal-backdrop" onMouseDown={event => { if (event.target === event.currentTarget && !busy) onClose(); }}>
    <div ref={dialog} className={`modal${wide ? ' modal-wide' : ''}${className ? ` ${className}` : ''}`} role="dialog" aria-modal="true" aria-labelledby="modal-title">
      <header className="modal-header"><div><h2 id="modal-title">{title}</h2>{description && <p>{description}</p>}</div><button className="icon-button" aria-label="关闭对话框" disabled={busy} onClick={onClose}><Icon name="close" size={16} /></button></header>
      {children}
    </div>
  </div>, document.body);
}

export function asError(error: unknown): string {
  return error instanceof Error ? error.message : '操作未完成，请重试。';
}

export function formatNumber(value: number): string {
  return new Intl.NumberFormat('zh-CN').format(value);
}
