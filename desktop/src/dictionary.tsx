import { useEffect, useRef, useState } from 'react';
import { Icon } from './icons';
import { asError, ErrorNotice, formatNumber, Modal, PageHeader } from './components';
import type { AppState, DictionaryEntry, UserEntry } from './types';

export function DictionaryPage({ state, entries, onEntries, loadError, notify }: { state: AppState; entries: UserEntry[]; onEntries: (entries: UserEntry[]) => void; loadError: string | null; notify: (message: string) => void }) {
  const [query, setQuery] = useState('');
  const [editor, setEditor] = useState<{ previous?: UserEntry } | null>(null);
  const [deleting, setDeleting] = useState<UserEntry | null>(null);
  const [browse, setBrowse] = useState(false);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
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
    <PageHeader eyebrow="把自己的表达收进来" title="我的词库" description="添加人名、常用短语和专业词语，让输入更合你的习惯。" action={<button className="button primary" onClick={() => setEditor({})}><Icon name="plus" size={18} />添加词语</button>} />
    <div className="dictionary-overview"><div className="card dictionary-stat"><span className="stat-icon"><Icon name="dictionary" size={22} /></span><div><span className="stat-label">内置词库</span><strong>{formatNumber(state.dictionary.total)}<span>条</span></strong></div><button className="icon-button" aria-label="浏览内置词库" title="浏览内置词库" onClick={() => setBrowse(true)}><Icon name="arrow" size={20} /></button></div><div className="card dictionary-stat personal"><span className="stat-icon"><Icon name="edit" size={22} /></span><div><span className="stat-label">个人词库</span><strong>{formatNumber(entries.length)}<span>条</span></strong></div><span className="small-badge">本机保存</span></div></div>
    <div className="card lexicon-card"><div className="lexicon-toolbar"><div><h2>个人词语</h2><span>你添加的每一个词，都在这里。</span></div><div className="search-field"><Icon name="search" size={17} /><input aria-label="搜索个人词库" placeholder="搜索词语或拼音" value={query} onChange={event => setQuery(event.target.value)} />{query && <button aria-label="清空个人词库搜索" className="search-clear" onClick={() => setQuery('')}><Icon name="close" size={14} /></button>}</div></div><ErrorNotice message={loadError} /><div className="table-scroll"><table className="lexicon-table"><thead><tr><th>中文词语</th><th>拼音</th><th className="frequency-column">词频</th><th className="actions-column">操作</th></tr></thead><tbody>{visible.map(entry => <tr key={`${entry.pinyin}\0${entry.word}`}><td className="word-cell">{entry.word}</td><td className="pinyin-cell">{entry.pinyin}</td><td className="frequency-column">{formatNumber(entry.frequency)}</td><td className="actions-column"><button className="table-action" aria-label={`编辑 ${entry.word}`} title="编辑" onClick={() => setEditor({ previous: entry })}><Icon name="edit" size={17} /></button><button className="table-action delete-action" aria-label={`删除 ${entry.word}`} title="删除" onClick={() => { setError(null); setDeleting(entry); }}><Icon name="trash" size={17} /></button></td></tr>)}</tbody></table>{!visible.length && <div className="empty-state"><span className="empty-state-icon"><Icon name={query ? 'search' : 'dictionary'} size={29} /></span><h3>{query ? '没有找到这个词语' : '给你的词库添加第一个词'}</h3><p>{query ? '试试其他词语或拼音。' : '名字、工作用语、常用短语，都可以从这里开始。'}</p>{!query && <button className="button secondary compact" onClick={() => setEditor({})}><Icon name="plus" size={16} />添加词语</button>}</div>}</div><footer className="table-footer"><span>{query ? `找到 ${formatNumber(filtered.length)} 条词语` : `共 ${formatNumber(entries.length)} 条个人词语`}{filtered.length > 100 && '，仅显示前 100 条，请搜索定位'}</span><span>添加后，重新打开输入应用即可使用</span></footer></div>
    <button className="browse-card" onClick={() => setBrowse(true)}><span className="browse-icon"><Icon name="dictionary" size={23} /></span><span><strong>浏览内置词库</strong><span>看看哪些词语已经收录，按中文或拼音查找。</span></span><Icon name="chevron" size={19} /></button>
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
  return <Modal title={previous ? '编辑词语' : '添加词语'} description="用你习惯的拼音，快速找到常用表达。" busy={pending} onClose={onClose}><form onSubmit={event => void save(event)}><div className="modal-body entry-form"><label htmlFor="entry-word">中文词语</label><input id="entry-word" data-autofocus value={word} maxLength={128} placeholder="例如：青石科技" disabled={pending} onChange={event => setWord(event.target.value)} autoComplete="off" /><label htmlFor="entry-pinyin">拼音</label><input id="entry-pinyin" value={pinyin} maxLength={256} placeholder="例如：qingshikeji" disabled={pending} onChange={event => setPinyin(event.target.value)} spellCheck={false} autoComplete="off" /><p className="field-hint">使用连续拼音，不带声调和空格；ü 请用 v。</p><label htmlFor="entry-frequency">词频 <span>影响相同拼音下的候选顺序</span></label><input id="entry-frequency" type="number" inputMode="numeric" value={frequency} min="0" max="1000000" step="1" disabled={pending} onChange={event => setFrequency(event.target.value)} /><p className="field-hint">推荐 5,000。调高后，这个词会更容易排在前面。</p><ErrorNotice message={error} /></div><footer className="modal-footer"><button type="button" className="button secondary" disabled={pending} onClick={onClose}>取消</button><button type="submit" className="button primary" disabled={pending}>{pending ? '正在保存…' : '保存词语'}</button></footer></form></Modal>;
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
  return <Modal title="内置词库" description={`${formatNumber(state.dictionary.total)} 条本地词语，随时查找。`} wide onClose={onClose}><div className="modal-body dictionary-browser"><div className="search-field wide-search"><Icon name="search" size={19} /><input aria-label="搜索内置词库" data-autofocus placeholder="输入中文或拼音，如 随便 / suibian" value={query} maxLength={128} onChange={event => setQuery(event.target.value)} />{query && <button aria-label="清空内置词库搜索" className="search-clear" onClick={() => setQuery('')}><Icon name="close" size={16} /></button>}</div><div className="browser-result-label" role="status">{pending ? '正在本地查找…' : query.trim() ? `找到 ${formatNumber(total)} 条词语${total > entries.length ? `，显示前 ${entries.length} 条` : ''}` : '支持中文词语和拼音搜索'}</div><ErrorNotice message={error} /><div className="browser-table-scroll" aria-busy={pending}><table className="lexicon-table"><thead><tr><th>中文词语</th><th>拼音</th><th className="frequency-column">词频</th><th>来源</th></tr></thead><tbody>{entries.map(entry => <tr key={`${entry.pinyin}\0${entry.word}`}><td className="word-cell">{entry.word}</td><td className="pinyin-cell">{entry.pinyin}</td><td className="frequency-column">{formatNumber(entry.frequency)}</td><td><span className="source-tag">{sources[entry.source] ?? entry.source}</span></td></tr>)}</tbody></table>{!entries.length && <div className="empty-state"><span className="empty-state-icon"><Icon name="search" size={29} /></span><h3>{pending ? '正在查找词语' : query.trim() ? '暂时没有找到' : '从一个词语开始'}</h3><p>{query.trim() ? '可以换一个中文词语或拼音再试试。' : '输入中文或拼音，查看已经收录的词语。'}</p></div>}</div></div><footer className="modal-footer browser-footer"><span><Icon name="shield" size={15} />查询完全在本机完成</span><button className="button secondary" onClick={onClose}>关闭</button></footer></Modal>;
}
