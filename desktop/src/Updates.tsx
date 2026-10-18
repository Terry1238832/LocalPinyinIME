import { useState } from 'react';
import type { ReactNode } from 'react';
import { asError, ErrorNotice, Meter, Switch } from './components';
import { Icon } from './icons';
import type { UpdateStatus } from './types';

// 软件更新 on the About page: the current version, what the last check found,
// downloading and starting a newer installer, and the automatic check switch.

function megabytes(bytes: number) {
  return `${(bytes / 1048576).toFixed(1)} MB`;
}

function day(iso?: string) {
  if (!iso) return '';
  const date = new Date(iso);
  return Number.isNaN(date.getTime()) ? '' : `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`;
}

// The release notes as a few plain lines: no headings, bold or code marks.
function summary(notes: string) {
  return notes.split('\n').map(line => line.replace(/^#+\s*/, '').replace(/\*\*|`/g, '').trim())
    .filter(line => line && !/^[-|]{3,}/.test(line)).slice(0, 6);
}

export function UpdatePanel({ status }: { status: UpdateStatus | null }) {
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [started, setStarted] = useState(false);
  if (!status) return null;
  const run = async (action: () => Promise<unknown>) => {
    setBusy(true); setError(null);
    try { await action(); } catch (failure) { setError(asError(failure)); } finally { setBusy(false); }
  };
  const api = window.localPinyin;
  const latest = status.latest;
  const check = <button className="button secondary" disabled={busy} onClick={() => void run(() => api.checkForUpdate())}>
    <Icon name="history" size={15} />检查更新</button>;
  let headline: ReactNode;
  let detail: ReactNode = null;
  let actions: ReactNode = check;
  switch (status.state) {
    case 'checking':
      headline = '正在检查更新…';
      actions = <button className="button secondary" disabled>正在检查…</button>;
      break;
    case 'current':
      headline = <><span className="update-dot current" aria-hidden="true" />已是最新版本</>;
      break;
    case 'available':
    case 'downloading':
    case 'ready':
      headline = <><span className="update-dot available" aria-hidden="true" />发现新版本 {latest?.version}</>;
      detail = latest && <div className="update-notes">
        <p className="update-meta">{[day(latest.published) && `${day(latest.published)} 发布`, latest.size ? megabytes(latest.size) : ''].filter(Boolean).join(' · ')}</p>
        <ul>{summary(latest.notes).map((line, index) => <li key={index}>{line.replace(/^[-*]\s*/, '')}</li>)}</ul>
        <button className="text-button" onClick={() => void run(() => api.openReleasePage())}>查看完整更新说明</button>
      </div>;
      if (status.state === 'available') {
        actions = latest?.downloadable
          ? <button className="button primary" disabled={busy} onClick={() => void run(() => api.downloadUpdate())}>
              <Icon name="folder" size={15} />下载新版本</button>
          : <button className="button secondary" onClick={() => void run(() => api.openReleasePage())}>到 GitHub 下载</button>;
      } else if (status.state === 'downloading') {
        actions = <div className="update-progress"><Meter value={Math.round((status.progress ?? 0) * 100)} max={100} label="下载进度" />
          <span>正在下载 {Math.round((status.progress ?? 0) * 100)}%</span></div>;
      } else {
        actions = started
          ? <span className="update-started">安装程序已启动，按提示完成安装</span>
          : <button className="button primary" disabled={busy} onClick={() => void run(async () => { await api.installUpdate(); setStarted(true); })}>
              <Icon name="check" size={15} />立即安装</button>;
        detail = <>{detail}<p className="update-ready">已下载并通过 SHA-256 校验。安装时设置程序会关闭，正在输入的应用需要重新打开。</p></>;
      }
      break;
    case 'error':
      headline = <><span className="update-dot error" aria-hidden="true" />检查或下载没有完成</>;
      detail = <ErrorNotice message={status.error ?? null} />;
      actions = <button className="button secondary" disabled={busy} onClick={() => void run(() => api.checkForUpdate())}>重试</button>;
      break;
    default:
      headline = '还没有检查过更新';
  }
  return <section className="update-panel" aria-labelledby="update-title">
    <div className="update-top">
      <div className="update-copy">
        <h2 id="update-title" className="block-title">软件更新<span>当前版本 {status.current}{status.checkedAt ? ` · 上次检查 ${day(status.checkedAt)}` : ''}</span></h2>
        <p className="update-headline" role="status" aria-live="polite">{headline}</p>
      </div>
      <div className="update-actions">{actions}</div>
    </div>
    {detail}
    <ErrorNotice message={error} />
    <div className="update-auto">
      <div><strong>自动检查更新</strong>
        <p>打开设置时检查 GitHub 上有没有新版本，最多每 12 小时一次。只读取版本信息，不上传任何内容；输入法本身始终不联网。</p></div>
      <Switch checked={status.autoCheck} disabled={busy} label="自动检查更新"
        onChange={enabled => void run(() => api.setAutoUpdateCheck(enabled))} />
    </div>
  </section>;
}
