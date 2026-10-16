import { useEffect, useLayoutEffect, useRef, useState } from 'react';
import { BrandMark, Icon, type IconName } from './icons';
import { asError, ErrorNotice } from './components';
import { AppearancePage, InputPage, LearningPage, AboutPage } from './pages';
import { DictionaryPage } from './dictionary';
import type { AppState, Settings, UserEntry } from './types';

type PageId = 'appearance' | 'input' | 'dictionary' | 'learning' | 'about';
const navigation: { id: PageId; label: string; icon: IconName }[] = [
  { id: 'appearance', label: '外观', icon: 'appearance' },
  { id: 'input', label: '输入', icon: 'input' },
  { id: 'dictionary', label: '词库', icon: 'dictionary' },
  { id: 'learning', label: '学习', icon: 'learning' },
  { id: 'about', label: '关于', icon: 'about' },
];

export default function App() {
  const [page, setPage] = useState<PageId>('appearance');
  const [state, setState] = useState<AppState | null>(() => window.localPinyin?.initialState ?? null);
  const [entries, setEntries] = useState<UserEntry[]>([]);
  const [lexiconRevision, setLexiconRevision] = useState(0);
  const [error, setError] = useState<string | null>(null);
  const [lexiconError, setLexiconError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);
  const [loading, setLoading] = useState(true);
  const [notice, setNotice] = useState<string | null>(null);
  const [systemDark, setSystemDark] = useState(() => window.matchMedia('(prefers-color-scheme: dark)').matches);
  const content = useRef<HTMLElement>(null);
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
    setLexiconRevision(previous => previous + 1);
    setState(previous => previous ? { ...previous, userLexiconCount: next.length } : previous);
    setLexiconError(null);
  };
  const controlWindow = async (action: 'minimize' | 'maximize' | 'close') => {
    try { await window.localPinyin.windowControl(action); }
    catch (failure) { setError(asError(failure)); }
  };

  return <div className="app-shell">
    <header className="titlebar">
      <span className="titlebar-caption"><BrandMark size={16} />本地拼音 · 设置</span>
      <div className="window-controls">
        <button aria-label="最小化" onClick={() => void controlWindow('minimize')}><Icon name="minimize" size={16} /></button>
        <button aria-label="最大化或还原" onClick={() => void controlWindow('maximize')}><Icon name="maximize" size={14} /></button>
        <button className="window-close" aria-label="关闭窗口" onClick={() => void controlWindow('close')}><Icon name="close" size={16} /></button>
      </div>
    </header>
    <div className="workspace">
      <aside className="sidebar" id="app-sidebar">
        <div className="brand"><BrandMark size={40} /><div><strong>本地拼音</strong><span>Local Pinyin</span></div></div>
        <nav aria-label="设置导航">{navigation.map(item =>
          <button key={item.id} className={`nav-item${page === item.id ? ' selected' : ''}`} aria-current={page === item.id ? 'page' : undefined}
            onClick={() => { setPage(item.id); content.current?.scrollTo({ top: 0 }); }}>
            <Icon name={item.icon} size={18} /><span>{item.label}</span>
          </button>)}
        </nav>
        <div className="sidebar-bottom">
          <div className="offline-pill"><span className="status-dot" />离线运行</div>
          <span className="sidebar-version">{state ? `版本 ${state.version}` : 'Local Pinyin'}</span>
        </div>
      </aside>
      <main className="main-content" ref={content} id="app-content" aria-busy={loading}>
        {!state ? <div className="connection-state"><span className="connection-icon"><Icon name="keyboard" size={28} /></span><h1>{loading ? '正在打开设置' : '暂时无法打开设置'}</h1><p>{loading ? '正在读取本机设置与词库。' : '本地输入引擎没有响应，请重试。'}</p><ErrorNotice message={error} />{!loading && <button className="button primary" onClick={() => void refresh()}>重新连接</button>}</div> : <div className="page-frame">
          <div className="global-message"><ErrorNotice message={error} /></div>
          <section id="page-appearance" className="page" hidden={page !== 'appearance'}><AppearancePage state={state} saving={saving} saveSettings={saveSettings} lookupRevision={lexiconRevision} /></section>
          <section id="page-input" className="page" hidden={page !== 'input'}><InputPage state={state} saving={saving} saveSettings={saveSettings} lookupRevision={lexiconRevision} /></section>
          <section id="page-dictionary" className="page" hidden={page !== 'dictionary'}><DictionaryPage state={state} entries={entries} onEntries={updateEntries} loadError={lexiconError} notify={setNotice} /></section>
          <section id="page-learning" className="page" hidden={page !== 'learning'}><LearningPage state={state} onState={setState} notify={setNotice} /></section>
          <section id="page-about" className="page" hidden={page !== 'about'}><AboutPage state={state} /></section>
        </div>}
      </main>
    </div>
    {notice && <div className="toast" role="status"><Icon name="check" size={16} />{notice}</div>}
  </div>;
}
