import { useEffect, useRef, useState } from 'react';
import { PagedCandidates } from "./PagedCandidates";
import { BrandMark, Icon, type IconName } from './icons';
import { asError, ErrorNotice, formatNumber, InfoRow, Modal, PageHeader, SectionTitle, Switch } from './components';
import type { AppState, Settings, Theme } from './types';

const fontSizes = [14, 16, 17, 18, 20, 22, 24];
type SettingsPageProps = { state: AppState; saving: boolean; saveSettings: (settings: Settings) => Promise<void> };

const themes: { value: Theme; label: string; icon: IconName; description: string }[] = [
  { value: 'light', label: '浅色', icon: 'sun', description: '清晰、明亮' },
  { value: 'dark', label: '深色', icon: 'moon', description: '柔和、专注' },
  { value: 'system', label: '跟随系统', icon: 'monitor', description: '与 Windows 保持一致' },
];

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
    <PageHeader eyebrow="让输入更合眼缘" title="外观" description="选一个喜欢的主题，把候选词调到刚刚好的大小。" action={<span className="quiet-tag"><Icon name="check" size={14} />自动保存</span>} />
    <div className="card theme-card"><SectionTitle title="界面主题" description="设置界面与输入候选窗口使用同一个主题。" /><div className="theme-options" role="radiogroup" aria-label="界面主题">{themes.map(item => <label key={item.value} className={`theme-option${state.settings.theme === item.value ? ' active' : ''}`}><input type="radio" name="theme" aria-label={item.label} value={item.value} checked={state.settings.theme === item.value} disabled={saving} onChange={() => void saveSettings({ ...state.settings, theme: item.value })} /><div className={`theme-sample sample-${item.value}`}><div className="sample-sidebar"><span /><span /><span /></div><div className="sample-content"><span className="sample-heading" /><span className="sample-line" /><div className="sample-box"><span /><span /></div></div></div><div className="theme-option-footer"><span className="theme-icon"><Icon name={item.icon} size={17} /></span><div><strong>{item.label}</strong><span>{item.description}</span></div><span className="radio-indicator">{state.settings.theme === item.value && <span />}</span></div></label>)}</div></div>
    <div className="card preview-card"><div className="preview-settings"><SectionTitle title="候选字大小" description="七档字号，适应不同的屏幕和阅读习惯。" /><div className="size-value"><span>{fontSizes[size - 1]}</span><span>px</span><span className="size-level">{size === 4 ? '标准' : `第 ${size} 档`}</span></div><div className="range-control"><span className="range-small">字</span><input aria-label="候选字大小" aria-valuetext={`${fontSizes[size - 1]} px，第 ${size} 档`} type="range" min="1" max="7" step="1" value={size} disabled={saving} onChange={event => setSize(Number(event.target.value))} onPointerUp={commitSize} onKeyUp={commitSize} onBlur={commitSize} /><span className="range-large">字</span></div><div className="range-labels"><span>小</span><button className="text-button" disabled={saving || size === 4} onClick={() => { setSize(4); void saveSettings({ ...state.settings, textSize: 4 }); }}>恢复标准</button><span>大</span></div></div><div className="live-preview"><div className="preview-topline"><span className="eyebrow">实时预览</span><span className="live-indicator">本地引擎</span></div><label className="preview-input-label" htmlFor="preview-pinyin">试试拼音</label><div className="preview-input-wrap"><Icon name="keyboard" size={18} /><input id="preview-pinyin" type="text" spellCheck={false} autoComplete="off" value={pinyin} maxLength={256} placeholder="输入拼音，如 suibian" onChange={event => setPinyin(event.target.value)} /><span className="input-caret" /></div><PagedCandidates words={candidates} label="候选预览" fontSize={fontSizes[size - 1]} pending={pending} variant="preview" rawFallback={rawFallback} emptyText={pending ? "正在查找候选…" : pinyin.trim() ? "没有找到候选，试试别的拼音。" : "输入拼音后，候选词会出现在这里。"} /><p className="preview-footnote">候选单行显示，左右箭头翻页；预览仅查询本地词库。</p><ErrorNotice message={lookupError} /></div></div>
    <div className="page-note"><Icon name="spark" size={17} /><span>设置会自动保存。正在打开的输入应用，会在下一次输入时读取新设置。</span></div>
  </>;
}

export function InputPage({ state, saving, saveSettings, lookupRevision }: SettingsPageProps & { lookupRevision: number }) {
  return <>
    <PageHeader eyebrow="让习惯自然延续" title="输入习惯" description="保留熟悉的快捷键，让中文和英文切换更顺手。" action={<span className="quiet-tag"><Icon name="check" size={14} />自动保存</span>} />
    <div className="card">
      <SectionTitle title="输入选项" />
      <InfoRow icon="keyboard" title="按 Shift 切换中英文" description="单独按下并松开 Shift，就能切换输入模式。">
        <Switch checked={state.settings.shiftToggleEnabled} disabled={saving} label="按 Shift 切换中英文" onChange={shiftToggleEnabled => void saveSettings({ ...state.settings, shiftToggleEnabled })} />
      </InfoRow>
      <InfoRow icon="spark" title="前文联想" description="词语上屏后，根据光标前文推荐后续词，左右箭头翻页，Tab 接受当前页联想。" last>
        <Switch checked={state.settings.predictionEnabled} disabled={saving} label="前文联想" onChange={predictionEnabled => void saveSettings({ ...state.settings, predictionEnabled })} />
      </InfoRow>
    </div>
    <ContextPreview state={state} lookupRevision={lookupRevision} />
    <div className="card shortcut-card"><SectionTitle title="常用快捷键" description="在输入法激活时使用。" /><div className="shortcut-row"><div><strong>切换中英文</strong><span>不论是否启用 Shift，都可以使用</span></div><div className="key-combination"><kbd>Ctrl</kbd><span>+</span><kbd>Space</kbd></div></div><div className="shortcut-row"><div><strong>确认当前候选</strong><span>确认当前词语，继续输入</span></div><kbd>Space</kbd></div><div className="shortcut-row"><div><strong>选择其他候选</strong><span>按当前页编号选词</span></div><div className="key-combination"><kbd>1</kbd><span>—</span><kbd>9</kbd></div></div><div className="shortcut-row"><div><strong>候选翻页</strong><span>单行显示，左右箭头切换上一页和下一页</span></div><div className="key-combination"><kbd>←</kbd><span>/</span><kbd>→</kbd></div></div><div className="shortcut-row"><div><strong>取消当前拼音</strong><span>关闭候选窗口，清空当前输入</span></div><kbd>Esc</kbd></div><div className="shortcut-row last"><div><strong>切换系统输入法</strong><span>在 Windows 已安装的输入法之间切换</span></div><div className="key-combination"><kbd>Win</kbd><span>+</span><kbd>Space</kbd></div></div></div>
    <div className="tip-card"><span className="tip-icon"><Icon name="input" size={24} /></span><div><h3>整句输入，也可以分词选择</h3><p>输入连续拼音后，按数字选择候选词。遇到人名或专业词语，可以把它加入「我的词库」。</p></div></div>
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
    setLookupError(null);
    if (!context || !query) { setLookupPending(false); return; }
    if (!/^[a-z0-9']+$/.test(query)) {
      setLookupError('拼音只接受英文字母、数字和英文单引号。');
      setLookupPending(false);
      return;
    }
    setLookupPending(true);
    const timer = window.setTimeout(() => {
      requestCandidates(`lookup:${revision}:${context}:${query}`, () => window.localPinyin.lookup(query, context)).then(result => {
        if (lookupRequest.current === id) { setCandidates(result.candidates); setLookupPending(false); }
      }).catch(failure => {
        if (lookupRequest.current === id) { setLookupError(asError(failure)); setLookupPending(false); }
      });
    }, 180);
    return () => { window.clearTimeout(timer); lookupRequest.current++; };
  }, [context, query, revision]);

  const rawFallback = candidates.length === 1 && /^[a-z0-9']+$/i.test(candidates[0]) && candidates[0].toLowerCase() === query;
  return <div className="card context-preview-card">
    <div className="context-preview-heading">
      <SectionTitle title="试试前文联想" description="在这里体验本地联想，点击后续词只会追加到这块试用区。" />
      <span className="quiet-tag"><Icon name="shield" size={14} />完全离线</span>
    </div>
    <div className="context-preview-fields">
      <div className="context-field">
        <div className="context-field-label"><label htmlFor="context-preview-text">前文</label><span>{precedingText.length}/32</span></div>
        <input id="context-preview-text" type="text" maxLength={32} autoComplete="off" value={precedingText} placeholder="例如：我想" onChange={event => setPrecedingText(lastContext(event.target.value))} />
      </div>
      <div className="context-field">
        <label htmlFor="context-preview-pinyin">后续拼音 <span>可选</span></label>
        <input id="context-preview-pinyin" type="text" maxLength={256} autoComplete="off" spellCheck={false} value={pinyin} placeholder="例如：qu" onChange={event => setPinyin(event.target.value)} />
      </div>
    </div>
    <div className="context-preview-results">
      <section className="context-result" aria-label="联想后续词" aria-busy={predictPending}>
        <div className="context-result-heading"><h3>后续词</h3><span>点击追加到前文</span></div>
        <PagedCandidates words={predictions} label="联想候选" variant="prediction" pending={predictPending} emptyText={!context ? "填写前文后，在这里查看后续词。" : predictPending ? "正在查找后续词…" : predictError ? "后续词查询未完成。" : "这段前文暂时没有后续词，可以继续输入拼音。"} onSelect={word => setPrecedingText(current => lastContext(current + word))} />
        <ErrorNotice message={predictError} />
      </section>
      <section className="context-result" aria-label="前文拼音候选" aria-busy={lookupPending}>
        <div className="context-result-heading"><h3>根据前文排列的拼音候选</h3><span>只读预览</span></div>
        <PagedCandidates words={candidates} label="前文候选" variant="context" pending={lookupPending} rawFallback={rawFallback} emptyText={!context ? "填写前文后，查看拼音候选。" : !query ? "输入后续拼音，看看前文如何影响选词。" : lookupPending ? "正在根据前文查找…" : lookupError ? "拼音候选查询未完成。" : "没有找到候选，试试别的拼音。"} />
        <ErrorNotice message={lookupError} />
      </section>
    </div>
    <div className="context-preview-footer"><span>试用不会写入学习记录；关闭前文联想开关后，仍可以在这里查询。</span><button className="text-button" onClick={() => { setPrecedingText('我想'); setPinyin(''); }}>重置试用</button></div>
    <div className="prediction-shortcuts"><Icon name="keyboard" size={16} /><p>实际输入时，词语上屏后按 <kbd>Tab</kbd> 接受当前页选中的联想词，按 <kbd>Esc</kbd> 关闭联想。空格和数字保持原来的输入行为，不会自动续写。</p></div>
  </div>;
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
    <PageHeader eyebrow="越来越懂你的表达" title="个性学习" description="记住你选择过的词，让常用表达更容易找到。" />
    <div className="learning-hero card"><div><span className="eyebrow">本机学习记录</span><div className="learning-count">{formatNumber(state.learning.count)}<span>条</span></div><p>在输入中积累，保存在这台电脑。</p></div><div className="learning-illustration" aria-hidden="true"><span className="orb orb-one" /><span className="orb orb-two" /><div className="learning-symbol"><Icon name="learning" size={48} /></div><span className="floating-word word-one">你好</span><span className="floating-word word-two">下次见</span><span className="floating-dot" /></div></div>
    <div className="learning-features"><div className="card feature-card"><span className="feature-icon violet"><Icon name="spark" size={23} /></span><h3>常用词更靠前</h3><p>根据你的候选选择，调整词语的排列顺序。</p></div><div className="card feature-card"><span className="feature-icon mint"><Icon name="shield" size={23} /></span><h3>隐私留在本机</h3><p>输入内容和学习记录不需要上传到服务器。</p></div></div>
    <div className="card reset-card"><InfoRow icon="reset" title="重新开始学习" description="清除候选排序与上下文学习记录，保留你添加的词语。" last><button className="button subtle-danger" onClick={() => { setError(null); setConfirm(true); }}>清除学习记录</button></InfoRow></div>
    <div className="page-note"><Icon name="about" size={17} /><span>清除后，正在运行的输入应用需要重新打开，才能停止使用已有的学习缓存。</span></div>
    {confirm && <Modal title="清除学习记录？" description="候选排序和上下文学习将恢复到初始状态。你添加到词库的词语会保留。" busy={pending} onClose={() => setConfirm(false)}><div className="modal-body"><div className="confirmation-symbol danger"><Icon name="reset" size={28} /></div><p className="confirmation-copy">这项操作无法撤销。以后输入时，输入法会重新学习你的选词习惯。</p><ErrorNotice message={error} /></div><footer className="modal-footer"><button className="button secondary" disabled={pending} onClick={() => setConfirm(false)}>取消</button><button className="button danger" disabled={pending} onClick={() => void clear()}>{pending ? '正在清除…' : '清除记录'}</button></footer></Modal>}
  </>;
}

export function AboutPage({ state }: { state: AppState }) {
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const activeSession = state.runtime.available && state.runtime.active;
  const sessionLabel = activeSession ? (state.runtime.mode.toLowerCase() === 'english' ? '英文输入中' : '中文输入中') : '未检测到活动会话';
  const openFolder = async () => {
    setPending(true); setError(null);
    try { await window.localPinyin.openDataDirectory(); }
    catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <>
    <PageHeader eyebrow="专注输入本身" title="关于 Local Pinyin" description="一个在本机工作的中文拼音输入法。" />
    <div className="about-hero card"><BrandMark size={72} /><div><h2>Local Pinyin</h2><p>让表达自然发生。</p><span className="version-tag">版本 {state.version}</span></div><span className="about-decoration" aria-hidden="true">拼</span></div>
    <div className="about-values"><div><Icon name="shield" size={22} /><strong>完全离线</strong><p>断网也能输入</p></div><div><Icon name="dictionary" size={22} /><strong>本地词库</strong><p>{formatNumber(state.dictionary.total)} 条内置词语</p></div><div><Icon name="learning" size={22} /><strong>个性学习</strong><p>记住你的选词习惯</p></div></div>
    <div className="card about-details"><SectionTitle title="运行信息" /><dl><div><dt>输入会话</dt><dd title="在输入应用中选择本地拼音后显示状态">{sessionLabel}<span className={`inline-status${activeSession ? '' : ' unavailable'}`} /></dd></div><div><dt>输入方式</dt><dd>全拼 · 连续拼音 · 前文联想 · 个人词库</dd></div><div><dt>设置界面</dt><dd>Electron · React · TypeScript</dd></div><div><dt>数据位置</dt><dd className="data-path" title={state.dataDirectory}>{state.dataDirectory}</dd></div></dl><div className="about-data-footer"><span>个人词库、设置与学习记录都保存在这里。</span><button className="button secondary compact" disabled={pending} onClick={() => void openFolder()}><Icon name="folder" size={16} />{pending ? '正在打开…' : '打开数据文件夹'}</button></div><ErrorNotice message={error} /></div>
    <p className="about-footnote">支持光标前文排序与离线后续词联想。同音词和较长的句子，仍可能需要手动选择候选。</p>
  </>;
}
