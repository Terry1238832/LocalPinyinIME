// 导入与导出: the user's data in one .lpbackup file. Export everything at
// once or choose parts; import chooses parts too, and whether each one is
// merged into what is here or replaces it.
import { useState } from 'react';
import { asError, ErrorNotice, formatNumber, Modal } from './components';
import { Icon } from './icons';
import { backupParts, type BackupMode, type BackupPart, type BackupSummary } from './types';

const PARTS: Record<BackupPart, { title: string; detail: string }> = {
  settings: { title: '设置', detail: '外观、输入选项、工具面板与剪贴板选项' },
  lexicon: { title: '个人词库', detail: '自己添加的词语' },
  learning: { title: '选词记录', detail: '学到的选词习惯' },
  phrases: { title: '输入习惯', detail: '添加的短语和候选里的表情' },
  panel: { title: '面板收藏', detail: '符号、颜文字、表情的收藏和最近使用' },
  stickers: { title: '表情包', detail: '上传的图片和动图' },
  clipboard: { title: '剪贴板记录', detail: '复制过的文字和图片，可能较大' },
  records: { title: '记录', detail: '加密的地址、账号密码等' },
};
const MERGE_HINT: Record<BackupPart, string> = {
  settings: '合并：备份里的选项覆盖这里的同名选项',
  lexicon: '合并：两边的词都保留，同一个词取较高词频',
  learning: '合并：两边的记录都保留，同一个词取较新的',
  phrases: '合并：两边的短语都保留',
  panel: '合并：两边的收藏和最近使用都保留',
  stickers: '合并：添加这里没有的表情包',
  clipboard: '合并：添加这里没有的记录',
  records: '记录是加密的，只能覆盖：设了密码的用原来的密码打开；没设密码的只能在原来那台电脑上打开',
};

const size = (bytes: number) => bytes < 1024 ? `${bytes} B` : bytes < 1024 * 1024 ? `${Math.round(bytes / 1024)} KB` : `${(bytes / 1024 / 1024).toFixed(1)} MB`;
const fileName = () => {
  const now = new Date();
  return `本地拼音备份-${now.getFullYear()}${String(now.getMonth() + 1).padStart(2, '0')}${String(now.getDate()).padStart(2, '0')}.lpbackup`;
};

export function BackupSection({ notify }: { notify: (message: string) => void }) {
  const [choosing, setChoosing] = useState(false);
  const [importing, setImporting] = useState<BackupSummary | null>(null);
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const exportParts = async (parts: BackupPart[]) => {
    setError(null);
    const file = await window.localPinyin.chooseBackupFile('save', fileName());
    if (!file) return false;
    setPending(true);
    try {
      const result = await window.localPinyin.exportBackup(parts, file);
      const total = Object.values(result.parts).reduce((sum, part) => sum + (part?.bytes ?? 0), 0);
      notify(`已导出 ${Object.keys(result.parts).length} 项（${size(total)}）`);
      return true;
    } catch (failure) { setError(asError(failure)); return false; }
    finally { setPending(false); }
  };
  const startImport = async () => {
    setError(null);
    const file = await window.localPinyin.chooseBackupFile('open');
    if (!file) return;
    setPending(true);
    try { setImporting(await window.localPinyin.inspectBackup(file)); }
    catch (failure) { setError(asError(failure)); }
    finally { setPending(false); }
  };

  return <section className="backup" aria-labelledby="backup-title">
    <h2 className="block-title" id="backup-title">导入与导出<span>换电脑或重装系统时，带走你的词库、习惯和收藏</span></h2>
    <div className="backup-actions">
      <div className="backup-card">
        <span className="backup-icon"><Icon name="download" size={18} /></span>
        <div><strong>导出数据</strong><p>保存成一个 .lpbackup 文件。快速导出包含全部内容。</p></div>
        <div className="backup-buttons">
          <button className="button secondary" disabled={pending} onClick={() => setChoosing(true)}>选择内容…</button>
          <button className="button primary" disabled={pending} onClick={() => void exportParts([...backupParts])}>快速导出</button>
        </div>
      </div>
      <div className="backup-card">
        <span className="backup-icon"><Icon name="upload" size={18} /></span>
        <div><strong>导入数据</strong><p>选择备份文件，再决定导入哪些内容、合并还是覆盖。</p></div>
        <div className="backup-buttons">
          <button className="button secondary" disabled={pending} onClick={() => void startImport()}>从备份导入…</button>
        </div>
      </div>
    </div>
    <ErrorNotice message={error} />
    {choosing && <ExportChooser busy={pending} onClose={() => setChoosing(false)}
      onExport={parts => void exportParts(parts).then(done => { if (done) setChoosing(false); })} />}
    {importing && <ImportChooser summary={importing} onClose={() => setImporting(null)}
      onDone={imported => { setImporting(null); notify(`已导入：${imported.map(part => PARTS[part].title).join('、')}`); }} />}
  </section>;
}

function ExportChooser({ busy, onClose, onExport }: { busy: boolean; onClose: () => void; onExport: (parts: BackupPart[]) => void }) {
  const [parts, setParts] = useState<Set<BackupPart>>(() => new Set(backupParts.filter(part => part !== 'clipboard')));
  const toggle = (part: BackupPart) => setParts(current => {
    const next = new Set(current);
    if (next.has(part)) next.delete(part); else next.add(part);
    return next;
  });
  return <Modal title="选择要导出的内容" description="勾选的内容会保存到一个备份文件里。" busy={busy} onClose={onClose}>
    <div className="modal-body backup-parts">{backupParts.map(part =>
      <label key={part} className="backup-part">
        <input type="checkbox" checked={parts.has(part)} onChange={() => toggle(part)} />
        <span><strong>{PARTS[part].title}</strong><small>{PARTS[part].detail}</small></span>
      </label>)}
    </div>
    <footer className="modal-footer">
      <button className="button secondary" disabled={busy} onClick={onClose}>取消</button>
      <button className="button primary" disabled={busy || !parts.size} onClick={() => onExport(backupParts.filter(part => parts.has(part)))}>
        {busy ? '正在导出…' : `导出 ${parts.size} 项`}</button>
    </footer>
  </Modal>;
}

function ImportChooser({ summary, onClose, onDone }: { summary: BackupSummary; onClose: () => void; onDone: (parts: BackupPart[]) => void }) {
  const present = backupParts.filter(part => summary.parts[part]);
  const [choices, setChoices] = useState<Partial<Record<BackupPart, BackupMode | 'skip'>>>(
    () => Object.fromEntries(present.map(part => [part, part === 'records' ? 'skip' : 'merge'])));
  const [pending, setPending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const chosen = present.filter(part => choices[part] && choices[part] !== 'skip');
  const run = async () => {
    setPending(true); setError(null);
    try {
      const modes = Object.fromEntries(chosen.map(part => [part, choices[part]])) as Partial<Record<BackupPart, BackupMode>>;
      onDone((await window.localPinyin.importBackup(summary.file, modes)).imported);
    } catch (failure) { setError(asError(failure)); setPending(false); }
  };
  const created = new Date(summary.created);
  return <Modal wide title="导入哪些内容？" busy={pending} onClose={onClose}
    description={`备份来自 ${Number.isNaN(created.getTime()) ? '未知时间' : created.toLocaleString('zh-CN')}${summary.version ? `（版本 ${summary.version}）` : ''}。`}>
    <div className="modal-body">
      {!present.length && <p className="confirmation-copy">这个备份里没有可以导入的内容。</p>}
      <ul className="import-parts">{present.map(part => {
        const info = summary.parts[part]!;
        const mode = choices[part] ?? 'skip';
        return <li key={part} className={mode === 'skip' ? 'skipped' : undefined}>
          <div className="import-part-title"><strong>{PARTS[part].title}</strong>
            <small>{part === 'settings' || part === 'panel' ? size(info.bytes) : `${formatNumber(info.items)} 条 · ${size(info.bytes)}`}</small>
            <small className="import-hint">{part === 'records' && mode !== 'skip' ? MERGE_HINT.records
              : mode === 'overwrite' ? '覆盖：用备份替换这里现有的内容' : mode === 'merge' ? MERGE_HINT[part] : '不导入'}</small></div>
          <div className="segmented" role="radiogroup" aria-label={`${PARTS[part].title}的导入方式`}>
            {(part === 'records' ? ['overwrite', 'skip'] as const : ['merge', 'overwrite', 'skip'] as const).map(value =>
              <label key={value} className={mode === value ? 'active' : undefined}>
                <input type="radio" name={`import-${part}`} checked={mode === value} disabled={pending}
                  onChange={() => setChoices(current => ({ ...current, [part]: value }))} />
                {value === 'merge' ? '合并' : value === 'overwrite' ? '覆盖' : '跳过'}
              </label>)}
          </div>
        </li>;
      })}</ul>
      <ErrorNotice message={error} />
    </div>
    <footer className="modal-footer">
      <button className="button secondary" disabled={pending} onClick={onClose}>取消</button>
      <button className="button primary" disabled={pending || !chosen.length} onClick={() => void run()}>
        {pending ? '正在导入…' : `导入 ${chosen.length} 项`}</button>
    </footer>
  </Modal>;
}
