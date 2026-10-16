import { useLayoutEffect, useRef, useState } from 'react';
import type { KeyboardEvent } from 'react';
import { Icon } from './icons';

type Page = { start: number; end: number };
type Props = {
  words: string[];
  label: string;
  fontSize?: number;
  pending?: boolean;
  emptyText: string;
  variant: 'preview' | 'prediction' | 'context';
  rawFallback?: boolean;
  onSelect?: (word: string) => void;
};

export function PagedCandidates({ words, label, fontSize, pending = false, emptyText, variant, rawFallback = false, onSelect }: Props) {
  const container = useRef<HTMLDivElement>(null);
  const controls = useRef<HTMLDivElement>(null);
  const measurements = useRef<HTMLDivElement>(null);
  const [pages, setPages] = useState<Page[]>([{ start: 0, end: 1 }]);
  const [page, setPage] = useState(0);
  const listKey = JSON.stringify(words);

  useLayoutEffect(() => {
    const update = () => {
      if (!container.current || !measurements.current || !controls.current || !container.current.clientWidth) return;
      // Spacing comes from the stylesheet, so paging matches the rendered row.
      const gap = parseFloat(getComputedStyle(measurements.current).columnGap) || 0;
      const rowGap = parseFloat(getComputedStyle(container.current).columnGap) || 0;
      const available = Math.max(1, container.current.clientWidth - controls.current.getBoundingClientRect().width - rowGap);
      const widths = Array.from(measurements.current.children, child => child.getBoundingClientRect().width);
      const next: Page[] = [];
      let start = 0;
      let used = 0;
      for (let index = 0; index < widths.length; ++index) {
        const width = Math.min(available, widths[index]);
        if (index > start && used + gap + width > available) {
          next.push({ start, end: index });
          start = index;
          used = 0;
        }
        used += (index > start ? gap : 0) + width;
      }
      if (widths.length) next.push({ start, end: widths.length });
      if (!next.length) next.push({ start: 0, end: 0 });
      setPages(current => JSON.stringify(current) === JSON.stringify(next) ? current : next);
      setPage(current => Math.min(current, next.length - 1));
    };
    setPage(0);
    update();
    const observer = new ResizeObserver(update);
    if (container.current) observer.observe(container.current);
    if (measurements.current) observer.observe(measurements.current);
    return () => observer.disconnect();
  }, [listKey, fontSize, variant, rawFallback]);

  const currentPage = Math.min(page, pages.length - 1);
  const visible = pages[currentPage];
  const move = (direction: number) => {
    if (!pending) setPage(current => Math.max(0, Math.min(pages.length - 1, current + direction)));
  };
  const handleKeys = (event: KeyboardEvent<HTMLDivElement>) => {
    if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey || !words.length) return;
    if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {
      event.preventDefault();
      move(event.key === 'ArrowLeft' ? -1 : 1);
    }
  };
  const item = (word: string, number: number, measure = false) => {
    if (variant === 'prediction') return <button type="button" className="prediction-chip" key={`${word}-${number}`} title={word} disabled={pending || measure} tabIndex={measure ? -1 : undefined} onClick={measure ? undefined : () => onSelect?.(word)}>{word}</button>;
    if (variant === 'context') return <span className={`context-word${number === 1 ? ' leading' : ''}`} key={`${word}-${number}`} title={word}>{word}{rawFallback && <span className="raw-input-label">原始输入</span>}</span>;
    return <span className={`preview-candidate${number === 1 ? ' first' : ''}`} key={`${word}-${number}`} title={word}><span className="candidate-number">{number}</span><span className="candidate-word">{word}</span>{rawFallback && <span className="raw-input-label">原始输入</span>}</span>;
  };
  return <div className={`paged-candidates ${variant === 'preview' ? 'candidate-preview' : 'context-candidate-pages'}`} aria-label={label} aria-live="polite" aria-busy={pending} style={fontSize ? { fontSize: `${fontSize}px` } : undefined}>
    <div className="paged-candidate-row" ref={container} tabIndex={words.length ? 0 : undefined} role="group" aria-label={`${label}，左右箭头翻页`} onKeyDown={handleKeys}>
      <div className="paged-candidate-items">{words.length ? words.slice(visible.start, visible.end).map((word, index) => item(word, index + 1)) : <p className={variant === 'preview' ? 'preview-placeholder' : 'context-empty'}>{emptyText}</p>}</div>
      <div className="candidate-page-controls" ref={controls} hidden={!words.length}>
        <button type="button" aria-label={`${label}上一页`} disabled={pending || currentPage === 0} onClick={() => move(-1)}><Icon name="chevron" size={13} className="page-chevron-left" /></button>
        <span className="candidate-page-status" aria-label={`${label}页码`}>{currentPage + 1}/{pages.length}</span>
        <button type="button" aria-label={`${label}下一页`} disabled={pending || currentPage + 1 === pages.length} onClick={() => move(1)}><Icon name="chevron" size={13} /></button>
      </div>
    </div>
    <div className="candidate-measurements" ref={measurements} aria-hidden="true">{words.map((word, index) => item(word, index + 1, true))}</div>
  </div>;
}
