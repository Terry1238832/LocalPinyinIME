import { Fragment, useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { BrandMark, Icon, type IconName } from './icons';
import { asError, CandidateStyleContext, ErrorNotice } from './components';
import { AppearancePage, InputPage, LearningPage, AboutPage } from './pages';
import { EnglishPage } from './EnglishPage';
import { DictionaryPage } from './dictionary';
import { ClipboardQuestion, PanelPage } from './PanelPage';
import { CommandPalette, requestPageCommand, type Command } from './CommandPalette';
import { Onboarding, tourSeen } from './Tour';
import type { AppState, Settings, Theme, UserEntry , UpdateStatus } from './types';

type PageId = 'appearance' | 'input' | 'english' | 'panel' | 'dictionary' | 'learning' | 'about';
// The sidebar is a candidate list: pages are numbered in this order (Ctrl+1–7
// choose them, as digits choose candidates) and grouped under a caption.
const navigation: { id: PageId; label: string; icon: IconName; keywords: string; group?: string }[] = [
  { id: 'appearance', label: '外观', icon: 'appearance', keywords: 'waiguan wg 主题 zhuti 字号 theme appearance', group: '输入法' },
  { id: 'input', label: '输入', icon: 'input', keywords: 'shuru sr 切换 联想 快捷键 input shortcut' },
  { id: 'english', label: '英文', icon: 'english', keywords: 'yingwen yw 本地英文 english 整句 拼写 纠错 capslock 快捷键' },
  { id: 'panel', label: '工具面板', icon: 'panel', keywords: 'gongju mianban gj 符号 表情 颜文字 剪贴板 jianqieban tab panel clipboard emoji', group: '收藏与记录' },
  { id: 'dictionary', label: '词库', icon: 'dictionary', keywords: 'ciku ck 个人词语 dictionary words' },
  { id: 'learning', label: '学习', icon: 'learning', keywords: 'xuexi xx 记录 learning history' },
  { id: 'about', label: '关于', icon: 'about', keywords: 'guanyu gy 版本 数据 about version' },
];

function BootScreen() {
  return <div className="boot" role="status" aria-label="正在打开设置">
    <div className="boot-side" aria-hidden="true">
      <div className="boot-brand"><span className="boot-mark" />本地拼音</div>
      <div className="boot-search" />
      <ol className="boot-nav">{[1, 2, 3, 4, 5, 6].map(number => <li key={number}>{number}<span /></li>)}</ol>
    </div>
    <div className="boot-main">
      <div className="boot-line" aria-hidden="true"><span className="boot-pinyin">she<i>'</i>zhi</span><span className="boot-caret" /></div>
      <div className="boot-cands" aria-hidden="true"><span><b>1</b>设置</span><span><b>2</b>摄制</span><span><b>3</b>社址</span></div>
      <p className="boot-note">正在读取设置和词库…</p>
    </div>
  </div>;
}

export default function App() {
  const [page, setPage] = useState<PageId>('appearance');
  const [state, setState] = useState<AppState | null>(() => window.localPinyin?.initialState ?? null);
  const [entries, setEntries] = useState<UserEntry[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [lexiconError, setLexiconError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);
  const [loading, setLoading] = useState(true);
  const [notice, setNotice] = useState<string | null>(null);
  const [palette, setPalette] = useState(false);
  // The clipboard question, once per launch until answered.
  const [askClipboard, setAskClipboard] = useState(true);
  // The first-run guide: once, after the clipboard question; again from 关于.
  const [tour, setTour] = useState(false);
  const tourChecked = useRef(false);
  const [systemDark, setSystemDark] = useState(() => window.matchMedia('(prefers-color-scheme: dark)').matches);
  const content = useRef<HTMLElement>(null);
  const nav = useRef<HTMLElement>(null);
  // Updates: the main process checks on its own and reports every change.
  const [update, setUpdate] = useState<UpdateStatus | null>(null);
  useEffect(() => {
    let alive = true;
    void window.localPinyin.getUpdateStatus().then(status => { if (alive) setUpdate(status); }).catch(() => undefined);
    const stop = window.localPinyin.onUpdateStatus(status => setUpdate(status));
    return () => { alive = false; stop(); };
  }, []);
  const updateWaiting = update?.state === 'available' || update?.state === 'downloading' || update?.state === 'ready';
  const [indicator, setIndicator] = useState<{ top: number; height: number; labelLeft: number; labelWidth: number } | null>(null);
  const savePending = useRef(false);

  const refresh = async () => {
    setLoading(true);
    setError(null);
    try {
      if (!window.localPinyin) throw new Error('无法连接本地输入引擎，请关闭设置后重新打开。');
      const snapshot = await window.localPinyin.getState();
      setState(snapshot);
      try {
        const result = await window.localPinyin.listUserLexicon();
        setEntries(result.entries);
        setLexiconError(null);
      } catch (failure) { setLexiconError(asError(failure)); }
    } catch (failure) { setError(asError(failure)); }
    finally { setLoading(false); }
  };

  useEffect(() => { void refresh(); }, []);
  // The panel answered the clipboard question.
  useEffect(() => window.localPinyin.onSettingsChanged(() => {
    void window.localPinyin.getState().then(setState).catch(() => undefined);
  }), []);
  useEffect(() => {
    const media = window.matchMedia('(prefers-color-scheme: dark)');
    const update = () => setSystemDark(media.matches);
    media.addEventListener('change', update);
    return () => media.removeEventListener('change', update);
  }, []);
  const theme = state?.settings.theme ?? 'system';
  const resolvedTheme = theme === 'system' ? (systemDark ? 'dark' : 'light') : theme;
  useLayoutEffect(() => { document.documentElement.dataset.theme = resolvedTheme; }, [resolvedTheme]);
  useEffect(() => {
    if (!notice) return;
    const timeout = window.setTimeout(() => setNotice(null), 3200);
    return () => window.clearTimeout(timeout);
  }, [notice]);

  // The current page's mark slides between navigation items; styles that
  // mark the word itself (a rule, a highlighter stroke) need where it is.
  useLayoutEffect(() => {
    const measure = () => {
      const selected = nav.current?.querySelector<HTMLElement>('[aria-current="page"]');
      const label = selected?.querySelector<HTMLElement>('.nav-label');
      if (selected) setIndicator({ top: selected.offsetTop, height: selected.offsetHeight,
        labelLeft: label?.offsetLeft ?? 0, labelWidth: label ? Math.min(label.scrollWidth, label.offsetWidth) : 0 });
    };
    measure();
    window.addEventListener('resize', measure);
    return () => window.removeEventListener('resize', measure);
  }, [page, state !== null]);

  const go = (id: PageId) => { setPage(id); content.current?.scrollTo({ top: 0 }); };
  useEffect(() => {
    if (!state || tourChecked.current || (state.settings.clipboardHistory === 'ask' && askClipboard)) return;
    tourChecked.current = true;
    if (!tourSeen()) setTour(true);
  }, [state, askClipboard]);

  // Ctrl+K opens the command palette; Ctrl+1–7 switch pages.
  useEffect(() => {
    const keydown = (event: KeyboardEvent) => {
      if (!event.ctrlKey || event.altKey || event.metaKey || event.shiftKey) return;
      if (event.key.toLowerCase() === 'k') {
        event.preventDefault();
        // Never stack the palette over a confirmation or editor dialog.
        if (!document.querySelector('.modal')) setPalette(open => !open);
        return;
      }
      const index = Number(event.key) - 1;
      if (Number.isInteger(index) && index >= 0 && index < navigation.length && !document.querySelector('[role="dialog"]')) {
        event.preventDefault();
        go(navigation[index].id);
      }
    };
    window.addEventListener('keydown', keydown);
    return () => window.removeEventListener('keydown', keydown);
  }, []);

  const saveSettings = async (next: Settings) => {
    if (savePending.current || !state) return;
    savePending.current = true;
    setSaving(true);
    setError(null);
    try { setState(await window.localPinyin.saveSettings(next)); }
    catch (failure) { setError(asError(failure)); }
    finally { savePending.current = false; setSaving(false); }
  };
  const updateEntries = (next: UserEntry[]) => {
    setEntries(next);
    setState(previous => previous ? { ...previous, userLexiconCount: next.length } : previous);
    setLexiconError(null);
  };
  const controlWindow = async (action: 'minimize' | 'maximize' | 'close') => {
    try { await window.localPinyin.windowControl(action); }
    catch (failure) { setError(asError(failure)); }
  };

  const commands = useMemo<Command[]>(() => {
    if (!state) return [];
    const settings = state.settings;
    const themeCommand = (value: Theme, title: string, icon: IconName, keywords: string): Command => ({
      id: `theme-${value}`, group: '设置', icon, title, keywords, hint: settings.theme === value ? '当前' : undefined,
      run: () => void saveSettings({ ...settings, theme: value }),
    });
    return [
      ...navigation.map((item, index): Command => ({ id: `page-${item.id}`, group: '页面', icon: item.icon, title: item.label,
        keywords: item.keywords, keys: ['Ctrl', String(index + 1)], run: () => go(item.id) })),
      themeCommand('light', '使用浅色主题', 'sun', 'qianse theme light 主题'),
      themeCommand('dark', '使用深色主题', 'moon', 'shense theme dark 主题'),
      themeCommand('system', '主题跟随系统', 'monitor', 'gensui xitong theme system 主题'),
      { id: 'toggle-shift', group: '设置', icon: 'switch', title: `${settings.shiftToggleEnabled ? '关闭' : '开启'}“按 Shift 切换中英文”`,
        keywords: 'shift qiehuan zhongyingwen 切换', run: () => void saveSettings({ ...settings, shiftToggleEnabled: !settings.shiftToggleEnabled }) },
      { id: 'toggle-prediction', group: '设置', icon: 'spark', title: `${settings.predictionEnabled ? '关闭' : '开启'}“联想下一个词”`,
        keywords: 'lianxiang prediction 联想', run: () => void saveSettings({ ...settings, predictionEnabled: !settings.predictionEnabled }) },
      { id: 'toggle-translate', group: '设置', icon: 'spark', title: `${settings.tabTranslate ? '关闭' : '开启'}“Tab 键翻译成英文”`,
        keywords: 'fanyi yingwen translate english tab 翻译', run: () => void saveSettings({ ...settings, tabTranslate: !settings.tabTranslate }) },
      { id: 'toggle-english', group: '设置', icon: 'english', title: `${settings.englishIme ? '关闭' : '开启'}本地英文输入法`,
        keywords: 'bendi yingwen english ime 本地英文 输入法', run: () => void saveSettings({ ...settings, englishIme: !settings.englishIme }) },
      { id: 'toggle-english-sentence', group: '设置', icon: 'english', title: `${settings.englishSentence ? '关闭' : '开启'}英文“整句输入”`,
        keywords: 'zhengju shuru english sentence 整句', run: () => void saveSettings({ ...settings, englishSentence: !settings.englishSentence }) },
      { id: 'toggle-panel', group: '设置', icon: 'panel', title: `${settings.panelDoubleTab ? '关闭' : '开启'}“连按两下 Tab 打开工具面板”`,
        keywords: 'gongju mianban tab panel 工具面板', run: () => void saveSettings({ ...settings, panelDoubleTab: !settings.panelDoubleTab }) },
      { id: 'toggle-clipboard', group: '设置', icon: 'clipboard', title: `${settings.clipboardHistory === 'on' ? '关闭' : '开启'}剪贴板记录`,
        keywords: 'jianqieban clipboard history 剪贴板', run: () => void saveSettings({ ...settings, clipboardHistory: settings.clipboardHistory === 'on' ? 'off' : 'on' }) },
      { id: 'add-word', group: '操作', icon: 'plus', title: '添加个人词语…', keywords: 'tianjia ciyu add word 词库',
        run: () => { go('dictionary'); requestPageCommand('add-word'); } },
      { id: 'browse', group: '操作', icon: 'search', title: '查找内置词库…', keywords: 'chazhao neizhi ciku search dictionary',
        run: () => { go('dictionary'); requestPageCommand('browse-dictionary'); } },
      { id: 'open-data', group: '操作', icon: 'folder', title: '打开数据文件夹', keywords: 'dakai shuju wenjianjia data folder',
        run: () => requestPageCommand('open-data') },
      { id: 'clear-learning', group: '操作', icon: 'reset', title: '清除学习记录…', keywords: 'qingchu xuexi clear learning',
        run: () => { go('learning'); requestPageCommand('clear-learning'); } },
      { id: 'tour', group: '操作', icon: 'spark', title: '重新观看新手引导', keywords: 'xinshou yindao daolan bangzhu tour guide help 引导 帮助',
        run: () => setTour(true) },
    ];
  }, [state]);

  const runtimeActive = !!state && state.runtime.available && state.runtime.active;
  const runtimeLabel = !state ? '正在连接' : runtimeActive ? (state.runtime.mode.toLowerCase() === 'english' ? '正在输入 · 英文' : '正在输入 · 中文') : '输入法未在使用';

  const english = !!state && state.runtime.mode.toLowerCase() === 'english';
  // Until the settings arrive: the same loading screen index.html paints
  // with the window, so taking over from it does not flicker.
  if (!state && loading) return <BootScreen />;
  return <CandidateStyleContext.Provider value={state?.settings.candidateStyle}><div className="app-shell">
    <a className="skip-link" href="#app-content" onClick={event => { event.preventDefault(); content.current?.focus(); }}>跳到内容</a>
    <aside className="sidebar" id="app-sidebar">
      <div className="sidebar-head"><BrandMark size={26} /><div><strong>本地拼音</strong><span>Local Pinyin</span></div></div>
      <button className="command-trigger" data-tour="search" onClick={() => setPalette(true)} disabled={!state}>
        <Icon name="search" size={15} /><span>搜索设置</span><span className="command-hint">Ctrl K</span>
      </button>
      <nav aria-label="设置导航" ref={nav} data-cand-style={state?.settings.candidateStyle ?? 'flow'}>
        {indicator && <span className="nav-indicator" aria-hidden="true" style={{ transform: `translateY(${indicator.top}px)`, height: indicator.height,
          '--label-left': `${indicator.labelLeft}px`, '--label-width': `${indicator.labelWidth}px` } as React.CSSProperties} />}
        {navigation.map((item, index) => <Fragment key={item.id}>
          {item.group && <span className="nav-group" aria-hidden="true">{item.group}</span>}
          <button className={`nav-item${page === item.id ? ' selected' : ''}${item.id === 'about' ? ' nav-about' : ''}`} aria-current={page === item.id ? 'page' : undefined}
            data-number={index + 1} title={`Ctrl+${index + 1}`} onClick={() => go(item.id)}>
            <span className="nav-label">{item.label}</span>
            {item.id === 'about' && updateWaiting && <span className="nav-badge">新版本</span>}
          </button>
        </Fragment>)}
      </nav>
      <div className="sidebar-footer">
        <div className={`runtime-status${runtimeActive ? ' active' : ''}`}>
          <span className="mode-tile" aria-hidden="true">{runtimeActive && english ? '英' : '中'}</span>
          <span className="runtime-copy">
            <span className="runtime-label">{runtimeLabel}</span>
            <span className="sidebar-version">{state ? `v${state.version} · 离线输入` : 'Local Pinyin'}</span>
          </span>
        </div>
      </div>
    </aside>
    <div className="main-column">
      <div className="window-bar">
        <div className="window-controls">
          <button aria-label="最小化" onClick={() => void controlWindow('minimize')}><Icon name="minimize" size={15} /></button>
          <button aria-label="最大化或还原" onClick={() => void controlWindow('maximize')}><Icon name="maximize" size={13} /></button>
          <button className="window-close" aria-label="关闭窗口" onClick={() => void controlWindow('close')}><Icon name="close" size={15} /></button>
        </div>
      </div>
      <main className="main-content" ref={content} id="app-content" tabIndex={-1} aria-busy={loading}>
        {!state ? <div className="connection-state"><span className="connection-icon"><Icon name="keyboard" size={26} /></span><h1>{loading ? '正在打开设置' : '暂时无法打开设置'}</h1><p>{loading ? '正在读取本机设置与词库。' : '本地输入引擎没有响应，请重试。'}</p><ErrorNotice message={error} />{!loading && <button className="button primary" onClick={() => void refresh()}>重新连接</button>}</div> : <div className="page-frame">
          <div className="global-message"><ErrorNotice message={error} /></div>
          <section id="page-appearance" className="page" hidden={page !== 'appearance'}><AppearancePage state={state} saving={saving} saveSettings={saveSettings} /></section>
          <section id="page-input" className="page" hidden={page !== 'input'}><InputPage state={state} saving={saving} saveSettings={saveSettings} /></section>
          <section id="page-english" className="page" hidden={page !== 'english'}><EnglishPage state={state} saving={saving} saveSettings={saveSettings} /></section>
          <section id="page-panel" className="page" hidden={page !== 'panel'}><PanelPage state={state} saving={saving} saveSettings={saveSettings} notify={setNotice} /></section>
          <section id="page-dictionary" className="page" hidden={page !== 'dictionary'}><DictionaryPage state={state} entries={entries} onEntries={updateEntries} loadError={lexiconError} notify={setNotice} /></section>
          <section id="page-learning" className="page" hidden={page !== 'learning'}><LearningPage state={state} onState={setState} notify={setNotice} /></section>
          <section id="page-about" className="page" hidden={page !== 'about'}><AboutPage state={state} update={update} notify={setNotice} onTour={() => setTour(true)} /></section>
        </div>}
      </main>
    </div>
    {palette && state && <CommandPalette commands={commands} onClose={() => setPalette(false)} />}
    {state && state.settings.clipboardHistory === 'ask' && askClipboard && !palette &&
      <ClipboardQuestion settings={state.settings} saveSettings={saveSettings} onClose={() => setAskClipboard(false)} />}
    {tour && state && <Onboarding settings={state.settings} theme={resolvedTheme} paletteOpen={palette} go={go}
      notify={setNotice} onClose={() => setTour(false)} />}
    {notice && <div className="toast" role="status"><Icon name="check" size={15} />{notice}</div>}
  </div></CandidateStyleContext.Provider>;
}
