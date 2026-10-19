// 剪贴板: what was copied lately, newest first. Clicking a copy pastes it
// where the panel was opened; a picture opens larger first. The search box
// finds words in texts and in pictures (read when they were copied), and the
// date list narrows to one day.
import { useCallback, useEffect, useState } from 'react';
import { Icon } from '../icons';
import type { ClipItem, ClipList } from './types';

const PAGE = 120;

function when(time: number) {
  const date = new Date(time), now = new Date();
  const clock = `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}`;
  const days = Math.round((new Date(now.toDateString()).getTime() - new Date(date.toDateString()).getTime()) / 86_400_000);
  if (days === 0) return clock;
  if (days === 1) return `昨天 ${clock}`;
  if (date.getFullYear() === now.getFullYear()) return `${date.getMonth() + 1}月${date.getDate()}日 ${clock}`;
  return `${date.getFullYear()}年${date.getMonth() + 1}月${date.getDate()}日`;
}

function dayLabel(day: string) {
  const [year, month, date] = day.split('-').map(Number);
  const value = new Date(year, month - 1, date), today = new Date(new Date().toDateString());
  const days = Math.round((today.getTime() - value.getTime()) / 86_400_000);
  if (days === 0) return '今天';
  if (days === 1) return '昨天';
  return year === today.getFullYear() ? `${month}月${date}日` : `${year}年${month}月${date}日`;
}

function retention(days: number) {
  return days === 0 ? '永久保存' : days === 365 ? '保存 1 年' : `保存 ${days} 天`;
}

// The part of `text` around the first match, with the match marked.
function Snippet({ text, query }: { text: string; query: string }) {
  const at = query ? text.toLowerCase().indexOf(query.toLowerCase()) : -1;
  if (at < 0) return <>{text}</>;
  const start = Math.max(0, at - 24);
  return <>{start > 0 ? '…' : ''}{text.slice(start, at)}<mark>{text.slice(at, at + query.length)}</mark>{text.slice(at + query.length)}</>;
}

export function Clipboard({ search, refreshKey, onError, onNotice }: {
  search: string; refreshKey: number; onError: (message: string) => void; onNotice: (message: string) => void;
}) {
  const [list, setList] = useState<ClipList | null>(null);
  const [day, setDay] = useState('');
  const [limit, setLimit] = useState(PAGE);
  const [viewing, setViewing] = useState<ClipItem | null>(null);
  const query = search.trim();

  const load = useCallback(() => {
    window.localPinyinPanel.clipboard({ search: query, day, limit }).then(setList)
      .catch(error => onError(error instanceof Error ? error.message : String(error)));
  }, [query, day, limit, onError]);
  useEffect(() => { load(); }, [load, refreshKey]);
  useEffect(() => { setLimit(PAGE); }, [query, day]);
  // New copies show up while the panel is open.
  useEffect(() => {
    if (list?.status !== 'on') return;
    const timer = window.setInterval(load, 2000);
    return () => window.clearInterval(timer);
  }, [list?.status, load]);
  useEffect(() => {
    if (!viewing) return;
    const close = (event: KeyboardEvent) => { if (event.key === 'Escape') { event.stopPropagation(); setViewing(null); } };
    window.addEventListener('keydown', close, true);
    return () => window.removeEventListener('keydown', close, true);
  }, [viewing]);

  const consent = (on: boolean) => {
    window.localPinyinPanel.clipboardConsent(on).then(() => { onNotice(on ? '已开启剪贴板记录' : '不记录剪贴板'); load(); })
      .catch(error => onError(error instanceof Error ? error.message : String(error)));
  };
  const paste = (item: ClipItem) => {
    window.localPinyinPanel.clipboardPaste(item.id).catch(error => onError(error instanceof Error ? error.message : String(error)));
  };
  const remove = (item: ClipItem) => {
    window.localPinyinPanel.clipboardDelete(item.id).then(() => { if (viewing?.id === item.id) setViewing(null); load(); })
      .catch(error => onError(error instanceof Error ? error.message : String(error)));
  };

  if (!list) return <div className="clipboard loading" aria-busy="true"><div className="spinner" /></div>;
  if (list.status !== 'on') {
    return (
      <div className="clipboard consent">
        <div className="consent-card">
          <span className="consent-icon"><Icon name="clipboard" size={26} /></span>
          <h2>{list.status === 'ask' ? '要记住复制过的内容吗？' : '剪贴板记录已关闭'}</h2>
          <p>开启后，本地拼音会记下你复制的文字和图片，在这里随时找回、再次粘贴；图片里的文字也能搜到。
            内容只保存在这台电脑上，不会上传，密码管理器等标记为不记录的内容会跳过。保存时间可以在设置里调整，最长永久保存。</p>
          <div className="consent-actions">
            <button type="button" className="button primary" onClick={() => consent(true)}>开启剪贴板记录</button>
            {list.status === 'ask' && <button type="button" className="button secondary" onClick={() => consent(false)}>不用了</button>}
          </div>
        </div>
      </div>
    );
  }
  return (
    <div className="clipboard">
      <div className="category-bar clip-bar">
        <label className="day-select">
          <Icon name="calendar" size={15} />
          <select value={day} aria-label="按日期查看" onChange={event => setDay(event.target.value)}>
            <option value="">全部日期</option>
            {list.days.map(entry => <option key={entry.day} value={entry.day}>{dayLabel(entry.day)} · {entry.count} 条</option>)}
          </select>
        </label>
        <span className="clip-count">{query || day ? `找到 ${list.total} 条` : `共 ${list.total} 条`} · {retention(list.retentionDays)}</span>
      </div>
      <div className="grid-scroller">
        {list.items.length ? (
          <ul className="clip-list">
            {list.items.map(item => (
              <li key={item.id} className={`clip ${item.kind}`}>
                <button type="button" className="clip-main" title={item.kind === 'image' ? '点击查看大图' : '点击粘贴'}
                        onClick={() => item.kind === 'image' ? setViewing(item) : paste(item)}>
                  {item.kind === 'image' ? (
                    <>
                      <img src={`app://local/media/clipboard/${item.id}.png`} alt="" loading="lazy" draggable={false} />
                      <span className="clip-image-meta">
                        <span>图片 {item.w && item.h ? `${item.w}×${item.h}` : ''}</span>
                        {item.ocr && query && item.ocr.toLowerCase().includes(query.toLowerCase()) &&
                          <span className="clip-ocr">图中文字：<Snippet text={item.ocr} query={query} /></span>}
                      </span>
                    </>
                  ) : <span className="clip-text"><Snippet text={item.text ?? ''} query={query} />{item.long ? '…' : ''}</span>}
                </button>
                <span className="clip-side">
                  <time>{when(item.time)}</time>
                  <button type="button" className="icon-button" aria-label="删除这条" title="删除" onClick={() => remove(item)}>
                    <Icon name="trash" size={14} /></button>
                </span>
              </li>
            ))}
            {list.total > list.items.length &&
              <li className="clip-more"><button type="button" className="text-button" onClick={() => setLimit(value => value + PAGE)}>
                再显示 {Math.min(PAGE, list.total - list.items.length)} 条</button></li>}
          </ul>
        ) : (
          <p className="empty-note">{query || day ? '没有找到符合的内容。' : '还没有记录。复制一段文字或一张图片，它就会出现在这里。'}</p>
        )}
      </div>
      {viewing && (
        <div className="viewer" role="dialog" aria-modal="true" aria-label="查看图片" onClick={() => setViewing(null)}>
          <img src={`app://local/media/clipboard/${viewing.id}.png`} alt="复制的图片" onClick={event => event.stopPropagation()} draggable={false} />
          <div className="viewer-bar" onClick={event => event.stopPropagation()}>
            <span>{when(viewing.time)}{viewing.w && viewing.h ? ` · ${viewing.w}×${viewing.h}` : ''}</span>
            {viewing.ocr && <span className="viewer-ocr" title={viewing.ocr}>图中文字：{viewing.ocr}</span>}
            <button type="button" className="button secondary compact" onClick={() => remove(viewing)}><Icon name="trash" size={14} />删除</button>
            <button type="button" className="button primary compact" onClick={() => paste(viewing)}>粘贴</button>
            <button type="button" className="icon-button" aria-label="关闭" onClick={() => setViewing(null)}><Icon name="close" size={16} /></button>
          </div>
        </div>
      )}
    </div>
  );
}
