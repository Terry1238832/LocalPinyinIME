import { useEffect, useRef, type ReactNode } from 'react';
import { Icon, type IconName } from './icons';

export function ErrorNotice({ message }: { message: string | null }) {
  return message ? <div className="error-notice" role="alert"><Icon name="warning" size={16} /><span>{message}</span></div> : null;
}

export function PageHeader({ title, description, action }: { title: string; description: string; action?: ReactNode }) {
  return <header className="page-header"><div><h1>{title}</h1><p>{description}</p></div>{action}</header>;
}

export function SectionTitle({ title, description }: { title: string; description?: string }) {
  return <div className="section-title"><h2>{title}</h2>{description && <p>{description}</p>}</div>;
}

// A group of setting rows, like a Windows Settings card.
export function SettingsGroup({ title, description, children, className = '' }: { title?: string; description?: string; children: ReactNode; className?: string }) {
  return <section className={`settings-group${className ? ` ${className}` : ''}`}>
    {title && <SectionTitle title={title} description={description} />}
    <div className="card group-card">{children}</div>
  </section>;
}

export function SettingRow({ icon, title, description, children }: { icon: IconName; title: string; description: string; children?: ReactNode }) {
  return <div className="setting-row"><span className="row-icon"><Icon name={icon} size={20} /></span><div className="row-copy"><h3>{title}</h3><p>{description}</p></div>{children && <div className="row-control">{children}</div>}</div>;
}

export function Switch({ checked, disabled, label, onChange }: { checked: boolean; disabled?: boolean; label: string; onChange: (checked: boolean) => void }) {
  return <span className="switch-wrap"><span className="switch-state" aria-hidden="true">{checked ? '开' : '关'}</span><button type="button" className="switch" role="switch" aria-label={label} aria-checked={checked} disabled={disabled} onClick={() => onChange(!checked)}><span /></button></span>;
}

export function Modal({ title, description, children, onClose, busy = false, wide = false }: { title: string; description?: string; children: ReactNode; onClose: () => void; busy?: boolean; wide?: boolean }) {
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
  return <div className="modal-backdrop" onMouseDown={event => { if (event.target === event.currentTarget && !busy) onClose(); }}><div ref={dialog} className={`modal${wide ? ' modal-wide' : ''}`} role="dialog" aria-modal="true" aria-labelledby="modal-title"><header className="modal-header"><div><h2 id="modal-title">{title}</h2>{description && <p>{description}</p>}</div><button className="icon-button" aria-label="关闭对话框" disabled={busy} onClick={onClose}><Icon name="close" size={18} /></button></header>{children}</div></div>;
}

export function asError(error: unknown): string {
  return error instanceof Error ? error.message : '操作未完成，请重试。';
}

export function formatNumber(value: number): string {
  return new Intl.NumberFormat('zh-CN').format(value);
}
