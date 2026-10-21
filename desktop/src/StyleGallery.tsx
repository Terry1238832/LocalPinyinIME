import { useEffect, useId, useRef, useState } from 'react';
import { StyleThumbnail, candidateStyles } from './CandidatePreview';
import { InfoButton, Modal } from './components';
import { Icon } from './icons';
import type { CandidateStyle } from './types';

// The candidate styles on the Appearance page: four cards at a time, the
// arrows beside them turn to the next four, and 更多 shows all of them at once.

const perGroup = 4;
const groups = Array.from({ length: Math.ceil(candidateStyles.length / perGroup) },
  (_, index) => candidateStyles.slice(index * perGroup, (index + 1) * perGroup));
const groupOf = (style: CandidateStyle) => Math.max(0, Math.floor(candidateStyles.findIndex(item => item.value === style) / perGroup));
const reducedMotion = () => window.matchMedia('(prefers-reduced-motion: reduce)').matches;

type Item = (typeof candidateStyles)[number];
type CardProps = { item: Item; group: string; active: boolean; disabled: boolean; detailed?: boolean; onChoose: (style: CandidateStyle) => void };

function StyleCard({ item, group, active, disabled, detailed = false, onChoose }: CardProps) {
  return <label className={`style-card${active ? ' active' : ''}`}>
    <input type="radio" name={group} value={item.value} aria-label={`${item.name} ${item.english}`}
      checked={active} disabled={disabled} onChange={() => onChoose(item.value)} />
    <StyleThumbnail candidateStyle={item.value} />
    <span className="style-card-text">
      <span className="style-card-name"><strong>{item.name}</strong><span>{item.english}</span>
        {active && <span className="style-card-check"><Icon name="check" size={12} />使用中</span>}</span>
      {detailed && <span className="style-card-description">{item.description}</span>}
    </span>
  </label>;
}

export function StyleGallery({ value, disabled, onChoose }: { value: CandidateStyle; disabled: boolean; onChoose: (style: CandidateStyle) => void }) {
  const [info, setInfo] = useState(false);
  const [overview, setOverview] = useState(false);
  const [group, setGroup] = useState(() => groupOf(value));
  const viewport = useRef<HTMLDivElement>(null);
  const shown = useRef(false);
  // The group an arrow is scrolling to; until the scroll gets there, the
  // groups it passes do not count.
  const heading = useRef<number | null>(null);
  const infoId = useId();

  const show = (index: number, smooth: boolean) => {
    const next = Math.max(0, Math.min(groups.length - 1, index));
    setGroup(next);
    const element = viewport.current;
    if (!element) return;
    const left = next * element.clientWidth;
    heading.current = Math.abs(element.scrollLeft - left) < 1 ? null : next;
    element.scrollTo({ left, behavior: smooth && !reducedMotion() ? 'smooth' : 'auto' });
  };
  // The chosen style's group is the one in view: on opening, and when the
  // choice moves (arrow keys, or a pick in the overview) to another group.
  useEffect(() => { show(groupOf(value), shown.current); shown.current = true; }, [value]);
  // A resize (or the page appearing) keeps the current group in view.
  const groupRef = useRef(group);
  groupRef.current = group;
  useEffect(() => {
    const element = viewport.current;
    if (!element) return;
    const observer = new ResizeObserver(() => { if (element.clientWidth) element.scrollLeft = groupRef.current * element.clientWidth; });
    observer.observe(element);
    return () => observer.disconnect();
  }, []);
  // Scrolling (a swipe, or focus moving to a card) settles on a group.
  const settle = () => {
    const element = viewport.current;
    if (!element?.clientWidth) return;
    const position = element.scrollLeft / element.clientWidth;
    if (heading.current !== null) {
      if (Math.abs(position - heading.current) > 0.01) return;
      heading.current = null;
    }
    setGroup(Math.round(position));
  };
  const settled = () => { heading.current = null; settle(); };

  return <section className="style-gallery" aria-labelledby="style-gallery-title" data-tour="styles">
    <div className="gallery-head">
      <h2 id="style-gallery-title">候选框风格</h2>
      <InfoButton label="候选框风格" open={info} controls={infoId} onToggle={setInfo} />
      <button type="button" className="text-button gallery-more" aria-haspopup="dialog" onClick={() => setOverview(true)}>
        更多<Icon name="chevron" size={12} /></button>
    </div>
    <div className="setting-detail gallery-detail" id={infoId} role="region" aria-label="候选框风格说明" hidden={!info}>
      <p>{candidateStyles.length} 种风格共用同一套输入规则：空格上屏、数字选词、方向键翻页都不变。风格、主题和字号选好即保存，正在显示的候选框立即换成新样式，其他程序在下一次输入时使用新设置。</p>
      <ul>{candidateStyles.map(item => <li key={item.value}><strong>{item.name}</strong>{item.description}</li>)}</ul>
    </div>
    <div className="style-carousel">
      <button type="button" className="carousel-arrow" aria-label="上一组风格" disabled={group === 0} onClick={() => show(group - 1, true)}>
        <Icon name="chevron" size={16} className="page-chevron-left" /></button>
      <div className="style-viewport" ref={viewport} onScroll={settle} onScrollEnd={settled}>
        <div className="style-cards" role="radiogroup" aria-labelledby="style-gallery-title">{groups.map((items, index) =>
          <div key={index} className="style-page" data-group={index}>{items.map(item =>
            <StyleCard key={item.value} item={item} group="candidate-style" active={value === item.value} disabled={disabled} onChoose={onChoose} />)}
          </div>)}
        </div>
      </div>
      <button type="button" className="carousel-arrow" aria-label="下一组风格" disabled={group === groups.length - 1} onClick={() => show(group + 1, true)}>
        <Icon name="chevron" size={16} /></button>
    </div>
    <div className="carousel-dots" aria-label={`第 ${group + 1} 组，共 ${groups.length} 组`} role="img">
      {groups.map((_, index) => <i key={index} className={index === group ? 'on' : undefined} />)}
    </div>
    {overview && <Modal title="全部候选框风格" description={`${candidateStyles.length} 款风格，点选即用；输入规则都一样。`}
      className="modal-gallery" onClose={() => setOverview(false)}>
      <div className="modal-body">
        <div className="style-overview" role="radiogroup" aria-label="全部候选框风格">{candidateStyles.map(item =>
          <StyleCard key={item.value} item={item} group="candidate-style-overview" active={value === item.value} disabled={disabled} detailed onChoose={onChoose} />)}
        </div>
      </div>
    </Modal>}
  </section>;
}
