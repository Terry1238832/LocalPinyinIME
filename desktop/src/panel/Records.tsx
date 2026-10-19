// 记录: addresses, accounts with their passwords, card numbers, anything the
// user wants to type again. Encrypted on disk; with a password it opens only
// after typing it. Items can be added, renamed, edited and deleted here; each
// value goes into the original application or onto the clipboard.
import { useEffect, useMemo, useRef, useState } from 'react';
import { Icon } from '../icons';
import type { VaultField, VaultItem, VaultStatus } from './types';

type Kind = VaultItem['kind'];
const KINDS: { id: Kind | 'all'; label: string }[] = [
  { id: 'all', label: '全部' }, { id: 'password', label: '密码' }, { id: 'address', label: '地址' }, { id: 'other', label: '其他' }];
const KIND_LABEL: Record<Kind, string> = { password: '密码', address: '地址', other: '其他' };
// What each kind starts with; more items can be added to any.
const TEMPLATES: Record<Kind, VaultField[]> = {
  password: [{ name: '邮箱/账号', value: '', secret: false }, { name: '密码', value: '', secret: true },
    { name: '网址', value: '', secret: false }, { name: '备注', value: '', secret: false }],
  address: [{ name: '收件人', value: '', secret: false }, { name: '电话', value: '', secret: false },
    { name: '地址', value: '', secret: false }, { name: '邮编', value: '', secret: false }],
  other: [{ name: '内容', value: '', secret: false }],
};
const api = () => window.localPinyinPanel;

function summary(item: VaultItem) {
  const field = item.fields.find(entry => entry.value);
  if (!field) return '';
  return field.secret ? '••••••' : field.value.split('\n')[0];
}

export function Records({ search, onError, onNotice }: {
  search: string; onError: (error: unknown) => void; onNotice: (message: string) => void;
}) {
  const [status, setStatus] = useState<VaultStatus | null>(null);
  const [items, setItems] = useState<VaultItem[] | null>(null);
  const [kind, setKind] = useState<Kind | 'all'>('all');
  const [selected, setSelected] = useState<string | null>(null);
  const [editing, setEditing] = useState<VaultItem | null>(null);
  const [deleting, setDeleting] = useState<VaultItem | null>(null);
  const [revealed, setRevealed] = useState<Set<string>>(new Set());

  useEffect(() => {
    api().vaultStatus().then(async next => {
      setStatus(next);
      if (next.exists && next.unlocked) setItems(await api().vaultItems());
      else if (next.exists && !next.password) setItems(await api().vaultUnlock(''));
    }).catch(onError);
  }, [onError]);

  const query = search.trim().toLowerCase();
  const shown = useMemo(() => (items ?? []).filter(item => (kind === 'all' || item.kind === kind) &&
    (!query || item.title.toLowerCase().includes(query) ||
      item.fields.some(field => !field.secret && field.value.toLowerCase().includes(query)))), [items, kind, query]);
  const current = items?.find(item => item.id === selected) ?? shown[0] ?? null;

  if (!status) return <div className="records loading" aria-busy="true"><div className="spinner" /></div>;
  if (!status.exists) return <Setup onDone={next => { setStatus({ exists: true, password: next.password, unlocked: true }); setItems(next.items); }} onError={onError} />;
  if (!items) return <Unlock onOpened={list => { setItems(list); setStatus({ ...status, unlocked: true }); }} onError={onError} />;

  const save = async (item: VaultItem) => {
    try {
      const list = await api().vaultSave(item);
      setItems(list);
      setEditing(null);
      const saved = item.id ? list.find(entry => entry.id === item.id) : list.find(entry => entry.title === item.title);
      if (saved) setSelected(saved.id);
      onNotice(item.id ? '已保存' : '已添加');
    } catch (error) { onError(error); }
  };
  const remove = async (item: VaultItem) => {
    try {
      setItems(await api().vaultDelete(item.id));
      setDeleting(null);
      setSelected(null);
      onNotice(`已删除“${item.title}”`);
    } catch (error) { onError(error); }
  };
  const copy = (field: VaultField) => {
    api().vaultCopy(field.value).then(() => onNotice(field.secret ? '已复制，45 秒后从剪贴板清除' : '已复制')).catch(onError);
  };
  const insert = (field: VaultField) => { api().insert(field.value).catch(onError); };
  const lock = () => { api().vaultLock().then(() => { setItems(null); setRevealed(new Set()); }).catch(onError); };

  return (
    <div className="records">
      <div className="category-bar records-bar">
        <div className="category-tabs" role="tablist" aria-label="记录分类">
          {KINDS.map(entry => (
            <button key={entry.id} type="button" role="tab" aria-selected={kind === entry.id}
                    className={kind === entry.id ? 'category active' : 'category'} onClick={() => setKind(entry.id)}>
              {entry.label}
            </button>
          ))}
        </div>
        <button type="button" className="text-button" onClick={() => setEditing({ id: '', kind: kind === 'all' ? 'password' : kind,
          title: '', fields: TEMPLATES[kind === 'all' ? 'password' : kind].map(field => ({ ...field })), created: 0, updated: 0 })}>
          <Icon name="plus" size={14} />添加</button>
        {status.password && <button type="button" className="text-button" onClick={lock}><Icon name="lock" size={14} />锁定</button>}
      </div>
      <div className="records-body">
        <ul className="record-list" aria-label="记录">
          {shown.map(item => (
            <li key={item.id}>
              <button type="button" className={current?.id === item.id ? 'record-row selected' : 'record-row'}
                      onClick={() => setSelected(item.id)}>
                <span className="record-title">{item.title}</span>
                <span className={`record-tag ${item.kind}`}>{KIND_LABEL[item.kind]}</span>
                <span className="record-summary">{summary(item)}</span>
              </button>
            </li>
          ))}
          {!shown.length && <li className="empty-note">{items.length ? '没有符合的记录。' : '还没有记录。点“添加”存一个地址或账号密码。'}</li>}
        </ul>
        {current && (
          <section className="record-detail" aria-label={current.title}>
            <header>
              <h2>{current.title}</h2>
              <span className={`record-tag ${current.kind}`}>{KIND_LABEL[current.kind]}</span>
              <span className="drag-space" />
              <button type="button" className="icon-button" title="编辑（改标题、改内容）" aria-label={`编辑 ${current.title}`}
                      onClick={() => setEditing({ ...current, fields: current.fields.map(field => ({ ...field })) })}><Icon name="edit" size={15} /></button>
              <button type="button" className="icon-button" title="删除" aria-label={`删除 ${current.title}`}
                      onClick={() => setDeleting(current)}><Icon name="trash" size={15} /></button>
            </header>
            <dl className="record-fields">
              {current.fields.filter(field => field.value).map((field, index) => {
                const key = `${current.id}:${index}`;
                const visible = !field.secret || revealed.has(key);
                return (
                  <div key={key} className="record-field">
                    <dt>{field.name}</dt>
                    <dd className={field.secret && !visible ? 'masked' : undefined}>{visible ? field.value : '••••••••'}</dd>
                    <span className="record-actions">
                      {field.secret && <button type="button" className="text-button" onClick={() => setRevealed(set => {
                        const next = new Set(set);
                        if (next.has(key)) next.delete(key); else next.add(key);
                        return next;
                      })}>{visible ? '隐藏' : '显示'}</button>}
                      <button type="button" className="text-button" onClick={() => copy(field)}>复制</button>
                      <button type="button" className="button secondary compact" onClick={() => insert(field)}>输入</button>
                    </span>
                  </div>
                );
              })}
            </dl>
          </section>
        )}
      </div>
      {editing && <Editor item={editing} onCancel={() => setEditing(null)} onSave={item => void save(item)} />}
      {deleting && (
        <div className="dialog-scrim" onMouseDown={event => { if (event.target === event.currentTarget) setDeleting(null); }}>
          <div className="dialog" role="dialog" aria-modal="true" aria-labelledby="record-delete-title">
            <h2 id="record-delete-title">删除“{deleting.title}”？</h2>
            <p className="dialog-text">删除后无法恢复。</p>
            <div className="dialog-actions">
              <button type="button" className="button secondary" onClick={() => setDeleting(null)}>取消</button>
              <button type="button" className="button danger" onClick={() => void remove(deleting)}>删除</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

function Setup({ onDone, onError }: { onDone: (result: { password: boolean; items: VaultItem[] }) => void; onError: (error: unknown) => void }) {
  const [password, setPassword] = useState('');
  const [again, setAgain] = useState('');
  const [busy, setBusy] = useState(false);
  const [problem, setProblem] = useState('');
  const field = useRef<HTMLInputElement>(null);
  useEffect(() => { field.current?.focus(); }, []);
  const create = async (value: string) => {
    if (value && value !== again) { setProblem('两次输入的密码不一样。'); return; }
    setBusy(true);
    try { onDone({ password: !!value, items: await api().vaultCreate(value) }); }
    catch (error) { onError(error); setBusy(false); }
  };
  return (
    <div className="records consent">
      <form className="consent-card records-setup" onSubmit={event => { event.preventDefault(); if (password) void create(password); }}>
        <span className="consent-icon"><Icon name="lock" size={24} /></span>
        <h2>第一次使用记录</h2>
        <p>记录可以存地址、账号密码、卡号或任何想再次输入的内容，加密保存在这台电脑上。
          设一个密码后，每次打开都要先输入它；忘记密码就无法找回。</p>
        <input ref={field} className="text-field" type="password" placeholder="设置密码" aria-label="设置密码" value={password}
               maxLength={128} onChange={event => { setPassword(event.target.value); setProblem(''); }} />
        <input className="text-field" type="password" placeholder="再输入一次" aria-label="再输入一次密码" value={again}
               maxLength={128} onChange={event => { setAgain(event.target.value); setProblem(''); }} />
        {problem && <p className="field-problem" role="alert">{problem}</p>}
        <div className="consent-actions">
          <button type="submit" className="button primary" disabled={busy || !password}>设置密码</button>
          <button type="button" className="button secondary" disabled={busy} onClick={() => void create('')}>不设密码</button>
        </div>
        <small>不设密码时，记录只用这台电脑的 Windows 账户保护；以后可以在设置里加密码。</small>
      </form>
    </div>
  );
}

function Unlock({ onOpened, onError }: { onOpened: (items: VaultItem[]) => void; onError: (error: unknown) => void }) {
  const [password, setPassword] = useState('');
  const [busy, setBusy] = useState(false);
  const field = useRef<HTMLInputElement>(null);
  useEffect(() => { field.current?.focus(); }, []);
  const open = async () => {
    setBusy(true);
    try { onOpened(await api().vaultUnlock(password)); }
    catch (error) { onError(error); setPassword(''); setBusy(false); field.current?.focus(); }
  };
  return (
    <div className="records consent">
      <form className="consent-card records-setup" onSubmit={event => { event.preventDefault(); if (password) void open(); }}>
        <span className="consent-icon"><Icon name="lock" size={24} /></span>
        <h2>记录已加密</h2>
        <p>输入密码后打开。打开后 5 分钟内不再询问。</p>
        <input ref={field} className="text-field" type="password" placeholder="密码" aria-label="记录密码" value={password}
               maxLength={128} onChange={event => setPassword(event.target.value)} />
        <div className="consent-actions">
          <button type="submit" className="button primary" disabled={busy || !password}>{busy ? '正在打开…' : '打开'}</button>
        </div>
      </form>
    </div>
  );
}

function Editor({ item, onCancel, onSave }: { item: VaultItem; onCancel: () => void; onSave: (item: VaultItem) => void }) {
  const [draft, setDraft] = useState<VaultItem>(item);
  const title = useRef<HTMLInputElement>(null);
  useEffect(() => { title.current?.focus(); }, []);
  const setField = (index: number, patch: Partial<VaultField>) =>
    setDraft(current => ({ ...current, fields: current.fields.map((field, at) => at === index ? { ...field, ...patch } : field) }));
  const changeKind = (kind: Kind) => setDraft(current => {
    // A new item takes the kind's fields; one being edited keeps what it has.
    const empty = current.fields.every(field => !field.value);
    return { ...current, kind, fields: !current.id && empty ? TEMPLATES[kind].map(field => ({ ...field })) : current.fields };
  });
  const valid = draft.title.trim() && draft.fields.every(field => field.name.trim());
  return (
    <div className="dialog-scrim" onMouseDown={event => { if (event.target === event.currentTarget) onCancel(); }}>
      <form className="dialog record-editor" role="dialog" aria-modal="true" aria-labelledby="record-editor-title"
            onSubmit={event => {
              event.preventDefault();
              if (valid) onSave({ ...draft, title: draft.title.trim(), fields: draft.fields.filter(field => field.value) });
            }}
            onKeyDown={event => { if (event.key === 'Escape') { event.stopPropagation(); onCancel(); } }}>
        <h2 id="record-editor-title">{item.id ? '编辑记录' : '添加记录'}</h2>
        <div className="segmented-row" role="radiogroup" aria-label="类型">
          {(['password', 'address', 'other'] as Kind[]).map(kind => (
            <button key={kind} type="button" role="radio" aria-checked={draft.kind === kind}
                    className={draft.kind === kind ? 'category active' : 'category'} onClick={() => changeKind(kind)}>{KIND_LABEL[kind]}</button>
          ))}
        </div>
        <label className="editor-line"><span>标题</span>
          <input ref={title} className="text-field" value={draft.title} maxLength={100} placeholder="例如 QQ 邮箱、家、工资卡"
                 onChange={event => setDraft(current => ({ ...current, title: event.target.value }))} /></label>
        <div className="editor-fields">
          {draft.fields.map((field, index) => (
            <div key={index} className="editor-field">
              <input className="text-field name" value={field.name} maxLength={32} aria-label="名称"
                     onChange={event => setField(index, { name: event.target.value })} />
              <input className="text-field value" value={field.value} maxLength={4000} aria-label={field.name || '内容'}
                     type={field.secret ? 'password' : 'text'} onChange={event => setField(index, { value: event.target.value })} />
              <label className="secret-toggle" title="显示为圆点，复制后自动清除">
                <input type="checkbox" checked={field.secret} onChange={event => setField(index, { secret: event.target.checked })} />隐藏</label>
              <button type="button" className="icon-button" aria-label={`去掉 ${field.name}`}
                      onClick={() => setDraft(current => ({ ...current, fields: current.fields.filter((_, at) => at !== index) }))}>
                <Icon name="close" size={12} /></button>
            </div>
          ))}
          <button type="button" className="text-button add-field" disabled={draft.fields.length >= 32}
                  onClick={() => setDraft(current => ({ ...current, fields: [...current.fields, { name: '', value: '', secret: false }] }))}>
            <Icon name="plus" size={13} />添加一项</button>
        </div>
        <div className="dialog-actions">
          <button type="button" className="button secondary" onClick={onCancel}>取消</button>
          <button type="submit" className="button primary" disabled={!valid}>保存</button>
        </div>
      </form>
    </div>
  );
}
