import { useEffect, useRef, useState } from 'react';
import { PagedCandidates } from './PagedCandidates';
import { BrandMark, Icon, type IconName } from './icons';
import { asError, ErrorNotice, formatNumber, Modal, PageHeader, SettingRow, SettingsGroup, Switch } from './components';
import type { AppState, Settings, Theme } from './types';

const fontSizes = [14, 16, 17, 18, 20, 22, 24];
type SettingsPageProps = { state: AppState; saving: boolean; saveSettings: (settings: Settings) => Promise<void> };

const themes: { value: Theme; label: string; icon: IconName }[] = [
  { value: 'light', label: '浅色', icon: 'sun' },
  { value: 'dark', label: '深色', icon: 'moon' },
  { value: 'system', label: '跟随系统', icon: 'monitor' },
];

const autoSave = <span className="save-status"><Icon name="check" size={14} />自动保存</span>;

function ThemeThumbnail({ theme }: { theme: Theme }) {
  // A miniature of the candidate popup in each theme.
  const popup = (tone: 'light' | 'dark') => <span className={`thumb-popup thumb-${tone}`}><span className="thumb-pinyin" /><span className="thumb-row"><span className="thumb-selected" /><span className="thumb-word" /><span className="thumb-word short" /></span></span>;
  // "System" shows one popup split down the middle: light left, dark right.
  return <span className={`theme-thumb thumb-bg-${theme}`} aria-hidden="true">
    {theme === 'system' ? <><span className="thumb-layer">{popup('light')}</span><span className="thumb-layer thumb-dark-half">{popup('dark')}</span></> : popup(theme)}
  </span>;
}

export function AppearancePage({ state, saving, saveSettings, lookupRevision }: SettingsPageProps & { lookupRevision: number }) {
  const [size, setSize] = useState(state.settings.textSize);
  const [pinyin, setPinyin] = useState('suibian');
  const [candidates, setCandidates] = useState<string[]>([]);
  const [pending, setPending] = useState(false);
  const [lookupError, setLookupError] = useState<string | null>(null);
  const request = useRef(0);
  useEffect(() => { if (!saving) setSize(state.settings.textSize); }, [state.settings.textSize, saving]);
  useEffect(() => {
    const id = ++request.current;
    setLookupError(null);
    if (!pinyin.trim()) { setCandidates([]); setPending(false); return; }
    setCandidates([]);
    setPending(true);
    const timer = window.setTimeout(() => {
      window.localPinyin.lookup(pinyin.trim()).then(result => {
        if (request.current === id) { setCandidates(result.candidates); setPending(false); }
      }).catch(error => {
        if (request.current === id) { setLookupError(asError(error)); setCandidates([]); setPending(false); }
      });
    }, 170);
    return () => { window.clearTimeout(timer); request.current++; };
  }, [pinyin, lookupRevision, state.learning.count]);
  const commitSize = () => {
    if (size !== state.settings.textSize && !saving) void saveSettings({ ...state.settings, textSize: size });
  };
  const rawFallback = candidates.length === 1 && /^[a-z0-9']+$/i.test(candidates[0]) && candidates[0].toLowerCase() === pinyin.trim().toLowerCase();
  return <>
    <PageHeader title="外观" description="界面主题和候选窗口的文字大小。" action={autoSave} />
    <SettingsGroup title="主题" description="设置窗口与输入时的候选窗口使用同一个主题。">
      <div className="theme-options" role="radiogroup" aria-label="界面主题">{themes.map(item =>
        <label key={item.value} className={`theme-option${state.settings.theme === item.value ? ' active' : ''}`}>
          <input type="radio" name="theme" aria-label={item.label} value={item.value} checked={state.settings.theme === item.value} disabled={saving}
            onChange={() => void saveSettings({ ...state.settings, theme: item.value })} />
          <ThemeThumbnail theme={item.value} />
          <span className="theme-label"><span className="radio-indicator" aria-hidden="true" /><Icon name={item.icon} size={16} />{item.label}</span>
        </label>)}
      </div>
    </SettingsGroup>
    <SettingsGroup title="候选窗口" description="拖动滑块调整候选字号，下面的预览与实际输入时一致。">
      <div className="setting-row size-row">
        <span className="row-icon"><Icon name="text" size={20} /></span>
        <div className="row-copy"><h3>候选字大小</h3><p>{fontSizes[size - 1]} px · {size === 4 ? '标准' : `第 ${size} 档`}</p></div>
        <div className="row-control range-control">
          <span className="range-small" aria-hidden="true">A</span>
          <input aria-label="候选字大小" aria-valuetext={`${fontSizes[size - 1]} px，第 ${size} 档`} type="range" min="1" max="7" step="1" value={size} disabled={saving}
            style={{ '--range-progress': `${((size - 1) / 6) * 100}%` } as React.CSSProperties}
            onChange={event => setSize(Number(event.target.value))} onPointerUp={commitSize} onKeyUp={commitSize} onBlur={commitSize} />
          <span className="range-large" aria-hidden="true">A</span>
          <button className="text-button" disabled={saving || size === 4} onClick={() => { setSize(4); void saveSettings({ ...state.settings, textSize: 4 }); }}>恢复标准</button>
        </div>
      </div>
      <div className="live-preview">
        <div className="preview-input-wrap">
          <label className="preview-input-label" htmlFor="preview-pinyin">试试拼音</label>
          <span className="preview-input"><Icon name="keyboard" size={17} /><input id="preview-pinyin" type="text" spellCheck={false} autoComplete="off" value={pinyin} maxLength={256} placeholder="输入拼音，如 suibian" onChange={event => setPinyin(event.target.value)} /></span>
        </div>
        <div className="ime-preview" aria-hidden={false}>
          <div className="ime-preview-pinyin">{pinyin.trim() || ' '}</div>
          <PagedCandidates words={candidates} label="候选预览" fontSize={fontSizes[size - 1]} pending={pending} variant="preview" rawFallback={rawFallback}
            emptyText={pending ? '正在查找候选…' : pinyin.trim() ? '没有找到候选，试试别的拼音。' : '输入拼音后，候选词会出现在这里。'} />
        </div>
        <ErrorNotice message={lookupError} />
      </div>
    </SettingsGroup>
    <p className="page-note"><Icon name="info" size={16} />正在使用的输入应用会在下一次输入时读取新设置。</p>
  </>;
}

export function InputPage({ state, saving, saveSettings, lookupRevision }: SettingsPageProps & { lookupRevision: number }) {
  return <>
    <PageHeader title="输入" description="中英文切换、前文排序和联想。" action={autoSave} />
    <SettingsGroup title="中英文切换">
      <SettingRow icon="switch" title="按 Shift 切换中英文" description="单独按下并松开 Shift 键切换输入模式。">
        <Switch checked={state.settings.shiftToggleEnabled} disabled={saving} label="按 Shift 切换中英文" onChange={shiftToggleEnabled => void saveSettings({ ...state.settings, shiftToggleEnabled })} />
      </SettingRow>
      <SettingRow icon="keyboard" title="Ctrl + Space 切换" description="始终可用，不受上面的选项影响。">
        <span className="key-combination"><kbd>Ctrl</kbd><span>+</span><kbd>Space</kbd></span>
      </SettingRow>
    </SettingsGroup>
    <SettingsGroup title="智能输入">
      <SettingRow icon="context" title="根据前文排列候选" description="输入拼音时参考光标前已经输入的文字为同音词排序，例如“他跑”之后输入 de，首选是“得”。">
        <span className="status-badge"><Icon name="check" size={14} />始终开启</span>
      </SettingRow>
      <SettingRow icon="spark" title="联想下一个词" description="词语上屏后，显示可能接着输入的词：按 Tab 接受，Esc 关闭。">
        <Switch checked={state.settings.predictionEnabled} disabled={saving} label="联想下一个词" onChange={predictionEnabled => void saveSettings({ ...state.settings, predictionEnabled })} />
      </SettingRow>
    </SettingsGroup>
    <ContextPreview state={state} lookupRevision={lookupRevision} />
    <SettingsGroup title="快捷键" description="输入法处于中文模式时可用。">
      <div className="shortcut-row"><div><strong>确认候选</strong><span>上屏当前选中的候选</span></div><kbd>Space</kbd></div>
      <div className="shortcut-row"><div><strong>选择候选</strong><span>按当前页的编号选择</span></div><span className="key-combination"><kbd>1</kbd><span>–</span><kbd>9</kbd></span></div>
      <div className="shortcut-row"><div><strong>翻页</strong><span>候选多于一页时切换</span></div><span className="key-combination"><kbd>←</kbd><kbd>→</kbd></span></div>
      <div className="shortcut-row"><div><strong>输入原始拼音</strong><span>把字母原样上屏</span></div><kbd>Enter</kbd></div>
      <div className="shortcut-row"><div><strong>取消输入</strong><span>清空拼音，关闭候选窗口</span></div><kbd>Esc</kbd></div>
      <div className="shortcut-row"><div><strong>切换系统输入法</strong><span>在已安装的输入法之间切换</span></div><span className="key-combination"><kbd>Win</kbd><span>+</span><kbd>Space</kbd></span></div>
    </SettingsGroup>
  </>;
}

function lastContext(text: string): string {
  const tail = text.slice(-32);
  // Do not begin the preview context in the middle of a surrogate pair.
  return tail.length && tail.charCodeAt(0) >= 0xdc00 && tail.charCodeAt(0) <= 0xdfff ? tail.slice(1) : tail;
}

function ContextPreview({ state, lookupRevision }: { state: AppState; lookupRevision: number }) {
  const [precedingText, setPrecedingText] = useState('我想');
  const [pinyin, setPinyin] = useState('');
  const [predictions, setPredictions] = useState<string[]>([]);
  const [candidates, setCandidates] = useState<string[]>([]);
  const [plainCandidates, setPlainCandidates] = useState<string[]>([]);
  const [predictPending, setPredictPending] = useState(false);
  const [lookupPending, setLookupPending] = useState(false);
  const [predictError, setPredictError] = useState<string | null>(null);
  const [lookupError, setLookupError] = useState<string | null>(null);
  const predictionRequest = useRef(0);
  const lookupRequest = useRef(0);
  const inflight = useRef(new Map<string, Promise<{ candidates: string[] }>>());
  const context = lastContext(precedingText).trim();
  const query = pinyin.trim().toLowerCase();
  const revision = `${lookupRevision}:${state.learning.count}`;

  const requestCandidates = (key: string, load: () => Promise<{ candidates: string[] }>) => {
    const existing = inflight.current.get(key);
    if (existing) return existing;
    const promise = Promise.resolve().then(load);
    inflight.current.set(key, promise);
    const remove = () => { if (inflight.current.get(key) === promise) inflight.current.delete(key); };
    void promise.then(remove, remove);
    return promise;
  };

  useEffect(() => {
    const id = ++predictionRequest.current;
    setPredictions([]);
    setPredictError(null);
    if (!context) { setPredictPending(false); return; }
    setPredictPending(true);
    const timer = window.setTimeout(() => {
      requestCandidates(`predict:${revision}:${context}`, () => window.localPinyin.predict(context)).then(result => {
        if (predictionRequest.current === id) { setPredictions(result.candidates); setPredictPending(false); }
      }).catch(failure => {
        if (predictionRequest.current === id) { setPredictError(asError(failure)); setPredictPending(false); }
      });
    }, 180);
    return () => { window.clearTimeout(timer); predictionRequest.current++; };
  }, [context, revision]);

  useEffect(() => {
    const id = ++lookupRequest.current;
    setCandidates([]);
    setPlainCandidates([]);
    setLookupError(null);
    if (!context || !query) { setLookupPending(false); return; }
    if (!/^[a-z0-9']+$/.test(query)) {
      setLookupError('拼音只接受英文字母、数字和英文单引号。');
      setLookupPending(false);
      return;
    }
    setLookupPending(true);
    const timer = window.setTimeout(() => {
      Promise.all([
        requestCandidates(`lookup:${revision}:${context}:${query}`, () => window.localPinyin.lookup(query, context)),
        requestCandidates(`lookup:${revision}::${query}`, () => window.localPinyin.lookup(query)),
      ]).then(([withContext, plain]) => {
        if (lookupRequest.current === id) { setCandidates(withContext.candidates); setPlainCandidates(plain.candidates); setLookupPending(false); }
      }).catch(failure => {
        if (lookupRequest.current === id) { setLookupError(asError(failure)); setLookupPending(false); }
      });
    }, 180);
    return () => { window.clearTimeout(timer); lookupRequest.current++; };
  }, [context, query, revision]);

  const rawFallback = candidates.length === 1 && /^[a-z0-9']+$/i.test(candidates[0]) && candidates[0].toLowerCase() === query;
  const emptyLookup = !context ? '填写前文后，查看拼音候选。' : !query ? '输入拼音，看看前文如何改变候选顺序。' : lookupPending ? '正在查找…' : lookupError ? '拼音候选查询未完成。' : '没有找到候选，试试别的拼音。';
  return <SettingsGroup title="试一试" description="在这里体验前文排序和联想；试用不会写入学习记录。" className="context-preview-card">
    <div className="context-preview-fields">
      <div className="context-field">
        <div className="context-field-label"><label htmlFor="context-preview-text">前文</label><span>{precedingText.length}/32</span></div>
        <input id="context-preview-text" type="text" maxLength={32} autoComplete="off" value={precedingText} placeholder="例如：我想" onChange={event => setPrecedingText(lastContext(event.target.value))} />
      </div>
      <div className="context-field">
        <div className="context-field-label"><label htmlFor="context-preview-pinyin">后续拼音</label><span>例如 qu、de、shijian</span></div>
        <input id="context-preview-pinyin" type="text" maxLength={256} autoComplete="off" spellCheck={false} value={pinyin} placeholder="输入拼音" onChange={event => setPinyin(event.target.value)} />
      </div>
    </div>
    <div className="context-preview-results">
      <section className="context-result" aria-label="前文拼音候选" aria-busy={lookupPending}>
        <div className="context-result-heading"><h3>参考前文</h3><span>输入时的实际顺序</span></div>
        <PagedCandidates words={candidates} label="前文候选" variant="context" pending={lookupPending} rawFallback={rawFallback} emptyText={emptyLookup} />
        <ErrorNotice message={lookupError} />
      </section>
      <section className="context-result muted" aria-label="无前文拼音候选" aria-busy={lookupPending}>
        <div className="context-result-heading"><h3>不参考前文</h3><span>对比</span></div>
        <PagedCandidates words={plainCandidates} label="无前文候选" variant="context" pending={lookupPending} emptyText={emptyLookup} />
      </section>
      <section className="context-result" aria-label="联想后续词" aria-busy={predictPending}>
        <div className="context-result-heading"><h3>联想下一个词</h3><span>点击追加到前文</span></div>
        <PagedCandidates words={predictions} label="联想候选" variant="prediction" pending={predictPending}
          emptyText={!context ? '填写前文后，在这里查看后续词。' : predictPending ? '正在查找后续词…' : predictError ? '后续词查询未完成。' : '这段前文暂时没有后续词。'}
          onSelect={word => setPrecedingText(current => lastContext(current + word))} />
        <ErrorNotice message={predictError} />
      </section>
    </div>
    <div className="context-preview-footer"><span>联想开关只影响实际输入，这里始终可以试用。</span><button className="text-button" onClick={() => { setPrecedingText('我想'); setPinyin(''); }}>重置</button></div>
  </SettingsGroup>;
}

export function LearningPage({ state, onState, notify }: { state: AppState; onState: (state: AppState) => void; notify: (message: string) => void }) {
  const [confirm, setConfirm] = useState(false);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const clear = async () => {
    setPending(true); setError(null);
    try {
      const result = await window.localPinyin.clearLearning();
      if (!result.cleared) throw new Error('学习记录未能清除，请重试。');
      const snapshot = await window.localPinyin.getState();
      onState(snapshot); setConfirm(false); notify('学习记录已清除');
    } catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <>
    <PageHeader title="学习" description="根据你选择过的词调整候选顺序，记录只保存在这台电脑。" />
    <div className="card learning-summary">
      <div className="learning-count"><span className="learning-number">{formatNumber(state.learning.count)}</span><span>条学习记录</span></div>
      <ul className="learning-points">
        <li><Icon name="check" size={16} />常选的词会排得更靠前</li>
        <li><Icon name="check" size={16} />选过的词组在整句里也会优先组合</li>
        <li><Icon name="shield" size={16} />不上传，不记录你输入的其他内容</li>
      </ul>
    </div>
    <SettingsGroup title="管理">
      <SettingRow icon="reset" title="清除学习记录" description="让候选顺序恢复到初始状态。你添加到个人词库的词语会保留。">
        <button className="button subtle-danger" onClick={() => { setError(null); setConfirm(true); }}>清除学习记录</button>
      </SettingRow>
    </SettingsGroup>
    <p className="page-note"><Icon name="info" size={16} />清除后，正在运行的输入应用需要重新打开，才会停止使用已有的学习缓存。</p>
    {confirm && <Modal title="清除学习记录？" description="候选排序和整句学习将恢复到初始状态，个人词库会保留。" busy={pending} onClose={() => setConfirm(false)}><div className="modal-body"><p className="confirmation-copy">这项操作无法撤销。之后输入时，输入法会重新学习你的选词习惯。</p><ErrorNotice message={error} /></div><footer className="modal-footer"><button className="button secondary" disabled={pending} onClick={() => setConfirm(false)}>取消</button><button className="button danger" disabled={pending} onClick={() => void clear()}>{pending ? '正在清除…' : '清除记录'}</button></footer></Modal>}
  </>;
}

export function AboutPage({ state }: { state: AppState }) {
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const activeSession = state.runtime.available && state.runtime.active;
  const sessionLabel = activeSession ? (state.runtime.mode.toLowerCase() === 'english' ? '英文模式' : '中文模式') : '未在使用';
  const openFolder = async () => {
    setPending(true); setError(null);
    try { await window.localPinyin.openDataDirectory(); }
    catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <>
    <PageHeader title="关于" description="一个完全在本机工作的中文拼音输入法。" />
    <div className="card about-hero"><BrandMark size={56} /><div><h2>本地拼音</h2><p>Local Pinyin · 版本 {state.version}</p></div><span className="status-badge"><Icon name="shield" size={14} />完全离线</span></div>
    <SettingsGroup title="运行信息">
      <dl className="about-details">
        <div><dt>输入状态</dt><dd>{sessionLabel}<span className={`inline-status${activeSession ? '' : ' unavailable'}`} /></dd></div>
        <div><dt>内置词库</dt><dd>{formatNumber(state.dictionary.total)} 条</dd></div>
        <div><dt>个人词库</dt><dd>{formatNumber(state.userLexiconCount)} 条</dd></div>
        <div><dt>数据位置</dt><dd className="data-path" title={state.dataDirectory}>{state.dataDirectory}</dd></div>
      </dl>
      <div className="about-data-footer"><span>个人词库、设置与学习记录都保存在这个文件夹。</span><button className="button secondary compact" disabled={pending} onClick={() => void openFolder()}><Icon name="folder" size={16} />{pending ? '正在打开…' : '打开数据文件夹'}</button></div>
      <ErrorNotice message={error} />
    </SettingsGroup>
    <SettingsGroup title="致谢">
      <p className="credits">前文语言模型的统计数据来自 Tatoeba（CC BY 2.0 FR）、中文维基新闻（CC BY 2.5）、Mozilla Common Voice 句子（CC0）和项目原创语料；只包含字词组合的统计数据，不包含原文。标准词库源自 Rime 拼音词库（Apache-2.0）。设置界面使用 Electron 与 React（MIT）。</p>
    </SettingsGroup>
  </>;
}
