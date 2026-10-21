import { useEffect, useLayoutEffect, useRef, useState, type CSSProperties, type ReactNode } from 'react';
import { createPortal } from 'react-dom';
import { candidateStyles } from './CandidatePreview';
import { Keys, Modal } from './components';
import { imeSwitchKeys } from './EnglishPage';
import { Icon } from './icons';
import type { CandidateStyle, Settings } from './types';

// The first-run guide: a welcome with the user's own candidate window, then
// five steps that stop on the real controls. Seen once (finished or skipped),
// it waits in 关于 › 新手引导 and the command palette.

// Captures of the native candidate window in every style and theme
// (LocalPinyinCandidateCapture <dir> --tour) and the taskbar's 中 / 英
// glyphs (--icons), not drawings of them.
const shots = import.meta.glob('./assets/tour/*.png', { eager: true, query: '?url', import: 'default' }) as Record<string, string>;
const shot = (name: string) => shots[`./assets/tour/${name}.png`];

const storageKey = 'localpinyin.tour';
export function tourSeen(): boolean {
  try { return localStorage.getItem(storageKey) === 'done'; }
  catch { return true; }
}
function markTourSeen() {
  try { localStorage.setItem(storageKey, 'done'); }
  catch { /* asked again next time */ }
}

type TourPage = 'appearance' | 'input' | 'english' | 'panel';
interface TourStep {
  // The page it is on; none: the sidebar, on any page.
  page?: TourPage;
  // data-tour of what it points at.
  target: string;
  title: string;
  body: (settings: Settings, theme: 'light' | 'dark') => ReactNode;
  // What to try on the control, and what it says once that is done.
  tryIt: string;
  done?: (now: Settings, before: Settings) => string | null;
}

const steps: TourStep[] = [
  {
    page: 'appearance', target: 'styles', title: '挑一个候选窗口',
    body: () => <p>这八种风格就是你打字时候选窗口的样子。点一张卡片，上面的预览和正在用的输入法一起换，不用保存。</p>,
    tryIt: '点一张风格卡片试试',
    done: (now, before) => now.candidateStyle !== before.candidateStyle
      ? `已换成${candidateStyles.find(item => item.value === now.candidateStyle)?.name ?? '新风格'}` : null,
  },
  {
    page: 'input', target: 'shift', title: '中、英一键切换',
    body: (_, theme) => <>
      <p>打开后，单独按一下 <Keys keys={['Shift']} /> 就在中文和英文之间切换，任务栏上的图标跟着变；<Keys keys={['Ctrl', 'Space']} /> 一直可用。</p>
      <div className="tour-modes" role="img" aria-label="任务栏图标：单按 Shift 在“中”和“英”之间切换">
        <span className="tour-mode"><img src={shot(`mode-zh-${theme}`)} width={32} height={32} alt="" />中文</span>
        <Icon name="switch" size={16} />
        <span className="tour-mode"><img src={shot(`mode-en-${theme}`)} width={32} height={32} alt="" />英文</span>
        <span className="tour-modes-note">任务栏 · 实际图标</span>
      </div>
    </>,
    tryIt: '打开右边的开关',
    done: now => now.shiftToggleEnabled ? '已开启：单按 Shift 切换' : null,
  },
  {
    page: 'english', target: 'english-ime', title: '第二个输入法：本地英文',
    body: settings => {
      const keys = imeSwitchKeys.find(item => item.value === settings.imeSwitchKey)?.keys ?? [];
      return <p>专门打英文：整句输入，拼错的词当场改好，全部在本机完成。
        {keys.length ? <>按 <Keys keys={keys} /> 在本地拼音和本地英文之间切换，下面可以换成别的键。</> : <>用 <Keys keys={['Win', 'Space']} /> 切换。</>}</p>;
    },
    tryIt: '右边的开关把它加进或移出 Windows 的输入法列表',
    done: now => now.englishIme ? '已在输入法列表中' : null,
  },
  {
    page: 'panel', target: 'panel-tab', title: '连按两下 Tab：工具面板',
    body: () => <p>打字时快速按两次 <Keys keys={['Tab']} />，光标旁打开一个小面板：符号、表情、颜文字、剪贴板和计算。直接打拼音就能搜索。</p>,
    tryIt: '回到任意输入框，连按两下 Tab',
    done: now => now.panelDoubleTab ? '已开启' : null,
  },
  {
    target: 'search', title: '找设置，按 Ctrl + K',
    body: () => <p>搜索任何设置或操作。左边的每一页都有编号，<Keys keys={['Ctrl', '1']} /> 到 <Keys keys={['Ctrl', '7']} /> 直接跳过去。以后想再看一遍：关于 › 新手引导。</p>,
    tryIt: '按 Ctrl + K 试试，引导随之完成',
  },
];

export function Onboarding({ settings, theme, paletteOpen, go, notify, onClose }: {
  settings: Settings;
  theme: 'light' | 'dark';
  paletteOpen: boolean;
  go: (page: TourPage) => void;
  notify: (message: string) => void;
  onClose: () => void;
}) {
  const [step, setStep] = useState<number | null>(null);
  const finish = (completed: boolean) => {
    markTourSeen();
    if (completed) notify('引导完成。想再看一遍：关于 › 新手引导');
    onClose();
  };
  return step === null
    ? <Welcome settings={settings} theme={theme} onStart={() => setStep(0)} onSkip={() => finish(false)} />
    : <Spotlight index={step} settings={settings} theme={theme} paletteOpen={paletteOpen} go={go}
        onStep={setStep} onFinish={finish} />;
}

function Welcome({ settings, theme, onStart, onSkip }: {
  settings: Settings; theme: 'light' | 'dark'; onStart: () => void; onSkip: () => void;
}) {
  const style = settings.candidateStyle;
  const name = candidateStyles.find(item => item.value === style)?.name ?? '';
  return <Modal title="欢迎使用本地拼音" description="离线的中文拼音输入法，另附一个英文输入法“本地英文”。" onClose={onSkip} className="tour-welcome">
    <div className="modal-body">
      <figure className="tour-shot" data-theme-shot={theme}>
        <img src={shot(`cand-${style}-${theme}`)} alt="你的候选窗口：输入 ni'hao'shi'jie，候选是你好世界、你好、拟好……" />
        <figcaption>你的候选窗口 · {name} · 实际截图</figcaption>
      </figure>
      <ol className="tour-basics">
        <li><span className="tour-basic-number">1</span><span>按 <Keys keys={['Win', 'Space']} /> 选“本地拼音”，打拼音就会出现上面的窗口。</span></li>
        <li><span className="tour-basic-number">2</span><span><Keys keys={['Space']} /> 放进第一个，<Keys keys={['1', '9']} joiner="–" /> 选词，<Keys keys={['Enter']} /> 原样放进拼音。</span></li>
        <li><span className="tour-basic-number">3</span><span>全部在这台电脑上完成，打字时不联网。</span></li>
      </ol>
    </div>
    <footer className="modal-footer">
      <button className="button secondary" onClick={onSkip}>以后再说</button>
      <button className="button primary" data-autofocus onClick={onStart}>开始导览 · {steps.length} 步</button>
    </footer>
  </Modal>;
}

type Box = { x: number; y: number; w: number; h: number };
const holePadding = 8;
const holeRadius = 12;

// The scrim's outline with the target cut out (even-odd): only the hole lets
// the pointer through to the control under it.
function scrimPath(hole: Box | null, width: number, height: number) {
  const outer = `M0 0H${width}V${height}H0Z`;
  if (!hole) return `path(evenodd, '${outer}')`;
  const { x, y, w, h } = hole;
  const r = Math.min(holeRadius, w / 2, h / 2);
  return `path(evenodd, '${outer} M${x + r} ${y}H${x + w - r}A${r} ${r} 0 0 1 ${x + w} ${y + r}V${y + h - r}` +
    `A${r} ${r} 0 0 1 ${x + w - r} ${y + h}H${x + r}A${r} ${r} 0 0 1 ${x} ${y + h - r}V${y + r}A${r} ${r} 0 0 1 ${x + r} ${y}Z')`;
}

// Beside a target in the sidebar, else below it, else above it, else in the
// corner; always inside the window.
function placeCard(hole: Box | null, card: { w: number; h: number }, width: number, height: number) {
  const gap = 14;
  const margin = 16;
  // Clear of the window bar, which drags the window and holds its buttons.
  const ceiling = 48;
  let left = width - card.w - margin;
  let top = height - card.h - margin;
  if (hole) {
    const centerLeft = hole.x + hole.w / 2 - card.w / 2;
    if (hole.x + hole.w < width * 0.3 && hole.x + hole.w + gap + card.w + margin <= width) {
      left = hole.x + hole.w + gap;
      top = hole.y + hole.h / 2 - card.h / 2;
    } else if (hole.y + hole.h + gap + card.h + margin <= height) {
      left = centerLeft;
      top = hole.y + hole.h + gap;
    } else if (hole.y - gap - card.h >= ceiling) {
      left = centerLeft;
      top = hole.y - gap - card.h;
    } else {
      // No room beside it: the corner that covers the least of the target.
      const covered = (x: number, y: number) =>
        Math.max(0, Math.min(x + card.w, hole.x + hole.w) - Math.max(x, hole.x)) *
        Math.max(0, Math.min(y + card.h, hole.y + hole.h) - Math.max(y, hole.y));
      const corners = [[width - card.w - margin, height - card.h - margin], [width - card.w - margin, ceiling],
        [margin, height - card.h - margin], [margin, ceiling]];
      [left, top] = corners.reduce((best, corner) => covered(corner[0], corner[1]) < covered(best[0], best[1]) ? corner : best);
    }
  }
  return {
    left: Math.round(Math.max(margin, Math.min(left, width - card.w - margin))),
    top: Math.round(Math.max(ceiling, Math.min(top, height - card.h - margin))),
  };
}

function Spotlight({ index, settings, theme, paletteOpen, go, onStep, onFinish }: {
  index: number;
  settings: Settings;
  theme: 'light' | 'dark';
  paletteOpen: boolean;
  go: (page: TourPage) => void;
  onStep: (index: number) => void;
  onFinish: (completed: boolean) => void;
}) {
  const step = steps[index];
  const last = index === steps.length - 1;
  const [hole, setHole] = useState<Box | null>(null);
  const [viewport, setViewport] = useState({ width: window.innerWidth, height: window.innerHeight });
  const [position, setPosition] = useState<{ left: number; top: number } | null>(null);
  const card = useRef<HTMLDivElement>(null);
  const primary = useRef<HTMLButtonElement>(null);
  // The settings as the step found them, to tell what the user changed.
  const before = useRef(settings);
  const latest = useRef({ index, onStep, onFinish });
  latest.current = { index, onStep, onFinish };

  // Go to the step's page, bring its control into view and follow it.
  useLayoutEffect(() => {
    before.current = settings;
    setHole(null);
    if (step.page) go(step.page);
    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    const find = () => document.querySelector<HTMLElement>(`[data-tour="${step.target}"]`);
    let frame = 0;
    const measure = () => {
      setViewport({ width: window.innerWidth, height: window.innerHeight });
      const element = find();
      if (!element || !element.getClientRects().length) { setHole(null); return; }
      const rect = element.getBoundingClientRect();
      setHole({ x: rect.left - holePadding, y: rect.top - holePadding, w: rect.width + holePadding * 2, h: rect.height + holePadding * 2 });
    };
    const follow = () => { cancelAnimationFrame(frame); frame = requestAnimationFrame(measure); };
    frame = requestAnimationFrame(() => {
      find()?.scrollIntoView({ block: 'center', behavior: reduced ? 'auto' : 'smooth' });
      measure();
    });
    window.addEventListener('resize', follow);
    document.addEventListener('scroll', follow, true);
    const observer = new ResizeObserver(follow);
    const content = document.getElementById('app-content');
    if (content) observer.observe(content);
    const element = find();
    if (element) observer.observe(element);
    return () => {
      cancelAnimationFrame(frame);
      window.removeEventListener('resize', follow);
      document.removeEventListener('scroll', follow, true);
      observer.disconnect();
    };
    // Only a new step moves the spotlight; a settings change is the user trying it.
  }, [index]);

  useLayoutEffect(() => {
    const element = card.current;
    if (element) setPosition(placeCard(hole, { w: element.offsetWidth, h: element.offsetHeight }, viewport.width, viewport.height));
  }, [hole, viewport, index, settings]);

  useEffect(() => { primary.current?.focus({ preventScroll: true }); }, [index]);

  // Opening the palette on the last step is the step done.
  useEffect(() => { if (paletteOpen && last) latest.current.onFinish(true); }, [paletteOpen, last]);

  // ← → move between steps and 1–5 choose one, as on a candidate list; Esc
  // leaves. Keys typed into the control being shown stay that control's.
  useEffect(() => {
    const keydown = (event: KeyboardEvent) => {
      if (event.ctrlKey || event.altKey || event.metaKey || document.querySelector('.modal, .palette')) return;
      const focus = document.activeElement;
      const ours = !focus || focus === document.body || !!card.current?.contains(focus);
      const { index: current, onStep: move, onFinish: end } = latest.current;
      if (event.key === 'Escape') { event.preventDefault(); end(false); return; }
      if (!ours) return;
      const digit = Number(event.key);
      if (event.key === 'ArrowRight') { event.preventDefault(); if (current < steps.length - 1) move(current + 1); else end(true); }
      else if (event.key === 'ArrowLeft') { event.preventDefault(); if (current > 0) move(current - 1); }
      else if (Number.isInteger(digit) && digit >= 1 && digit <= steps.length) { event.preventDefault(); move(digit - 1); }
    };
    document.addEventListener('keydown', keydown);
    return () => document.removeEventListener('keydown', keydown);
  }, []);

  const done = step.done?.(settings, before.current) ?? null;
  const titleId = `tour-title-${index}`;
  return createPortal(<div className="tour" data-cand-style={settings.candidateStyle}>
    <div className="tour-scrim" aria-hidden="true" style={{ clipPath: scrimPath(hole, viewport.width, viewport.height) }} />
    {hole && <div className="tour-ring" aria-hidden="true" style={{ left: hole.x, top: hole.y, width: hole.w, height: hole.h }} />}
    <div ref={card} className="tour-card" role="dialog" aria-modal="false" aria-labelledby={titleId} aria-describedby={`${titleId}-body`}
      style={(position ? { left: position.left, top: position.top } : { visibility: 'hidden' }) as CSSProperties}>
      <div className="tour-card-head">
        <ol className="tour-progress" aria-label={`第 ${index + 1} 步，共 ${steps.length} 步`}>{steps.map((item, number) =>
          <li key={item.target}><button type="button" className={number === index ? 'current' : number < index ? 'seen' : undefined}
            aria-current={number === index ? 'step' : undefined} aria-label={`第 ${number + 1} 步：${item.title}`}
            onClick={() => onStep(number)}>{number + 1}</button></li>)}
        </ol>
        <button type="button" className="text-button tour-skip" onClick={() => onFinish(false)}>跳过</button>
      </div>
      <h2 id={titleId}>{step.title}</h2>
      <div className="tour-body" id={`${titleId}-body`}>{step.body(settings, theme)}</div>
      <p className={`tour-try${done ? ' done' : ''}`} role="status">
        <Icon name={done ? 'check' : 'arrow'} size={14} />{done ?? step.tryIt}
      </p>
      <footer className="tour-card-foot">
        {index > 0 ? <button type="button" className="button secondary" onClick={() => onStep(index - 1)}>上一步</button> : <span />}
        <button ref={primary} type="button" className="button primary" onClick={() => last ? onFinish(true) : onStep(index + 1)}>
          {last ? '完成' : '下一步'}
        </button>
      </footer>
    </div>
  </div>, document.body);
}
