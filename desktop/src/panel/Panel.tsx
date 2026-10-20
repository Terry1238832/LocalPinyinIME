// The tool panel the input method opens with Tab twice. Pages down the side;
// symbols, kaomoji and emoji collect in a tray at the bottom, so several can
// be picked in a row, and go in together with 确定. 添加短语 ties what is in
// the tray to words: typing one of them later offers it first.
import { useCallback, useEffect, useRef, useState, type DragEvent } from 'react';
import { Icon, type IconName } from '../icons';
import { Browse } from './Browse';
import { emojiNames, loadCatalog, type Item } from './catalog';
import { Clipboard } from './Clipboard';
import { Records } from './Records';
import { Stickers } from './Stickers';
import { Tools } from './Tools';
import type { PanelData, PanelKind, Phrase, Pick } from './types';

type Page = 'tools' | 'clipboard' | PanelKind | 'stickers' | 'records';
const PAGES: { id: Page; label: string; icon: IconName; search?: string }[] = [
  { id: 'tools', label: '工具', icon: 'tools' },
  { id: 'clipboard', label: '剪贴板', icon: 'clipboard', search: '搜索复制过的文字，也能搜图片里的字' },
  { id: 'symbol', label: '符号', icon: 'omega', search: '搜索符号，如 约等于、箭头、美元' },
  { id: 'kaomoji', label: '颜文字', icon: 'kaomoji', search: '搜索颜文字，如 开心、生气、猫' },
  { id: 'emoji', label: '表情', icon: 'smile', search: '搜索表情，如 笑、爱心、猫' },
  { id: 'stickers', label: '表情包', icon: 'sticker', search: '按名字搜索表情包' },
  { id: 'records', label: '记录', icon: 'lock', search: '搜索记录的标题或内容' },
];
const TRAY_LIMIT = 64;
const api = () => window.localPinyinPanel;
const message = (error: unknown) => error instanceof Error ? error.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, '') : String(error);

function storedPage(): Page {
  try {
    const value = localStorage.getItem('panel.page');
    if (PAGES.some(page => page.id === value)) return value as Page;
  } catch { /* storage unavailable */ }
  return 'symbol';
}

type TrayItem = Pick & { item: Item };

export default function Panel() {
  const [page, setPage] = useState<Page>(storedPage);
  const [data, setData] = useState<PanelData | null>(null);
  const [tray, setTray] = useState<TrayItem[]>([]);
  const [search, setSearch] = useState('');
  const [pinned, setPinned] = useState(false);
  const [toast, setToast] = useState<{ text: string; error: boolean } | null>(null);
  const [phrasing, setPhrasing] = useState(false);
  const [opened, setOpened] = useState(0);
  const searchField = useRef<HTMLInputElement>(null);
  const toastTimer = useRef(0);

  const notify = useCallback((text: string, error = false) => {
    setToast({ text, error });
    window.clearTimeout(toastTimer.current);
    toastTimer.current = window.setTimeout(() => setToast(null), error ? 4200 : 2200);
  }, []);
  const fail = useCallback((error: unknown) => notify(message(error), true), [notify]);
  // Opened from the small panel's ⤢: on the page it was showing.
  const reload = useCallback(() => {
    api().data().then(next => {
      setData(next);
      if (next.page) setPage(next.page);
    }).catch(fail);
  }, [fail]);

  useEffect(() => {
    reload();
    return api().onOpen(() => { reload(); setOpened(value => value + 1); });
  }, [reload]);
  useEffect(() => {
    try { localStorage.setItem('panel.page', page); } catch { /* storage unavailable */ }
    setSearch('');
  }, [page]);
  useEffect(() => { void api().keepOpen(pinned || page === 'stickers'); }, [pinned, page]);
  // Each opening starts from the search box (or the tool field).
  useEffect(() => { if (page !== 'tools') searchField.current?.focus(); }, [opened, page]);

  const text = tray.map(entry => entry.value).join('');
  const confirm = useCallback(async (value = text, picks: Pick[] = tray.map(({ kind, value: item }) => ({ kind, value: item }))) => {
    if (!value) return;
    try {
      // Emoji confirmed here are offered among candidates for their names.
      for (const entry of tray) if (entry.kind === 'emoji' && value === text) void api().learnEmoji(entry.value, emojiNames(entry.item));
      await api().insert(value, picks);
      setTray([]);
      reload();
    } catch (error) { fail(error); }
  }, [text, tray, reload, fail]);

  const pick = (kind: PanelKind) => (item: Item) => {
    if (tray.length >= TRAY_LIMIT) { notify(`一次最多选 ${TRAY_LIMIT} 个`, true); return; }
    setTray(list => [...list, { kind, value: item.c, item }]);
  };
  const favorite = (kind: PanelKind) => (item: Item, on: boolean) => {
    api().favorite(kind, item.c, on).then(state => {
      setData(current => current && { ...current, state });
      notify(on ? `已收藏 ${item.c}` : `已取消收藏 ${item.c}`);
    }).catch(fail);
  };

  useEffect(() => {
    const keys = (event: KeyboardEvent) => {
      const typing = event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement ||
        event.target instanceof HTMLSelectElement;
      if (event.key === 'Escape') {
        if (phrasing) { setPhrasing(false); return; }
        if (search && typing) { setSearch(''); return; }
        void api().close();
      } else if (event.key === 'Enter' && event.ctrlKey && tray.length) {
        event.preventDefault();
        void confirm();
      } else if (event.key === 'Backspace' && !typing && tray.length) {
        event.preventDefault();
        setTray(list => list.slice(0, -1));
      } else if (event.key === 'Tab' && event.ctrlKey) {
        event.preventDefault();
        const index = PAGES.findIndex(entry => entry.id === page);
        setPage(PAGES[(index + (event.shiftKey ? PAGES.length - 1 : 1)) % PAGES.length].id);
      }
    };
    window.addEventListener('keydown', keys);
    return () => window.removeEventListener('keydown', keys);
  }, [phrasing, search, tray, page, confirm]);

  const current = PAGES.find(entry => entry.id === page)!;
  const browsing = page === 'symbol' || page === 'kaomoji' || page === 'emoji';
  // A picture dragged in over any page goes to 表情包.
  const dragIn = (event: DragEvent) => {
    const types = [...event.dataTransfer.types];
    if (page !== 'stickers' && (types.includes('Files') || types.includes('text/uri-list'))) setPage('stickers');
  };

  return (
    <div className="panel" onDragEnter={dragIn}>
      <nav className="rail" aria-label="面板">
        {PAGES.map(entry => (
          <button key={entry.id} type="button" className={entry.id === page ? 'rail-item active' : 'rail-item'}
                  aria-current={entry.id === page ? 'page' : undefined} onClick={() => setPage(entry.id)}>
            <Icon name={entry.icon} size={20} />
            <span>{entry.label}</span>
          </button>
        ))}
        <span className="rail-fill" />
        <button type="button" className="rail-item small" title="打开设置" onClick={() => void api().openSettings()}>
          <Icon name="gear" size={18} /><span>设置</span>
        </button>
      </nav>
      <main className="body">
        <header className="topbar">
          {current.search ? (
            <label className="search">
              <Icon name="search" size={16} />
              <input ref={searchField} value={search} placeholder={current.search} aria-label={current.search} spellCheck={false}
                     maxLength={100} onChange={event => setSearch(event.target.value)} />
              {search && <button type="button" className="icon-button" aria-label="清空搜索" onClick={() => { setSearch(''); searchField.current?.focus(); }}>
                <Icon name="close" size={13} /></button>}
            </label>
          ) : <h1 className="topbar-title">{current.label}</h1>}
          <span className="drag-space" />
          <button type="button" className={pinned ? 'icon-button toggled' : 'icon-button'} aria-pressed={pinned}
                  title={pinned ? '取消固定：输入后收起' : '固定面板：输入后也不收起'} onClick={() => setPinned(value => !value)}>
            <Icon name="pin" size={16} /></button>
          <button type="button" className="icon-button" title="关闭（Esc）" aria-label="关闭" onClick={() => void api().close()}>
            <Icon name="close" size={16} /></button>
        </header>
        <div className="content">
          {page === 'tools' && <Tools focusKey={opened} onInsert={value => void confirm(value, [])} />}
          {page === 'clipboard' && <Clipboard search={search} refreshKey={opened} onError={message => notify(message, true)} onNotice={notify} />}
          {browsing && data && <Browse kind={page} state={data.state} search={search} onPick={pick(page)} onFavorite={favorite(page)} />}
          {page === 'records' && <Records search={search} onError={fail} onNotice={notify} />}
          {page === 'stickers' && data && <Stickers stickers={data.stickers} search={search}
            onChange={stickers => setData(current => current && { ...current, stickers })} onError={message => notify(message, true)} onNotice={notify} />}
        </div>
        {(browsing || tray.length > 0) && (
          <footer className="tray-bar">
            <div className="tray" aria-label="已选内容" aria-live="polite">
              {tray.length ? tray.map((entry, index) => (
                <button key={index} type="button" className={`tray-item ${entry.kind}`} title="点一下移除"
                        onClick={() => setTray(list => list.filter((_, at) => at !== index))}>{entry.value}</button>
              )) : <span className="tray-hint">点选{current.label}放到这里，可以连续选几个；右键收藏</span>}
            </div>
            {tray.length > 0 && <button type="button" className="text-button" onClick={() => setTray([])}>清空</button>}
            <button type="button" className="button secondary" disabled={!tray.length || text.length > 64}
                    title={text.length > 64 ? '短语内容最多 64 个字符' : '输入几个词，以后打这些词时它排第一'}
                    onClick={() => setPhrasing(true)}>添加短语</button>
            <button type="button" className="button primary" disabled={!tray.length} title="输入到原来的位置（Ctrl+Enter）"
                    onClick={() => void confirm()}>确定</button>
          </footer>
        )}
      </main>
      {phrasing && <PhraseDialog text={text} onClose={() => setPhrasing(false)} onSaved={(labels) => {
        setPhrasing(false);
        notify(`以后输入“${labels.join('”“')}”时，${text} 会排在第一`);
      }} onError={fail} />}
      {toast && <div className={toast.error ? 'toast error' : 'toast'} role="status">{toast.text}</div>}
    </div>
  );
}

// Several words for one text, as chips: typing a word and pressing Enter
// (or a comma or space) adds it.
function PhraseDialog({ text, onClose, onSaved, onError }: {
  text: string; onClose: () => void; onSaved: (labels: string[]) => void; onError: (error: unknown) => void;
}) {
  const [labels, setLabels] = useState<string[]>([]);
  const [draft, setDraft] = useState('');
  const [existing, setExisting] = useState<Phrase[]>([]);
  const [busy, setBusy] = useState(false);
  const field = useRef<HTMLInputElement>(null);
  // English words only: Chinese ones are typed as pinyin already.
  const valid = (value: string) => /^[A-Za-z]{1,32}$/.test(value);

  useEffect(() => {
    field.current?.focus();
    api().phrases().then(result => setExisting(result.phrases.filter(phrase => phrase.text === text))).catch(onError);
  }, [text, onError]);

  const commitDraft = () => {
    const words = draft.split(/[,，、\s]+/).map(word => word.trim()).filter(Boolean);
    const bad = words.find(word => !valid(word));
    if (bad) { onError(new Error(`“${bad}”不行：短语只能用英文字母，最多 32 个`)); return labels; }
    const next = [...new Set([...labels, ...words])].filter(word => !existing.some(phrase => phrase.label === word)).slice(0, 10);
    setLabels(next);
    setDraft('');
    return next;
  };
  const save = async () => {
    const all = draft.trim() ? commitDraft() : labels;
    if (!all.length || (draft.trim() && all === labels)) { if (!all.length) field.current?.focus(); return; }
    setBusy(true);
    try { await api().addPhrase(text, all); onSaved(all); }
    catch (error) { onError(error); setBusy(false); }
  };
  const remove = (phrase: Phrase) => {
    api().deletePhrase(phrase.pinyin, phrase.text).then(result => setExisting(result.phrases.filter(item => item.text === text))).catch(onError);
  };

  return (
    <div className="dialog-scrim" onMouseDown={event => { if (event.target === event.currentTarget) onClose(); }}>
      <div className="dialog" role="dialog" aria-modal="true" aria-labelledby="phrase-title">
        <h2 id="phrase-title">添加短语</h2>
        <p className="dialog-text">以后打这些英文，<span className="phrase-target">{text}</span> 会是第一个候选；
          选了别的，它也只退到第二位。短语只能用英文字母，可以在设置的“输入习惯”里删掉。</p>
        <div className="chip-field" onClick={() => field.current?.focus()}>
          {labels.map(label => (
            <span key={label} className="chip">{label}
              <button type="button" aria-label={`去掉 ${label}`} onClick={() => setLabels(list => list.filter(item => item !== label))}>
                <Icon name="close" size={11} /></button></span>
          ))}
          <input ref={field} value={draft} placeholder={labels.length ? '再加一个' : '例如 dollar，回车添加'} aria-label="短语"
                 maxLength={40} onChange={event => setDraft(event.target.value)}
                 onKeyDown={event => {
                   if ((event.key === 'Enter' || event.key === ',' || event.key === '，') && !event.nativeEvent.isComposing) {
                     event.preventDefault();
                     if (event.key === 'Enter' && !draft.trim() && labels.length) void save(); else commitDraft();
                   } else if (event.key === 'Backspace' && !draft && labels.length) setLabels(list => list.slice(0, -1));
                   else if (event.key === 'Escape') { event.stopPropagation(); onClose(); }
                 }} />
        </div>
        {existing.length > 0 && (
          <div className="existing">
            <span>已有的短语</span>
            {existing.map(phrase => (
              <span key={phrase.pinyin + phrase.label} className="chip muted" title={phrase.pinyin}>{phrase.label}
                <button type="button" aria-label={`删除短语 ${phrase.label}`} onClick={() => remove(phrase)}><Icon name="close" size={11} /></button></span>
            ))}
          </div>
        )}
        <div className="dialog-actions">
          <button type="button" className="button secondary" onClick={onClose}>取消</button>
          <button type="button" className="button primary" disabled={busy || (!labels.length && !draft.trim())} onClick={() => void save()}>
            {busy ? '正在保存…' : '保存'}</button>
        </div>
      </div>
    </div>
  );
}

// Loading the lists early keeps the first switch to a page quick.
void loadCatalog('symbol');
