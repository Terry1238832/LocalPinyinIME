// 工具面板: how the panel opens and stays, emoji among candidates, and the
// clipboard history (on or off, how long copies are kept, clearing it).
// Also 输入习惯 (the panel's 添加短语, managed on the 学习 page) and the
// clipboard question asked once after installing or updating.
import { useCallback, useEffect, useState } from 'react';
import { asError, ErrorNotice, formatNumber, Modal, PageHeader, SaveStatus, SettingRow, SettingsGroup, Switch } from './components';
import { KeyDemo } from './FeatureDemo';
import { Icon } from './icons';
import { clipboardRetentionChoices, type AppState, type Phrase, type Settings } from './types';

type Props = { state: AppState; saving: boolean; saveSettings: (settings: Settings) => Promise<void>; notify: (message: string) => void };

const retentionLabel = (days: number) => days === 0 ? '永久' : days === 1 ? '1 天' : days === 365 ? '1 年' : `${days} 天`;
const size = (bytes: number) => bytes < 1024 * 1024 ? `${Math.max(1, Math.round(bytes / 1024))} KB` : `${(bytes / 1024 / 1024).toFixed(1)} MB`;

export function PanelPage({ state, saving, saveSettings, notify }: Props) {
  const settings = state.settings;
  const [summary, setSummary] = useState<{ items: number; images: number; bytes: number } | null>(null);
  const [clearing, setClearing] = useState(false);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const recording = settings.clipboardHistory === 'on';
  const loadSummary = useCallback(() => {
    window.localPinyin.clipboardSummary().then(setSummary).catch(() => setSummary(null));
  }, []);
  useEffect(() => { loadSummary(); }, [loadSummary, settings.clipboardHistory]);
  const clear = async () => {
    setPending(true); setError(null);
    try { setSummary(await window.localPinyin.clearClipboard()); setClearing(false); notify('剪贴板记录已清空'); }
    catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <>
    <PageHeader kicker="gong'ju" title="工具面板" description="连按两下 Tab 打开：符号、颜文字、表情、表情包、剪贴板和计算等小工具。" action={<SaveStatus saving={saving} />} />
    <SettingsGroup id="group-panel" title="打开方式">
      <SettingRow title="连按两下 Tab 打开工具面板" description="在输入框里快速按两次 Tab 打开面板，选好的内容会输入到原来的位置。只按一次 Tab 还是普通的 Tab（或翻译）。"
        demo={<KeyDemo before="你好" keys={['Tab', 'Tab']} output=" → 工具面板" />}>
        <Switch checked={settings.panelDoubleTab} disabled={saving} label="连按两下 Tab 打开工具面板"
          onChange={panelDoubleTab => void saveSettings({ ...settings, panelDoubleTab })} />
      </SettingRow>
      <SettingRow title="面板在后台待命" description={settings.panelResident
          ? '面板一直在后台准备好，打开更快，会多占用一些内存。'
          : '默认在最后一次使用 10 分钟后自动退出，不占内存；开启后一直在后台，打开更快。'}>
        <Switch checked={settings.panelResident} disabled={saving} label="面板在后台待命"
          onChange={panelResident => void saveSettings({ ...settings, panelResident })} />
      </SettingRow>
      <SettingRow title="用过的表情出现在候选" description="在面板里用过的表情，之后打它的名字时排在第三个候选，例如 kaixin → 😄。可以在“学习 › 输入习惯”里逐个删掉。">
        <Switch checked={settings.emojiCandidates} disabled={saving} label="用过的表情出现在候选"
          onChange={emojiCandidates => void saveSettings({ ...settings, emojiCandidates })} />
      </SettingRow>
    </SettingsGroup>
    <SettingsGroup id="group-clipboard" title="剪贴板">
      <SettingRow title="记住复制过的文字和图片" description="在面板的“剪贴板”里找回、搜索（图片里的文字也能搜到）并再次粘贴。只保存在这台电脑，不会上传；密码管理器等标记为不记录的内容会跳过。">
        <Switch checked={recording} disabled={saving} label="记住复制过的文字和图片"
          onChange={on => void saveSettings({ ...settings, clipboardHistory: on ? 'on' : 'off' })} />
      </SettingRow>
      <SettingRow title="保存时间" description={settings.clipboardRetentionDays === 0 ? '一直保存，直到你删除或清空。' : `超过 ${retentionLabel(settings.clipboardRetentionDays)}的记录会自动删除。`}>
        <div className="segmented" role="radiogroup" aria-label="剪贴板保存时间">{clipboardRetentionChoices.map(days =>
          <label key={days} className={settings.clipboardRetentionDays === days ? 'active' : undefined}>
            <input type="radio" name="clipboard-retention" value={days} checked={settings.clipboardRetentionDays === days} disabled={saving}
              onChange={() => void saveSettings({ ...settings, clipboardRetentionDays: days })} />{retentionLabel(days)}
          </label>)}
        </div>
      </SettingRow>
      <SettingRow title="已保存的记录" description={summary && summary.items
          ? `共 ${formatNumber(summary.items)} 条${summary.images ? `，其中图片 ${formatNumber(summary.images)} 张` : ''}，占用 ${size(summary.bytes)}。`
          : '还没有记录。'}>
        <button className="button secondary danger-text" disabled={!summary?.items} onClick={() => { setError(null); setClearing(true); }}>
          <Icon name="trash" size={15} />清空记录</button>
      </SettingRow>
    </SettingsGroup>
    {clearing && <Modal title="清空剪贴板记录？" description="所有保存的文字和图片都会删除，无法恢复。" busy={pending} onClose={() => setClearing(false)}>
      <div className="modal-body"><ErrorNotice message={error} /></div>
      <footer className="modal-footer"><button className="button secondary" disabled={pending} onClick={() => setClearing(false)}>取消</button>
        <button className="button danger" disabled={pending} onClick={() => void clear()}>{pending ? '正在清空…' : '清空'}</button></footer>
    </Modal>}
  </>;
}

// 输入习惯: words tied to text in the panel (美元 → $), and emoji offered for
// their names. Each can be added here too, or deleted.
export function Habits({ notify }: { notify: (message: string) => void }) {
  const [phrases, setPhrases] = useState<Phrase[] | null>(null);
  const [emoji, setEmoji] = useState<Phrase[]>([]);
  const [query, setQuery] = useState('');
  const [text, setText] = useState('');
  const [label, setLabel] = useState('');
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  useEffect(() => {
    window.localPinyin.listPhrases().then(result => setPhrases(result.phrases)).catch(failure => setError(asError(failure)));
    window.localPinyin.listEmojiCandidates().then(result => setEmoji(result.phrases)).catch(() => undefined);
  }, []);
  const add = async (event: React.FormEvent) => {
    event.preventDefault();
    const content = text.trim(), words = label.split(/[,，、\s]+/).filter(Boolean);
    if (!content || content.length > 64) { setError('请填写内容，最多 64 个字符。'); return; }
    if (!words.length || words.some(word => !/^([㐀-鿿]{1,32}|[A-Za-z]{1,32})$/.test(word))) {
      setError('短语只能是汉字，或只用英文字母，最多 32 个；多个短语用逗号隔开。'); return;
    }
    setPending(true); setError(null);
    try {
      let result: { phrases: Phrase[] } | null = null;
      for (const word of words) result = await window.localPinyin.addPhrase(content, word);
      if (result) setPhrases(result.phrases);
      setText(''); setLabel('');
      notify(`已添加：输入“${words.join('”“')}”时，${content} 排在第一`);
    } catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  const remove = async (phrase: Phrase) => {
    try { setPhrases((await window.localPinyin.deletePhrase(phrase.pinyin, phrase.text)).phrases); notify('短语已删除'); }
    catch (failure) { setError(asError(failure)); }
  };
  const removeEmoji = async (phrase: Phrase) => {
    try { setEmoji((await window.localPinyin.deleteEmojiCandidate(phrase.pinyin, phrase.text)).phrases); }
    catch (failure) { setError(asError(failure)); }
  };
  const needle = query.trim().toLowerCase();
  const shown = (phrases ?? []).filter(phrase => !needle || phrase.label.toLowerCase().includes(needle) ||
    phrase.pinyin.includes(needle) || phrase.text.toLowerCase().includes(needle)).sort((a, b) => b.created - a.created);
  return <section className="collection habits" aria-labelledby="habits-title">
    <div className="collection-toolbar">
      <div className="collection-title"><h2 id="habits-title">输入习惯</h2><span className="count-pill">{formatNumber(phrases?.length ?? 0)}</span></div>
      <div className="search-field"><Icon name="search" size={15} /><input className="input-field" aria-label="搜索输入习惯" placeholder="搜索短语或内容"
        value={query} onChange={event => setQuery(event.target.value)} />{query && <button aria-label="清空搜索" className="search-clear" onClick={() => setQuery('')}><Icon name="close" size={13} /></button>}</div>
    </div>
    <p className="habits-intro">打这些短语时，对应的内容排第一个候选；选了别的，它也只退到第二位，直到在这里删掉。工具面板里点“添加短语”也会加到这里。</p>
    <form className="habit-form" onSubmit={event => void add(event)}>
      <input className="input-field" aria-label="内容" placeholder="内容，例如 $ 或 ¥" value={text} maxLength={64} onChange={event => setText(event.target.value)} />
      <Icon name="arrow" size={15} style={{ transform: 'rotate(180deg)' }} />
      <input className="input-field" aria-label="短语" placeholder="短语，例如 美元，美刀" value={label} maxLength={120} onChange={event => setLabel(event.target.value)} />
      <button className="button primary" disabled={pending || !text.trim() || !label.trim()}><Icon name="plus" size={15} />{pending ? '正在添加…' : '添加'}</button>
    </form>
    <ErrorNotice message={error} />
    <div className="table-scroll">
      <table className="lexicon-table">
        <thead><tr><th>短语</th><th>拼音</th><th>内容</th><th className="actions-column"><span className="visually-hidden">操作</span></th></tr></thead>
        <tbody>{shown.slice(0, 200).map(phrase => <tr key={`${phrase.pinyin}\0${phrase.text}`}>
          <td className="word-cell">{phrase.label}</td>
          <td className="pinyin-cell">{phrase.pinyin}</td>
          <td className="habit-text">{phrase.text}</td>
          <td className="actions-column"><span className="row-actions">
            <button className="table-action delete-action" aria-label={`删除短语 ${phrase.label}`} title="删除" onClick={() => void remove(phrase)}><Icon name="trash" size={15} /></button>
          </span></td>
        </tr>)}</tbody>
      </table>
      {phrases && !shown.length && <div className="empty-state compact">
        <h3>{needle ? '没有找到' : '还没有输入习惯'}</h3>
        <p>{needle ? '试试别的短语或内容。' : '例如内容填 $，短语填“美元，美刀”，以后打 meiyuan 第一个就是 $。'}</p>
      </div>}
    </div>
    {emoji.length > 0 && <div className="habit-emoji">
      <h3>候选里的表情<span>在工具面板用过的表情，打这些名字时排第三</span></h3>
      <ul>{emoji.slice().sort((a, b) => b.created - a.created).slice(0, 120).map(item =>
        <li key={`${item.pinyin}\0${item.text}`} className="chip-item" title={item.pinyin}>
          <span className="habit-emoji-glyph">{item.text}</span>{item.label}
          <button aria-label={`不再在“${item.label}”的候选里出现 ${item.text}`} onClick={() => void removeEmoji(item)}><Icon name="close" size={12} /></button>
        </li>)}
      </ul>
    </div>}
  </section>;
}

// Asked once after installing or updating: the clipboard history keeps what
// the user copies, so it starts only with their yes.
export function ClipboardQuestion({ settings, saveSettings, onClose }: {
  settings: Settings; saveSettings: (settings: Settings) => Promise<void>; onClose: () => void;
}) {
  const answer = async (on: boolean) => {
    await saveSettings({ ...settings, clipboardHistory: on ? 'on' : 'off' });
    onClose();
  };
  return <Modal title="要记住复制过的内容吗？" description="新功能：工具面板里的剪贴板（连按两下 Tab 打开）。" onClose={onClose}>
    <div className="modal-body clipboard-question">
      <span className="question-icon"><Icon name="clipboard" size={24} /></span>
      <ul>
        <li>记下你复制的文字和图片，在面板里随时找回、搜索、再次粘贴；图片里的文字也能搜到。</li>
        <li>只保存在这台电脑，不会上传。密码管理器等标记为不记录的内容会跳过。</li>
        <li>默认保存 7 天，可以在设置的“工具面板”里改成 1 天到永久，也能随时关闭或清空。</li>
      </ul>
    </div>
    <footer className="modal-footer">
      <button className="button secondary" onClick={() => void answer(false)}>不用了</button>
      <button className="button primary" data-autofocus onClick={() => void answer(true)}>开启剪贴板记录</button>
    </footer>
  </Modal>;
}
