import { useEffect, useMemo, useRef, useState } from 'react';
import { Icon, type IconName } from './icons';
import { Keys } from './components';

export type Command = {
  id: string;
  title: string;
  group: '页面' | '设置' | '操作';
  icon: IconName;
  keywords?: string;
  keys?: string[];
  hint?: string;
  run: () => void;
};

// Actions that belong to a page are requested with this event; the page that
// owns the dialog listens for it.
export type PageCommand = 'add-word' | 'browse-dictionary' | 'clear-learning' | 'open-data';
export const pageCommandEvent = 'localpinyin:command';
export function requestPageCommand(command: PageCommand) {
  window.dispatchEvent(new CustomEvent<PageCommand>(pageCommandEvent, { detail: command }));
}
export function usePageCommand(command: PageCommand, handler: () => void) {
  const latest = useRef(handler);
  latest.current = handler;
  useEffect(() => {
    const listener = (event: Event) => { if ((event as CustomEvent<PageCommand>).detail === command) latest.current(); };
    window.addEventListener(pageCommandEvent, listener);
    return () => window.removeEventListener(pageCommandEvent, listener);
  }, [command]);
}

export function CommandPalette({ commands, onClose }: { commands: Command[]; onClose: () => void }) {
  const [query, setQuery] = useState('');
  const [active, setActive] = useState(0);
  const input = useRef<HTMLInputElement>(null);
  const list = useRef<HTMLDivElement>(null);
  const results = useMemo(() => {
    const terms = query.trim().toLowerCase().split(/\s+/).filter(Boolean);
    return commands.filter(command => {
      const haystack = `${command.title} ${command.keywords ?? ''} ${command.group}`.toLowerCase();
      return terms.every(term => haystack.includes(term));
    });
  }, [commands, query]);
  useEffect(() => setActive(0), [query]);
  useEffect(() => {
    const previous = document.activeElement as HTMLElement | null;
    input.current?.focus();
    return () => previous?.focus();
  }, []);
  useEffect(() => {
    list.current?.querySelector<HTMLElement>(`[data-index="${active}"]`)?.scrollIntoView({ block: 'nearest' });
  }, [active]);
  const run = (command: Command | undefined) => {
    if (!command) return;
    onClose();
    // Let the palette unmount (and restore focus) before the command opens a dialog.
    window.setTimeout(command.run, 0);
  };
  const onKeyDown = (event: React.KeyboardEvent) => {
    if (event.key === 'ArrowDown' || event.key === 'ArrowUp') {
      event.preventDefault();
      if (!results.length) return;
      const step = event.key === 'ArrowDown' ? 1 : -1;
      setActive(current => (current + step + results.length) % results.length);
    } else if (event.key === 'Enter') {
      event.preventDefault();
      run(results[active]);
    } else if (event.key === 'Escape') {
      event.preventDefault();
      onClose();
    } else if (event.key === 'Tab') {
      event.preventDefault();
    }
  };
  let previousGroup = '';
  return <div className="palette-backdrop" onMouseDown={event => { if (event.target === event.currentTarget) onClose(); }}>
    <div className="palette" role="dialog" aria-modal="true" aria-label="命令面板" onKeyDown={onKeyDown}>
      <div className="palette-search">
        <Icon name="search" size={18} />
        <input ref={input} value={query} onChange={event => setQuery(event.target.value)} placeholder="搜索页面、设置和操作…"
          role="combobox" aria-expanded="true" aria-controls="palette-results" aria-autocomplete="list"
          aria-activedescendant={results[active] ? `palette-${results[active].id}` : undefined} spellCheck={false} autoComplete="off" />
        <kbd>Esc</kbd>
      </div>
      <div className="palette-results" id="palette-results" role="listbox" aria-label="命令" ref={list}>
        {results.map((command, index) => {
          const heading = command.group !== previousGroup ? command.group : null;
          previousGroup = command.group;
          return <div key={command.id} role="presentation">
            {heading && <div className="palette-group" role="presentation">{heading}</div>}
            <div id={`palette-${command.id}`} data-index={index} role="option" aria-selected={index === active}
              className={`palette-item${index === active ? ' active' : ''}`}
              onMouseMove={() => setActive(index)} onClick={() => run(command)}>
              <span className="palette-icon"><Icon name={command.icon} size={16} /></span>
              <span className="palette-title">{command.title}</span>
              {command.hint && <span className="palette-hint">{command.hint}</span>}
              {command.keys && <Keys keys={command.keys} joiner="" />}
            </div>
          </div>;
        })}
        {!results.length && <div className="palette-empty"><Icon name="search" size={20} /><span>没有找到“{query.trim()}”</span></div>}
      </div>
      <footer className="palette-footer"><span><kbd>↑</kbd><kbd>↓</kbd>选择</span><span><kbd>Enter</kbd>执行</span><span className="palette-brand">本地拼音</span></footer>
    </div>
  </div>;
}
