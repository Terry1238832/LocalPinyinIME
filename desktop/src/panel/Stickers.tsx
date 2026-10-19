// 表情包: the user's own pictures, still or animated. Drop pictures on the
// box (or click it to choose from a folder); clicking one pastes it where
// the panel was opened. The panel stays open on this page, since dragging
// from a folder means clicking outside it.
import { useEffect, useRef, useState, type DragEvent } from 'react';
import { Icon } from '../icons';
import type { Sticker } from './types';

const ACCEPT = ['image/png', 'image/jpeg', 'image/gif', 'image/webp', 'image/bmp'];

export function Stickers({ stickers, search, onChange, onError, onNotice }: {
  stickers: Sticker[]; search: string; onChange: (list: Sticker[]) => void;
  onError: (message: string) => void; onNotice: (message: string) => void;
}) {
  const [dragging, setDragging] = useState(false);
  const [busy, setBusy] = useState(false);
  const [confirming, setConfirming] = useState<string | null>(null);
  const picker = useRef<HTMLInputElement>(null);
  const query = search.trim().toLowerCase();
  const shown = query ? stickers.filter(sticker => sticker.name.toLowerCase().includes(query)) : stickers;

  // Closing the file dialog without choosing lets clicks outside put the panel away again.
  useEffect(() => {
    const input = picker.current;
    const cancel = () => { void window.localPinyinPanel.choosingFile(false); };
    input?.addEventListener('cancel', cancel);
    return () => input?.removeEventListener('cancel', cancel);
  }, []);

  const add = async (files: File[]) => {
    const pictures = files.filter(file => ACCEPT.includes(file.type) || /\.(png|jpe?g|gif|webp|bmp)$/i.test(file.name));
    if (!pictures.length) { onError('只能添加 PNG、JPG、GIF、WebP 或 BMP 图片。'); return; }
    setBusy(true);
    try {
      const payload = await Promise.all(pictures.slice(0, 50).map(async file => ({ name: file.name, data: new Uint8Array(await file.arrayBuffer()) })));
      onChange(await window.localPinyinPanel.addStickers(payload));
      onNotice(`已添加 ${payload.length} 张表情包`);
    } catch (error) {
      onError(error instanceof Error ? error.message : String(error));
    } finally { setBusy(false); }
  };
  const choose = () => {
    void window.localPinyinPanel.choosingFile(true);
    picker.current?.click();
  };
  const drop = (event: DragEvent) => {
    event.preventDefault();
    setDragging(false);
    void add([...event.dataTransfer.files]);
  };
  const remove = (id: string) => {
    window.localPinyinPanel.deleteSticker(id).then(list => { onChange(list); setConfirming(null); })
      .catch(error => onError(error instanceof Error ? error.message : String(error)));
  };

  return (
    <div className={dragging ? 'stickers dragging' : 'stickers'}
         onDragOver={event => { if ([...event.dataTransfer.types].includes('Files')) { event.preventDefault(); setDragging(true); } }}
         onDragLeave={event => { if (event.currentTarget === event.target) setDragging(false); }}
         onDrop={drop}>
      <input ref={picker} type="file" accept={ACCEPT.join(',')} multiple hidden
             onChange={event => { void window.localPinyinPanel.choosingFile(false); void add([...(event.target.files ?? [])]); event.target.value = ''; }} />
      <div className="grid-scroller">
        <div className="sticker-grid">
          <button type="button" className="sticker-drop" onClick={choose} disabled={busy} aria-label="添加表情包：点击选择图片，或把图片拖到这里">
            <Icon name="upload" size={22} />
            <span>{busy ? '正在添加…' : dragging ? '松开即可添加' : '拖入图片'}</span>
            <small>或点击选择</small>
          </button>
          {shown.map(sticker => (
            <div key={sticker.id} className={confirming === sticker.id ? 'sticker confirming' : 'sticker'}>
              <button type="button" className="sticker-main" title={`${sticker.name} · 点击发送`}
                      onClick={() => window.localPinyinPanel.paste(sticker.id).catch(error => onError(error instanceof Error ? error.message : String(error)))}
                      onContextMenu={event => { event.preventDefault(); setConfirming(sticker.id); }}>
                <img src={`app://local/media/stickers/${sticker.id}${sticker.ext}`} alt={sticker.name} loading="lazy" draggable={false} />
              </button>
              {confirming === sticker.id ? (
                <div className="sticker-confirm">
                  <button type="button" className="button danger compact" onClick={() => remove(sticker.id)}>删除</button>
                  <button type="button" className="button secondary compact" onClick={() => setConfirming(null)}>取消</button>
                </div>
              ) : (
                <button type="button" className="sticker-delete" aria-label={`删除 ${sticker.name}`} title="删除"
                        onClick={() => setConfirming(sticker.id)}><Icon name="close" size={12} /></button>
              )}
            </div>
          ))}
        </div>
        {!stickers.length && <p className="empty-note">还没有表情包。把喜欢的图片或动图拖到上面的框里，点一下就能发送。</p>}
        {!!stickers.length && query && !shown.length && <p className="empty-note">没有名字里带“{search.trim()}”的表情包。</p>}
      </div>
    </div>
  );
}
