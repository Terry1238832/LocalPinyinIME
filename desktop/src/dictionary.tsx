import { useEffect, useRef, useState } from 'react';
import { Icon } from './icons';
import { asError, ErrorNotice, formatNumber, MiniCandidates, Modal, PageHeader } from './components';
import { usePageCommand } from './CommandPalette';
import type { AppState, DictionaryEntry, UserEntry } from './types';

const layers: { key: 'core' | 'local' | 'standard'; label: string; description: string }[] = [
  { key: 'local', label: '常用词库', description: '项目整理的常用字词' },
  { key: 'standard', label: '标准词库', description: 'Rime 拼音词库' },
  { key: 'core', label: '基础词库', description: '最高频的字与词' },
];

// One series (entries per layer): a single hue, values labelled directly.
function LayerBars({ state }: { state: AppState }) {
  const values = layers.map(layer => state.dictionary[layer.key]);
  const largest = Math.max(1, ...values);
  const sum = values.reduce((total, value) => total + value, 0) || 1;
  return <ul className="layer-bars" aria-label="内置词库构成">{layers.map((layer, index) => {
    const value = values[index];
    const share = (value / sum) * 100;
    return <li key={layer.key} title={`${layer.label}（${layer.description}）：${formatNumber(value)} 条，占 ${share < 1 ? share.toFixed(1) : Math.round(share)}%`}>
      <span className="layer-name">{layer.label}</span>
      <span className="layer-track"><span className="layer-bar" style={{ width: `${Math.max(0.6, (value / largest) * 100)}%` }} /></span>
      <span className="layer-value">{formatNumber(value)}</span>
    </li>;
  })}</ul>;
}

export function DictionaryPage({ state, entries, onEntries, loadError, notify }: { state: AppState; entries: UserEntry[]; onEntries: (entries: UserEntry[]) => void; loadError: string | null; notify: (message: string) => void }) {
  const [query, setQuery] = useState('');
  const [editor, setEditor] = useState<{ previous?: UserEntry } | null>(null);
  const [deleting, setDeleting] = useState<UserEntry | null>(null);
  const [browse, setBrowse] = useState(false);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  usePageCommand('add-word', () => setEditor({}));
  usePageCommand('browse-dictionary', () => setBrowse(true));
  const normalizedQuery = query.trim().toLowerCase();
  const filtered = entries.filter(entry => !normalizedQuery || entry.word.toLowerCase().includes(normalizedQuery) || entry.pinyin.toLowerCase().includes(normalizedQuery));
  const visible = filtered.slice(0, 100);
  const remove = async () => {
    if (!deleting) return;
    setPending(true); setError(null);
    try {
      const result = await window.localPinyin.deleteUserEntry(deleting);
      onEntries(result.entries); setDeleting(null); notify('词语已从个人词库删除');
    } catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <>
    <PageHeader kicker="ci'ku" title="词库" description="添加人名、常用短语和专业词语，让它们更容易打出来。" action={<>
      <button className="button secondary" aria-label="浏览内置词库" onClick={() => setBrowse(true)}><Icon name="search" size={15} />查找内置词库</button>
      <button className="button primary" onClick={() => setEditor({})}><Icon name="plus" size={16} />添加词语</button>
    </>} />
    <section className="ledger">
    <div className="metrics" aria-label="词库概况" role="group">
      <div className="metric metric-builtin">
        <span className="eyebrow"><Icon name="database" size={14} />内置词语</span>
        <strong className="metric-value">{formatNumber(state.dictionary.total)}</strong>
        <LayerBars state={state} />
      </div>
      <div className="metric">
        <span className="eyebrow"><Icon name="user" size={14} />个人词语</span>
        <strong className="metric-value">{formatNumber(entries.length)}</strong>
        <p className="metric-note">排在候选前列，只保存在本机。</p>
      </div>
    </div>
    <div className="collection">
      <div className="collection-toolbar">
        <div className="collection-title"><h2>个人词语</h2><span className="count-pill">{formatNumber(entries.length)}</span></div>
        <div className="search-field"><Icon name="search" size={15} /><input className="input-field" aria-label="搜索个人词库" placeholder="搜索词语或拼音" value={query} onChange={event => setQuery(event.target.value)} />{query && <button aria-label="清空个人词库搜索" className="search-clear" onClick={() => setQuery('')}><Icon name="close" size={13} /></button>}</div>
      </div>
      <ErrorNotice message={loadError} />
      <div className="table-scroll">
        <table className="lexicon-table">
          <thead><tr><th>中文词语</th><th>拼音</th><th className="frequency-column">词频</th><th className="actions-column"><span className="visually-hidden">操作</span></th></tr></thead>
          <tbody>{visible.map(entry => <tr key={`${entry.pinyin}\0${entry.word}`}>
            <td className="word-cell">{entry.word}</td>
            <td className="pinyin-cell">{entry.pinyin}</td>
            <td className="frequency-column">{formatNumber(entry.frequency)}</td>
            <td className="actions-column"><span className="row-actions">
              <button className="table-action" aria-label={`编辑 ${entry.word}`} title="编辑" onClick={() => setEditor({ previous: entry })}><Icon name="edit" size={15} /></button>
              <button className="table-action delete-action" aria-label={`删除 ${entry.word}`} title="删除" onClick={() => { setError(null); setDeleting(entry); }}><Icon name="trash" size={15} /></button>
            </span></td>
          </tr>)}</tbody>
        </table>
        {!visible.length && <div className="empty-state">
          <MiniCandidates pinyin={query ? query.slice(0, 12) : 'qingshi'} words={query ? ['…'] : ['青石', '轻视', '清史']} />
          <h3>{query ? '没有找到这个词语' : '还没有个人词语'}</h3>
          <p>{query ? '试试其他词语或拼音。' : '名字、工作用语、常用短语，添加后会排在候选前列。'}</p>
          {!query && <button className="button secondary compact" onClick={() => setEditor({})}><Icon name="plus" size={15} />添加词语</button>}
        </div>}
      </div>
      <footer className="collection-footer"><span>{query ? `找到 ${formatNumber(filtered.length)} 条词语` : `共 ${formatNumber(entries.length)} 条个人词语`}{filtered.length > 100 && '，仅显示前 100 条，请搜索定位'}</span><span>添加后，重新打开正在输入的程序即可使用</span></footer>
    </div>
    </section>
    {editor && <EntryEditor previous={editor.previous} onClose={() => setEditor(null)} onSaved={next => { onEntries(next); setEditor(null); notify(editor.previous ? '词语已更新' : '词语已添加到个人词库'); }} />}
    {deleting && <Modal title="删除这个词语？" description="只会删除这条个人词语，不会删除内置词库中的同名词语。" busy={pending} onClose={() => setDeleting(null)}><div className="modal-body"><div className="delete-entry-preview"><strong>{deleting.word}</strong><span>{deleting.pinyin}</span></div><ErrorNotice message={error} /></div><footer className="modal-footer"><button className="button secondary" disabled={pending} onClick={() => setDeleting(null)}>取消</button><button className="button danger" disabled={pending} onClick={() => void remove()}>{pending ? '正在删除…' : '删除词语'}</button></footer></Modal>}
    {browse && <BuiltInDictionary state={state} onClose={() => setBrowse(false)} />}
  </>;
}

function EntryEditor({ previous, onClose, onSaved }: { previous?: UserEntry; onClose: () => void; onSaved: (entries: UserEntry[]) => void }) {
  const [word, setWord] = useState(previous?.word ?? '');
  const [pinyin, setPinyin] = useState(previous?.pinyin ?? '');
  const [frequency, setFrequency] = useState(String(previous?.frequency ?? 5000));
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const save = async (event: React.FormEvent) => {
    event.preventDefault();
    const normalizedWord = word.trim();
    const normalizedPinyin = pinyin.trim().toLowerCase();
    const parsedFrequency = Number(frequency);
    if (!normalizedWord || normalizedWord.length > 128) { setError('请输入词语，长度不超过 128 个字符。'); return; }
    if (!/^[a-z]+$/.test(normalizedPinyin) || normalizedPinyin.length > 256) { setError('请输入连续的英文字母拼音，不带声调、空格或分隔符。'); return; }
    if (!frequency.trim() || !Number.isInteger(parsedFrequency) || parsedFrequency < 0 || parsedFrequency > 1000000) { setError('词频应为 0 到 1,000,000 之间的整数。'); return; }
    setPending(true); setError(null);
    try {
      const result = await window.localPinyin.upsertUserEntry({ word: normalizedWord, pinyin: normalizedPinyin, frequency: parsedFrequency }, previous);
      onSaved(result.entries);
    } catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };
  return <Modal title={previous ? '编辑词语' : '添加词语'} description="输入词语和它的完整拼音。" busy={pending} onClose={onClose}>
    <form onSubmit={event => void save(event)}>
      <div className="modal-body entry-form">
        <div className="form-field"><label htmlFor="entry-word">中文词语</label><input className="input-field" id="entry-word" data-autofocus value={word} maxLength={128} placeholder="例如：青石科技" disabled={pending} onChange={event => setWord(event.target.value)} autoComplete="off" /></div>
        <div className="form-field"><label htmlFor="entry-pinyin">拼音</label><input className="input-field mono" id="entry-pinyin" value={pinyin} maxLength={256} placeholder="例如：qingshikeji" disabled={pending} onChange={event => setPinyin(event.target.value)} spellCheck={false} autoComplete="off" /><p className="field-hint">连续拼音，不带声调和空格；ü 用 v 代替。</p></div>
        <div className="form-field"><label htmlFor="entry-frequency">词频 <span>影响相同拼音下的候选顺序</span></label><input className="input-field" id="entry-frequency" type="number" inputMode="numeric" value={frequency} min="0" max="1000000" step="1" disabled={pending} onChange={event => setFrequency(event.target.value)} /><p className="field-hint">推荐 5,000。调高后，这个词会更容易排在前面。</p></div>
        <ErrorNotice message={error} />
      </div>
      <footer className="modal-footer"><button type="button" className="button secondary" disabled={pending} onClick={onClose}>取消</button><button type="submit" className="button primary" disabled={pending}>{pending ? '正在保存…' : '保存词语'}</button></footer>
    </form>
  </Modal>;
}

function BuiltInDictionary({ state, onClose }: { state: AppState; onClose: () => void }) {
  const [query, setQuery] = useState('');
  const [entries, setEntries] = useState<DictionaryEntry[]>([]);
  const [total, setTotal] = useState(0);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const request = useRef(0);
  useEffect(() => {
    const id = ++request.current;
    setError(null);
    if (!query.trim()) { setEntries([]); setTotal(0); setPending(false); return; }
    setPending(true);
    const timer = window.setTimeout(() => {
      window.localPinyin.searchDictionary(query.trim(), 100).then(result => {
        if (request.current === id) { setEntries(result.entries); setTotal(result.total); setPending(false); }
      }).catch(failure => {
        if (request.current === id) { setEntries([]); setTotal(0); setError(asError(failure)); setPending(false); }
      });
    }, 180);
    return () => { window.clearTimeout(timer); request.current++; };
  }, [query]);
  const sources: Record<string, string> = { core: '基础词库', local: '常用词库', standard: '标准词库' };
  return <Modal title="内置词库" description={`${formatNumber(state.dictionary.total)} 条本地词语，随时查找。`} wide onClose={onClose}>
    <div className="modal-body dictionary-browser">
      <div className="search-field wide-search"><Icon name="search" size={17} /><input className="input-field" aria-label="搜索内置词库" data-autofocus placeholder="输入中文或拼音，如 随便 / suibian" value={query} maxLength={128} onChange={event => setQuery(event.target.value)} />{query && <button aria-label="清空内置词库搜索" className="search-clear" onClick={() => setQuery('')}><Icon name="close" size={14} /></button>}</div>
      <div className="browser-result-label" role="status">{pending ? '正在本地查找…' : query.trim() ? `找到 ${formatNumber(total)} 条词语${total > entries.length ? `，显示前 ${entries.length} 条` : ''}` : '支持中文词语和拼音搜索'}</div>
      <ErrorNotice message={error} />
      <div className="browser-table-scroll" aria-busy={pending}>
        <table className="lexicon-table">
          <thead><tr><th>中文词语</th><th>拼音</th><th className="frequency-column">词频</th><th className="source-column">来源</th></tr></thead>
          <tbody>{entries.map(entry => <tr key={`${entry.pinyin}\0${entry.word}`}><td className="word-cell">{entry.word}</td><td className="pinyin-cell">{entry.pinyin}</td><td className="frequency-column">{formatNumber(entry.frequency)}</td><td className="source-column"><span className="source-tag">{sources[entry.source] ?? entry.source}</span></td></tr>)}</tbody>
        </table>
        {!entries.length && <div className="empty-state"><span className="empty-state-icon"><Icon name="search" size={22} /></span><h3>{pending ? '正在查找词语' : query.trim() ? '暂时没有找到' : '从一个词语开始'}</h3><p>{query.trim() ? '可以换一个中文词语或拼音再试试。' : '输入中文或拼音，查看已经收录的词语。'}</p></div>}
      </div>
    </div>
    <footer className="modal-footer browser-footer"><span><Icon name="shield" size={14} />查询完全在本机完成</span><button className="button secondary" onClick={onClose}>关闭</button></footer>
  </Modal>;
}
