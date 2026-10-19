import { useLayoutEffect, useRef, useState } from 'react';
import type { CSSProperties, KeyboardEvent, ReactNode } from 'react';
import { Icon } from './icons';
import type { CandidateStyle } from './types';

// The candidate window on the Appearance page. Each style renders its own
// structure (as the native painters in src/ime/candidate_styles do); all of
// them share the measuring, paging and keyboard rules below, which mirror the
// native layout so the preview pages exactly like the IME.

type Page = { start: number; end: number };
export type FlowForm = 'compact' | 'balanced' | 'expanded';
// Adaptive Flow's drop, in the candidate row: its insets, where the row's
// items sit (for the white copy of the text inside it) and how each edge
// moves this time.
type Drop = { left: number; top: number; right: number; bottom: number; inkLeft: number; inkTop: number; inkWidth: number;
  timing: { left: string; top: string; right: string; bottom: string } };

// As adaptive_flow.cpp moves its drop: the edges moving the way it travels
// run ahead, a little past and back; the others follow later.
const dropLead = '240ms cubic-bezier(.3, 1.45, .55, 1)';
const dropTrail = '400ms cubic-bezier(.7, 0, .3, 1) 60ms';
function dropTiming(from: Drop | null, to: Omit<Drop, 'timing'>) {
  const all = { left: dropLead, top: dropLead, right: dropLead, bottom: dropLead };
  if (!from) return all;
  // Centers in the row, from the insets (left/right and top/bottom).
  const travelX = (to.left - to.right) - (from.left - from.right);
  const travelY = (to.top - to.bottom) - (from.top - from.bottom);
  if (Math.abs(travelX) >= Math.abs(travelY) && Math.abs(travelX) > 1)
    return { ...all, left: travelX > 0 ? dropTrail : dropLead, right: travelX > 0 ? dropLead : dropTrail };
  if (Math.abs(travelY) > 1) return { ...all, top: travelY > 0 ? dropTrail : dropLead, bottom: travelY > 0 ? dropLead : dropTrail };
  return all;
}

export const candidateStyles: { value: CandidateStyle; name: string; english: string; description: string }[] = [
  { value: 'rail', name: '光标轨道', english: 'Cursor Rail', description: '候选排在一条轨道上，焦点线随指针滑动；紧凑精准，适合高频输入。' },
  { value: 'type', name: '现代活字', english: 'Modern Type', description: '让汉字做主角：字号、字重和一道朱红短线区分主次，没有按钮感。' },
  { value: 'glass', name: '悬浮晶体', english: 'Floating Glass', description: '分层的悬浮面板，当前候选是一枚陶瓷质感的实体键帽。' },
  { value: 'flow', name: '自适应流体', english: 'Adaptive Flow', description: '当前候选是一滴流动的液体：换候选时拉长、收腰、再聚拢流过去；窗口按候选长短在紧凑、平衡、展开三种形态间切换。' },
  { value: 'capsule', name: '胶囊', english: 'Capsule', description: '整个窗口是一条圆角长条：拼音在左，候选随后，当前候选是一枚实心胶囊。窗口最矮。' },
  { value: 'column', name: '竖排列表', english: 'Column List', description: '候选一行一个，像菜单一样从上往下读；长词和整句都能完整显示。' },
  { value: 'flap', name: '翻牌', english: 'Split-Flap', description: '机场航班牌：每个字一块翻页牌，当前候选是一排琥珀色牌；候选变化时，变了的字一块接一块翻过来。' },
  { value: 'marker', name: '荧光笔', english: 'Highlighter', description: '一张笔记纸：当前候选被荧光笔划上一道，笔触跟着指针走，换词时重新划一笔。' },
];

// What the preview shows: a fixed input, as the IME would list it.
export const previewSample = {
  pinyin: "sui'bian",
  words: ['随便', '岁', '随', '碎', '虽', '遂', '穗', '隋', '髓', '祟', '燧', '邃'],
};

// The same thresholds as decide_candidate_flow_form
// (src/ime/candidate_styles/adaptive_flow.cpp): a form is entered on a clear
// signal and kept on a weaker one.
export function decideFlowForm(fontPx: number, longest: number, median: number, innerMax: number, previous: FlowForm | null): FlowForm {
  const em = Math.max(1, fontPx);
  const inner = Math.max(1, innerMax);
  const spread = longest / Math.max(1, median);
  const longEnter = Math.max(9 * em, 0.42 * inner);
  const longStay = Math.max(7 * em, 0.33 * inner);
  if (longest >= longEnter || (previous === 'expanded' && longest >= longStay)) return 'expanded';
  const compact = previous === 'compact' ? longest <= 4.2 * em && spread <= 4 : longest <= 3.2 * em && spread <= 2.6;
  return compact ? 'compact' : 'balanced';
}

// Greedy pages, as candidate_layout::paginate: from each page's first item
// add items while the row fits; the first slot is the current choice.
function paginate(count: number, available: number, gap: number, width: (index: number, first: boolean) => number,
  leadOnOwnRow = false, maxPerPage = 9): Page[] {
  const pages: Page[] = [];
  for (let start = 0; start < count;) {
    let end = start + 1;
    let used = leadOnOwnRow ? 0 : Math.min(available, width(start, true));
    let empty = leadOnOwnRow;
    for (; end < count && end - start < maxPerPage; ++end) {
      const next = used + (empty ? 0 : gap) + Math.min(available, width(end, false));
      if (next > available) break;
      used = next;
      empty = false;
    }
    pages.push({ start, end });
    start = end;
  }
  return pages.length ? pages : [{ start: 0, end: 0 }];
}

const expandedRows = 5;
// Column List: kRowsPerPage in src/ime/candidate_styles/column_list.cpp.
const columnRows = 6;

// Pinyin as the candidate window shows it: syllables, lighter separators,
// caret. Long pinyin keeps its end (where typing happens) in view.
function PinyinRow({ text }: { text: string }) {
  const parts = text.split("'");
  return <div className="ime-preview-pinyin">
    <span className="ime-pinyin-run">
      {parts.map((part, index) => <span key={index}>{index > 0 && <span className="ime-separator">'</span>}{part}</span>)}
      <span className="ime-caret" aria-hidden="true" />
    </span>
  </div>;
}

type Props = {
  candidateStyle: CandidateStyle;
  words: string[];
  pinyin: string;
  fontSize: number;
};

export function CandidatePreview({ candidateStyle: style, words, pinyin, fontSize }: Props) {
  const label = '候选预览';
  const popup = useRef<HTMLDivElement>(null);
  const row = useRef<HTMLDivElement>(null);
  const measurements = useRef<HTMLDivElement>(null);
  const focusMeasurements = useRef<HTMLDivElement>(null);
  const formMemory = useRef<FlowForm | null>(null);
  const [pages, setPages] = useState<Page[]>([{ start: 0, end: 1 }]);
  const [page, setPage] = useState(0);
  const [form, setForm] = useState<FlowForm>('balanced');
  const [hovered, setHovered] = useState<number | null>(null);
  const [marker, setMarker] = useState<{ left: number; width: number; top?: number; height?: number } | null>(null);
  const [drop, setDrop] = useState<Drop | null>(null);
  const [sloshes, setSloshes] = useState(0);
  const lastDrop = useRef<{ key: string; drop: Drop } | null>(null);
  const [ticks, setTicks] = useState<number[]>([]);
  const listKey = JSON.stringify(words);

  useLayoutEffect(() => {
    const update = () => {
      const panel = popup.current, line = row.current, list = measurements.current, focusList = focusMeasurements.current;
      if (!panel || !line || !list || !focusList) return;
      if (!words.length) {
        setPages(current => current.length === 1 && current[0].end === 0 ? current : [{ start: 0, end: 0 }]);
        return;
      }
      const gap = parseFloat(getComputedStyle(list).columnGap) || 0;
      const widths = Array.from(list.children, child => child.getBoundingClientRect().width);
      const focusWidths = Array.from(focusList.children, child => child.getBoundingClientRect().width);
      const textWidths = Array.from(list.children, child => child.querySelector('.candidate-word')?.getBoundingClientRect().width ?? 0);
      // The popup sizes to its content like the native window, so its widest
      // row comes from the stage around it, not from the popup itself.
      const stage = panel.parentElement;
      const items = line.querySelector<HTMLElement>('.paged-candidate-items');
      let available = items?.clientWidth || line.clientWidth;
      // Adaptive Flow decides against the nominal 720 px popup, as the IME
      // does, whatever width its current form allows.
      let nominal = available;
      if (stage && items) {
        const stageStyle = getComputedStyle(stage);
        const inner = stage.clientWidth - parseFloat(stageStyle.paddingLeft) - parseFloat(stageStyle.paddingRight);
        const cap = parseFloat(getComputedStyle(panel).getPropertyValue('--popup-max')) || inner;
        const chrome = panel.offsetWidth - items.clientWidth;
        available = Math.min(inner, cap) - chrome;
        nominal = Math.min(inner, 720) - chrome;
      }
      available = Math.max(1, available);
      let next: Page[];
      if (style === 'flow') {
        const sorted = [...textWidths].sort((a, b) => a - b);
        const chosen = decideFlowForm(fontSize, sorted[sorted.length - 1], sorted[Math.floor(sorted.length / 2)], Math.max(1, nominal), formMemory.current);
        formMemory.current = chosen;
        setForm(chosen);
        if (chosen === 'compact') next = paginate(widths.length, available, gap, (index, first) => widths[index] + (first ? 4 : 0));
        else if (chosen === 'balanced') next = paginate(widths.length, available, gap, index => widths[index], true);
        else next = paginate(widths.length, Infinity, 0, () => 0, false, expandedRows);
      } else if (style === 'column') {
        next = paginate(widths.length, Infinity, 0, () => 0, false, columnRows);
      } else {
        // Each page starts with the current choice, set in its own size.
        next = paginate(widths.length, available, gap, (index, first) => first ? focusWidths[index] : widths[index]);
      }
      setPages(current => JSON.stringify(current) === JSON.stringify(next) ? current : next);
      setPage(current => Math.min(current, next.length - 1));
    };
    setPage(0);
    update();
    const observer = new ResizeObserver(update);
    if (row.current) observer.observe(row.current);
    if (measurements.current) observer.observe(measurements.current);
    const stage = popup.current?.parentElement;
    if (stage) observer.observe(stage);
    return () => observer.disconnect();
  }, [listKey, fontSize, style]);

  const currentPage = Math.min(page, pages.length - 1);
  const visible = pages[currentPage];
  const shown = words.slice(visible.start, visible.end);
  const paging = pages.length > 1;
  const arrangement = style === 'column' ? 'column'
    : style !== 'flow' || form === 'compact' ? 'row' : form === 'balanced' ? 'focus-row' : 'column';
  // Column List keeps the fullest page's height while paging.
  const rows = Math.max(1, ...pages.map(item => item.end - item.start));

  // Cursor Rail's focus line, Modern Type's short rule and the Highlighter's
  // stroke rest on the current choice and move to the candidate under the pointer.
  useLayoutEffect(() => {
    const line = row.current;
    if ((style !== 'rail' && style !== 'type' && style !== 'marker') || !line || !shown.length) { setMarker(null); return; }
    const item = line.querySelector<HTMLElement>(`[data-slot="${hovered ?? 0}"]`) ?? line.querySelector<HTMLElement>('[data-slot="0"]');
    const word = item?.querySelector<HTMLElement>('.candidate-word');
    const number = item?.querySelector<HTMLElement>('.candidate-number');
    if (!item || !word || !number) { setMarker(null); return; }
    const origin = line.getBoundingClientRect().left;
    const wordBox = word.getBoundingClientRect();
    if (style === 'rail') {
      // A fine tick between neighboring candidates, like a scale's graduations.
      const boxes = Array.from(line.querySelectorAll<HTMLElement>('.paged-candidate-items [data-slot]'), element => element.getBoundingClientRect());
      const next = boxes.slice(1).map((box, index) => Math.round((boxes[index].right + box.left) / 2 - origin));
      setTicks(current => JSON.stringify(current) === JSON.stringify(next) ? current : next);
      const left = number.getBoundingClientRect().left - origin;
      setMarker({ left, width: Math.min(wordBox.right, item.getBoundingClientRect().right) - origin - left });
    } else if (style === 'marker') {
      // Over most of the word, from just before it to a little past its
      // end, slightly low: as highlighter.cpp places it.
      const size = parseFloat(getComputedStyle(word).fontSize);
      const center = (wordBox.top + wordBox.bottom) / 2 - line.getBoundingClientRect().top;
      setMarker({ left: wordBox.left - origin - size * 0.08, width: wordBox.width + size * 0.33,
        top: center - size * 0.3, height: size * 0.75 });
    } else {
      const size = parseFloat(getComputedStyle(word).fontSize);
      setMarker({ left: wordBox.left - origin, width: Math.min(wordBox.width, Math.max(14, size)) });
    }
  }, [hovered, currentPage, listKey, fontSize, style, pages]);

  // Adaptive Flow's drop rests on the current choice, hugging its text, and
  // in Compact and Expanded flows to the candidate under the pointer. A new
  // page or list pours it in from its left end.
  const dropKey = `${listKey}:${currentPage}:${form}`;
  useLayoutEffect(() => {
    const line = row.current;
    const items = line?.querySelector<HTMLElement>('.paged-candidate-items');
    if (style !== 'flow' || !line || !items || !shown.length) { setDrop(null); lastDrop.current = null; return; }
    const slot = form !== 'balanced' && hovered !== null ? hovered : 0;
    const item = items.querySelector<HTMLElement>(`[data-slot="${slot}"]`);
    const word = item?.querySelector<HTMLElement>('.candidate-word');
    const number = item?.querySelector<HTMLElement>('.candidate-number');
    if (!item || !word || !number) { setDrop(null); return; }
    const base = line.getBoundingClientRect();
    const box = item.getBoundingClientRect();
    const text = document.createRange();
    text.selectNodeContents(word);
    const lead = number.getBoundingClientRect().left - box.left;
    const right = Math.min(box.right, Math.max(text.getBoundingClientRect().right + lead + 2, box.left + box.height * 1.5));
    const rest = { left: box.left - base.left, top: box.top - base.top, right: base.right - right, bottom: base.bottom - box.bottom,
      inkLeft: items.offsetLeft - (box.left - base.left), inkTop: items.offsetTop - (box.top - base.top), inkWidth: items.offsetWidth };
    const previous = lastDrop.current?.key === dropKey ? lastDrop.current.drop : null;
    const next = { ...rest, timing: dropTiming(previous, rest) };
    if (previous && ['left', 'top', 'right', 'bottom'].every(edge =>
      Math.abs(previous[edge as 'left'] - next[edge as 'left']) < 0.5)) return;
    lastDrop.current = { key: dropKey, drop: next };
    setSloshes(count => count + 1);
    if (previous) { setDrop(next); return; }
    // Poured in: first a round drop at the left end, then the full length.
    const height = box.height;
    setDrop({ ...next, right: base.right - (box.left + height), timing: { ...next.timing, right: '0s' } });
    // Not cancelled on a re-run: the next run sees the full drop as current.
    requestAnimationFrame(() => setDrop(current => current && lastDrop.current?.drop === next
      ? { ...next, timing: { ...next.timing, right: '340ms cubic-bezier(.3, 1.35, .5, 1)' } } : current));
  }, [hovered, currentPage, listKey, fontSize, style, pages, form]);

  const move = (direction: number) => {
    setPage(current => Math.max(0, Math.min(pages.length - 1, current + direction)));
  };
  const handleKeys = (event: KeyboardEvent<HTMLDivElement>) => {
    if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey || !words.length) return;
    if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {
      event.preventDefault();
      move(event.key === 'ArrowLeft' ? -1 : 1);
    }
  };

  // Split-Flap: where each slot's tiles start along the row, so the tiles
  // turn over one after another from the left.
  const tileStarts = shown.reduce<number[]>((starts, word, index) =>
    [...starts, index ? starts[index - 1] + Array.from(shown[index - 1]).length : 0], []);
  // Adaptive Flow: where the current choice is while the drop is away.
  const traced = (slot: number) => style === 'flow' && form !== 'balanced' && slot === 0 && hovered !== null && hovered !== 0;
  const candidate = (word: string, slot: number, focus: boolean, extra?: ReactNode, measure = false) =>
    <span className={`preview-candidate ${focus ? 'focus' : 'chip'}${style === 'marker' && !measure && slot === (hovered ?? 0) ? ' marked' : ''}${!measure && traced(slot) ? ' traced' : ''}`}
      key={`${word}-${slot}`} title={word} data-slot={measure ? undefined : slot}
      style={style === 'flow' ? { '--slot': slot } as CSSProperties : undefined}
      onMouseEnter={measure ? undefined : () => setHovered(slot)}
      onMouseLeave={measure ? undefined : () => setHovered(current => current === slot ? null : current)}>
      <span className="candidate-number">{slot + 1}</span>
      <span className="candidate-word">{style === 'flap'
        ? Array.from(word).map((character, index) => <span key={index} className="flap-tile"
            style={measure ? undefined : { animationDelay: `${((tileStarts[slot] ?? 0) + index) * 16}ms` }}>{character}</span>)
        : word}</span>
      {extra}
    </span>;
  const hint = <span className="focus-hint" aria-hidden="true">空格</span>;

  // `copy`: the same candidates, passive, for the white text inside the drop.
  const items = (copy = false): ReactNode => {
    if (style === 'flow' && form === 'balanced')
      return <>{candidate(shown[0], 0, true, hint, copy)}{shown.length > 1 && <div className="palette-chips">{shown.slice(1).map((word, index) => candidate(word, index + 1, false, undefined, copy))}</div>}</>;
    if (style === 'flow' && form === 'expanded')
      return shown.map((word, index) => candidate(word, index, index === 0, index === 0 ? hint : undefined, copy));
    return shown.map((word, index) => candidate(word, index, index === 0, undefined, copy));
  };
  const body = items();

  const pager = <div className="candidate-pager" hidden={!paging}>
    <button type="button" aria-label={`${label}上一页`} disabled={currentPage === 0} onClick={() => move(-1)}><Icon name="chevron" size={13} className="page-chevron-left" /></button>
    <span className="candidate-page-slot">
      {/* Up to three pages read as dots (the current one a short accent bar),
          like the native window; the "n/m" text stays for assistive tech. */}
      {pages.length <= 3 && <span className="page-dots" aria-hidden="true">{pages.map((_, index) => <i key={index} className={index === currentPage ? 'on' : undefined} />)}</span>}
      <span className={`candidate-page-status${pages.length <= 3 ? ' visually-hidden' : ''}`} aria-label={`${label}页码`}>{`${currentPage + 1}/${pages.length}`}</span>
    </span>
    <button type="button" aria-label={`${label}下一页`} disabled={currentPage + 1 === pages.length} onClick={() => move(1)}><Icon name="chevron" size={13} /></button>
  </div>;

  return <div className="ime-preview" ref={popup} data-cand-style={style} data-arrangement={arrangement} data-form={style === 'flow' ? form : undefined}>
    <div className="paged-candidates candidate-preview" aria-label={label} aria-live="polite" style={{ fontSize: `${fontSize}px` }}>
      <div className="palette-query">
        {style === 'flow' && <span className="palette-mark" aria-hidden="true" />}
        {style === 'type' && <span className="type-mark" aria-hidden="true" />}
        <PinyinRow text={pinyin} />
        {style === 'capsule' && <span className="capsule-divider" aria-hidden="true" />}
        <div className="candidate-page-controls" hidden={!words.length}>
          {pager}
          <span className="candidate-gear" aria-hidden="true"><Icon name="gear" size={15} /></span>
        </div>
      </div>
      <div className="paged-candidate-row" ref={row} tabIndex={words.length ? 0 : undefined} role="group" aria-label={`${label}，左右箭头翻页`} onKeyDown={handleKeys}>
        <div className="paged-candidate-items" key={`${listKey}:${currentPage}:${style}:${form}`}
          style={style === 'column' ? { '--rows': rows } as CSSProperties : undefined}>{body}</div>
        {style === 'rail' && words.length > 0 && <span className="rail-line" aria-hidden="true" />}
        {style === 'rail' && words.length > 0 && ticks.map(left => <i key={left} className="rail-tick" aria-hidden="true" style={{ left }} />)}
        {drop && <span className="flow-drop" aria-hidden="true" key={dropKey}
          style={{ left: drop.left, top: drop.top, right: drop.right, bottom: drop.bottom,
            transition: `left ${drop.timing.left}, top ${drop.timing.top}, right ${drop.timing.right}, bottom ${drop.timing.bottom}` }}>
          <span className="flow-drop-ink" style={{ left: drop.inkLeft, top: drop.inkTop, width: drop.inkWidth,
            transition: `left ${drop.timing.left}, top ${drop.timing.top}` }}>{items(true)}</span>
          <i className="flow-sheen" key={sloshes} />
        </span>}
        {marker && <span className={style === 'rail' ? 'rail-marker' : style === 'marker' ? 'marker-stroke' : 'type-rule'} aria-hidden="true"
          key={style === 'marker' ? `${listKey}:${currentPage}` : undefined}
          style={{ left: marker.left, width: marker.width, top: marker.top, height: marker.height }} />}
      </div>
      {style === 'rail' && paging && <div className="rail-track" aria-hidden="true"><i style={{ left: `${(currentPage * 100) / pages.length}%`, width: `${100 / pages.length}%` }} /></div>}
      <div className="candidate-measurements" ref={measurements} aria-hidden="true">{words.map((word, index) => candidate(word, index % 9, false, undefined, true))}</div>
      <div className="focus-measurements" ref={focusMeasurements} aria-hidden="true">{words.map((word, index) => candidate(word, 0, true, undefined, true))}</div>
    </div>
  </div>;
}

// A miniature of a style for its card and the theme swatches: the same
// structure and palette as the real window, at thumbnail scale.
export function StyleThumbnail({ candidateStyle: style, tone }: { candidateStyle: CandidateStyle; tone?: 'light' | 'dark' }) {
  const words = ['你好', '拟好', '你'];
  const query = <span className="thumb-query">{style === 'flow' && <b className="thumb-mark" />}{style === 'type' && <b className="thumb-seal" />}ni'hao<i /></span>;
  // Adaptive Flow's drop is drawn mid-flow: a droplet still drawing away from it.
  const body = <span className={style === 'column' ? 'thumb-list' : 'thumb-row'}>{words.map((word, index) =>
    <span key={word} className={`thumb-word${index === 0 ? ' on' : ''}`}><small>{index + 1}</small>
      {style === 'flap' ? Array.from(word).map((character, tile) => <b key={tile} className="thumb-tile">{character}</b>)
        : style === 'marker' && index === 0 ? <i className="thumb-ink">{word}</i> : word}</span>)}
    {style === 'rail' && <span className="thumb-rail"><i /></span>}
  </span>;
  return <span className="style-thumb" data-cand-style={style} data-cand-tone={tone} aria-hidden="true">
    <span className="thumb-panel">{query}{style === 'capsule' && <span className="thumb-divider" />}{body}</span>
  </span>;
}
