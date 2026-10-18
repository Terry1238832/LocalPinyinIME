import { useEffect, useId, useState } from 'react';
import { CandidatePreview, StyleThumbnail, candidateStyles, previewSample } from './CandidatePreview';
import { InputOptions } from './InputOptions';
import { StyleGallery } from './StyleGallery';
import { BrandMark, Icon, type IconName } from './icons';
import { asError, Badge, ErrorNotice, formatNumber, InfoButton, Keys, Meter, MiniCandidates, Modal, PageHeader, SaveStatus, SettingRow, SettingStatus, SettingsGroup, Switch } from './components';
import { KeyDemo, TypingDemo } from './FeatureDemo';
import { usePageCommand } from './CommandPalette';
import type { AppState, CandidateStyle, Settings, Theme, UpdateStatus } from './types';
import { UpdatePanel } from './Updates';

const fontSizes = [14, 16, 17, 18, 20, 22, 24];
type SettingsPageProps = { state: AppState; saving: boolean; saveSettings: (settings: Settings) => Promise<void> };

const themes: { value: Theme; label: string; icon: IconName }[] = [
  { value: 'light', label: '浅色', icon: 'sun' },
  { value: 'dark', label: '深色', icon: 'moon' },
  { value: 'system', label: '跟随系统', icon: 'monitor' },
];

// A theme swatch is the chosen candidate style in that theme's colors.
function ThemeSwatch({ theme, candidateStyle }: { theme: Theme; candidateStyle: CandidateStyle }) {
  return <span className={`swatch-art swatch-art-${theme}`} aria-hidden="true">
    {theme === 'system'
      ? <><span className="swatch-half"><StyleThumbnail candidateStyle={candidateStyle} tone="light" /></span>
          <span className="swatch-half swatch-half-dark"><StyleThumbnail candidateStyle={candidateStyle} tone="dark" /></span></>
      : <StyleThumbnail candidateStyle={candidateStyle} tone={theme} />}
  </span>;
}

export function AppearancePage({ state, saving, saveSettings }: SettingsPageProps) {
  const [size, setSize] = useState(state.settings.textSize);
  // Shown at once; saved in the background like the other settings.
  const [candidateStyle, setCandidateStyle] = useState<CandidateStyle>(state.settings.candidateStyle);
  useEffect(() => { if (!saving) setSize(state.settings.textSize); }, [state.settings.textSize, saving]);
  useEffect(() => { if (!saving) setCandidateStyle(state.settings.candidateStyle); }, [state.settings.candidateStyle, saving]);
  const chooseStyle = (next: CandidateStyle) => {
    setCandidateStyle(next);
    if (next !== state.settings.candidateStyle) void saveSettings({ ...state.settings, candidateStyle: next });
  };
  const commitSize = () => {
    if (size !== state.settings.textSize && !saving) void saveSettings({ ...state.settings, textSize: size });
  };
  return <>
    <PageHeader kicker="wai'guan" title="外观" description="候选窗口的样子，所见即所得。" action={<SaveStatus saving={saving} />} />
    <StyleGallery value={candidateStyle} disabled={saving} onChoose={chooseStyle} />
    <section className="studio" aria-label="候选窗口">
      <div className="stage">
        <div className="stage-scene">
          <p className="stage-document" aria-hidden="true">周末一起出去走走，<span className="stage-composition">{previewSample.pinyin.replaceAll("'", '')}</span><span className="stage-caret" /></p>
          <CandidatePreview candidateStyle={candidateStyle} words={previewSample.words} pinyin={previewSample.pinyin} fontSize={fontSizes[size - 1]} />
        </div>
        <span className="stage-meta">{candidateStyles.find(item => item.value === candidateStyle)?.name} · 实际大小 · {fontSizes[size - 1]} px</span>
      </div>
      <div className="inspector">
        <div className="inspector-field field-theme">
          <span className="field-label" id="theme-field-label">主题</span>
          <div className="theme-swatches" role="radiogroup" aria-label="界面主题">{themes.map(item =>
            <label key={item.value} className={`theme-swatch${state.settings.theme === item.value ? ' active' : ''}`}>
              <input type="radio" name="theme" aria-label={item.label} value={item.value} checked={state.settings.theme === item.value} disabled={saving}
                onChange={() => void saveSettings({ ...state.settings, theme: item.value })} />
              <ThemeSwatch theme={item.value} candidateStyle={candidateStyle} />
              <span className="swatch-label"><Icon name={item.icon} size={13} />{item.label}</span>
            </label>)}
          </div>
        </div>
        <div className="inspector-field field-size">
          <div className="field-label-row"><span className="field-label">候选字号</span><button className="text-button" disabled={saving || size === 4} onClick={() => { setSize(4); void saveSettings({ ...state.settings, textSize: 4 }); }}>恢复标准</button></div>
          <div className="size-control">
            <span className="range-small" aria-hidden="true">A</span>
            <span className="range-wrap">
              <input aria-label="候选字大小" aria-valuetext={`${fontSizes[size - 1]} px，第 ${size} 档`} type="range" min="1" max="7" step="1" value={size} disabled={saving}
                style={{ '--range-progress': `${((size - 1) / 6) * 100}%` } as React.CSSProperties}
                onChange={event => setSize(Number(event.target.value))} onPointerUp={commitSize} onKeyUp={commitSize} onBlur={commitSize} />
              <span className="range-ticks" aria-hidden="true">{fontSizes.map((_, index) => <i key={index} className={index < size ? 'on' : undefined} />)}</span>
            </span>
            <span className="range-large" aria-hidden="true">A</span>
            <output className="size-value">{fontSizes[size - 1]}<span>px</span></output>
          </div>
        </div>
      </div>
    </section>
  </>;
}

const shortcuts: { title: string; keys: string[]; joiner?: string }[] = [
  { title: '确认候选', keys: ['Space'] },
  { title: '选择候选', keys: ['1', '9'], joiner: '–' },
  { title: '翻页', keys: ['←', '→'], joiner: '' },
  { title: '原样上屏', keys: ['Enter'] },
  { title: '取消输入', keys: ['Esc'] },
  { title: '切换输入法', keys: ['Win', 'Space'] },
];

export function InputPage({ state, saving, saveSettings }: SettingsPageProps) {
  const settings = state.settings;
  return <>
    <PageHeader kicker="shu'ru" title="输入" description="切换方式、候选、常用场景、拼写、输入方案与辅助功能。" action={<SaveStatus saving={saving} />} />
    <SettingsGroup id="group-switching" title="中英文切换">
      <SettingRow title="按 Shift 切换中英文" description="单独按下并松开 Shift 键时切换；按住 Shift 打出的大写字母照常输入。正在输入的字母会原样保留。"
        demo={<KeyDemo before="hello" keys={['Shift']} modes={['中', '英']} />}>
        <Switch checked={settings.shiftToggleEnabled} disabled={saving} label="按 Shift 切换中英文"
          onChange={shiftToggleEnabled => void saveSettings({ ...settings, shiftToggleEnabled })} />
      </SettingRow>
      <SettingRow title="Ctrl + Space 切换" description="随时可用，不受上面的选项影响。正在输入的字母会原样保留，不会消失。"
        demo={<KeyDemo before="hello" keys={['Ctrl', 'Space']} modes={['中', '英']} />}><SettingStatus>始终开启</SettingStatus></SettingRow>
    </SettingsGroup>
    <SettingsGroup id="group-candidates" title="候选">
      <SettingRow title="联想下一个词" description="词语上屏后显示接下来可能输入的词：Tab 接受，Esc 关闭。"
        demo={<TypingDemo before="你好" keys="" candidates={['世界', '朋友', '呀']} after="世界" />}>
        <Switch checked={settings.predictionEnabled} disabled={saving} label="联想下一个词"
          onChange={predictionEnabled => void saveSettings({ ...settings, predictionEnabled })} />
      </SettingRow>
      <SettingRow title="根据前文排列候选" description="参考光标前的文字为同音词排序，例如“他跑”之后输入 de，首选是“得”。"
        demo={<TypingDemo before="他跑" keys="de" candidates={['得', '的', '地']} after="得" />}>
        <SettingStatus>始终开启</SettingStatus>
      </SettingRow>
    </SettingsGroup>
    <InputOptions settings={settings} saving={saving} onChange={next => void saveSettings(next)} />
    <section className="legend" aria-labelledby="legend-title">
      <h2 className="block-title" id="legend-title">快捷键<span>中文模式下可用</span></h2>
      <ul className="legend-grid">{shortcuts.map(item =>
        <li key={item.title}><Keys keys={item.keys} joiner={item.joiner ?? '+'} /><span>{item.title}</span></li>)}
      </ul>
    </section>
  </>;
}

export function LearningPage({ state, onState, notify }: { state: AppState; onState: (state: AppState) => void; notify: (message: string) => void }) {
  const [confirm, setConfirm] = useState(false);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [resetInfo, setResetInfo] = useState(false);
  const resetInfoId = useId();
  usePageCommand('clear-learning', () => { setError(null); setConfirm(true); });
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
  const { count, capacity } = state.learning;
  return <>
    <PageHeader kicker="xue'xi" title="学习" description="记住你的选词习惯，只保存在这台电脑。" />
    <section className="learning-hero" aria-label="学习概况">
      <div className="hero-figure">
        <span className="eyebrow"><Icon name="history" size={14} />学习记录</span>
        <div className="hero-value">{formatNumber(count)}<span>条</span></div>
        <div className="meter-block">
          <Meter value={count} max={capacity} label="学习记录用量" />
          <p className="meter-caption">已用 {formatNumber(count)} / {formatNumber(capacity)} 条，超出后最早的记录会被替换</p>
        </div>
      </div>
      <ol className="flow" aria-label="学习如何改变候选">
        <li>
          <MiniCandidates pinyin="annui" words={['安徽', '按钮']} selected={1} />
          <strong><span className="flow-step">1</span>你选了“按钮”</strong><span>选择只发生在本机</span>
        </li>
        <li>
          <span className="flow-memory" aria-hidden="true"><Icon name="history" size={18} /><b>annui → 按钮</b></span>
          <strong><span className="flow-step">2</span>记住这次选择</strong><span>最近选过的最优先，其次看次数</span>
        </li>
        <li>
          <MiniCandidates pinyin="annui" words={['按钮', '安徽']} selected={0} />
          <strong><span className="flow-step">3</span>下次排在第一</strong><span>整句里也会优先组合</span>
        </li>
      </ol>
    </section>
    <section className="danger-zone">
      <div><h3>清除学习记录<InfoButton label="清除学习记录" open={resetInfo} controls={resetInfoId} onToggle={setResetInfo} /></h3>
        <p id={resetInfoId} hidden={!resetInfo}>候选顺序恢复到初始状态，个人词库会保留。清除后，正在运行的程序需要重新打开。</p></div>
      <button className="button secondary danger-text" onClick={() => { setError(null); setConfirm(true); }}>清除学习记录</button>
    </section>
    {confirm && <Modal title="清除学习记录？" description="候选排序和整句学习将恢复到初始状态，个人词库会保留。" busy={pending} onClose={() => setConfirm(false)}><div className="modal-body"><p className="confirmation-copy">这项操作无法撤销。之后输入时，输入法会重新学习你的选词习惯。</p><ErrorNotice message={error} /></div><footer className="modal-footer"><button className="button secondary" disabled={pending} onClick={() => setConfirm(false)}>取消</button><button className="button danger" disabled={pending} onClick={() => void clear()}>{pending ? '正在清除…' : '清除记录'}</button></footer></Modal>}
  </>;
}

export function AboutPage({ state, update }: { state: AppState; update: UpdateStatus | null }) {
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
  usePageCommand('open-data', () => void openFolder());
  return <>
    <PageHeader kicker="guan'yu" title="关于" description="一个完全在本机工作的中文拼音输入法。" />
    <div className="about-layout">
    <section className="about-hero">
      <span className="about-mark"><BrandMark size={72} /></span>
      <div className="about-identity">
        <h2>本地拼音</h2>
        <p>Local Pinyin · 版本 {state.version}</p>
        <div className="about-badges"><Badge tone="success" icon="shield">离线输入</Badge><Badge icon="keyboard">Windows TSF</Badge></div>
      </div>
    </section>
    <div className="about-details">
    <dl className="facts">
      <div><dt>输入状态</dt><dd><span className={`inline-status${activeSession ? '' : ' unavailable'}`} />{sessionLabel}</dd></div>
      <div><dt>内置词库</dt><dd>{formatNumber(state.dictionary.total)}<span>条</span></dd></div>
      <div><dt>个人词库</dt><dd>{formatNumber(state.userLexiconCount)}<span>条</span></dd></div>
      <div><dt>学习记录</dt><dd>{formatNumber(state.learning.count)}<span>条</span></dd></div>
    </dl>
    <section className="data-location about-data-footer">
      <div className="data-copy"><span className="eyebrow"><Icon name="folder" size={14} />数据位置</span><span className="data-path" title={state.dataDirectory}>{state.dataDirectory}</span><p>个人词库、设置与学习记录都保存在这个文件夹。</p></div>
      <button className="button secondary" disabled={pending} onClick={() => void openFolder()}><Icon name="folder" size={15} />{pending ? '正在打开…' : '打开数据文件夹'}</button>
    </section>
    </div>
    <ErrorNotice message={error} />
    </div>
    <UpdatePanel status={update} />
    <section className="credits">
      <h2 className="block-title">致谢</h2>
      <p>前文语言模型的统计数据来自 Tatoeba（CC BY 2.0 FR）、中文维基新闻（CC BY 2.5）、Mozilla Common Voice 句子（CC0）和项目原创语料；只包含字词组合的统计数据，不包含原文。标准词库源自 Rime 拼音词库（Apache-2.0），常用词与字参考了 jieba 词频表（MIT）与 Unicode Unihan 数据。设置界面使用 Electron 与 React（MIT）。</p>
    </section>
  </>;
}
