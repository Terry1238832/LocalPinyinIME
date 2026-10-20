// 表情包: the user's own pictures, still or animated. Drop pictures on the
// page from a folder, a chat (WeChat and the like) or a web page, paste a
// copied one with Ctrl+V, or click the box to choose; clicking one pastes it
// where the panel was opened. A picture from a web page is fetched again by
// its address, since what is copied is only its first frame.
import { useEffect, useRef, useState, type DragEvent } from 'react';
import { Icon } from '../icons';
import type { Sticker } from './types';

const ACCEPT = ['image/png', 'image/jpeg', 'image/gif', 'image/webp', 'image/bmp'];
// What a drag from another program may carry a picture as.
const PICTURE_DRAG_TYPES =['Files', 'text/uri-list', 'text/html'];

// The pictures an HTML copy or drag shows.
function htmlPictures(html: string) {
  return html ? [...new DOMParser().parseFromString(html, 'text/html').querySelectorAll('img[src]')]
    .map(image => image.getAttribute('src') ?? '').filter(Boolean) : [];
}

// Links in a drag's link list and HTML: file://, http(s) and data: pictures.
function droppedLinks(data: DataTransfer) {
  const links = data.getData('text/uri-list').split(/\r?\n/).map(line => line.trim()).filter(line => line && !line.startsWith('#'));
  return [...new Set([...links, ...htmlPictures(data.getData('text/html'))])];
}

const isWeb = (link: string) => /^https?:\/\//i.test(link);

function dataPicture(link: string, index: number) {
  const comma = link.indexOf(',');
  const body = link.slice(comma + 1);
  const data = /;base64$/i.test(link.slice(0, comma)) ? Uint8Array.from(atob(body), ch => ch.charCodeAt(0))
    : new TextEncoder().encode(decodeURIComponent(body));
  return { name: `拖入的图片 ${index + 1}`, data };
}

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

  const fail = (error: unknown) => onError(error instanceof Error ? error.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, '') : String(error));
  // `note` replaces the usual notice (e.g. only the still frame was added).
  const adding = async (work: (note: (text: string) => void) => Promise<Sticker[]>) => {
    setBusy(true);
    let notice = '';
    try {
      const before = stickers.length;
      const list = await work(text => { notice = text; });
      onChange(list);
      onNotice(notice || `已添加 ${Math.max(1, list.length - before)} 张表情包`);
    } catch (error) { fail(error); } finally { setBusy(false); }
  };
  const payload = (files: File[]) => Promise.all(files.slice(0, 50).map(async file =>
    ({ name: file.name, data: new Uint8Array(await file.arrayBuffer()) })));
  // A web picture's original by its address; failing that, the copied frame.
  const fromWeb = (link: string, frames: File[]) => adding(async note => {
    try { return await window.localPinyinPanel.addStickerFromUrl(link); } catch (error) {
      if (!frames.length) throw error;
      note('没能取到原图，已添加静态的一帧');
      return window.localPinyinPanel.addStickers(await payload(frames));
    }
  });
  // Files are checked by their content where they are saved, so a picture
  // from a chat program's cache counts even without a picture's name.
  const add = (files: File[]) => {
    const pictures = files.filter(file => !file.type || ACCEPT.includes(file.type) || /\.(png|jpe?g|gif|webp|bmp|dat)$/i.test(file.name));
    if (!pictures.length) { onError('只能添加 PNG、JPG、GIF、WebP 或 BMP 图片。'); return; }
    void adding(async () => window.localPinyinPanel.addStickers(await payload(pictures)));
  };
  const choose = () => picker.current?.click();
  const drop = (event: DragEvent) => {
    event.preventDefault();
    setDragging(false);
    const files = [...event.dataTransfer.files];
    if (files.length) { add(files); return; }
    const links = droppedLinks(event.dataTransfer);
    const local = links.filter(link => link.startsWith('file:'));
    const inline = links.filter(link => /^data:image\//.test(link));
    const web = links.filter(isWeb);
    if (local.length) void adding(() => window.localPinyinPanel.addStickerLinks(local.slice(0, 50)));
    else if (inline.length) void adding(() => window.localPinyinPanel.addStickers(inline.slice(0, 50).map(dataPicture)));
    else if (web.length) void fromWeb(web[0], []);
    else onError('没有拿到这张图片。可以在聊天里右键图片选“复制”，再回到这里按 Ctrl+V。');
  };
  // Ctrl+V: a copied picture or picture file; copied text still goes to the search box.
  useEffect(() => {
    const paste = (event: ClipboardEvent) => {
      const data = event.clipboardData;
      if (!data) return;
      const files = [...data.files];
      if (!files.length && data.types.includes('text/plain')) return;
      event.preventDefault();
      // "复制图片" in a browser: the first frame, and the page's HTML for it.
      const [source] = htmlPictures(data.getData('text/html'));
      if (source && isWeb(source)) { void fromWeb(source, files); return; }
      if (source && /^data:image\//.test(source)) { void adding(() => window.localPinyinPanel.addStickers([dataPicture(source, 0)])); return; }
      if (files.length) { add(files); return; }
      void adding(() => window.localPinyinPanel.addStickerFromClipboard());
    };
    window.addEventListener('paste', paste);
    return () => window.removeEventListener('paste', paste);
  });
  const remove = (id: string) => {
    window.localPinyinPanel.deleteSticker(id).then(list => { onChange(list); setConfirming(null); })
      .catch(error => onError(error instanceof Error ? error.message : String(error)));
  };

  return (
    <div className={dragging ? 'stickers dragging' : 'stickers'}
         onDragOver={event => { if ([...event.dataTransfer.types].some(type => PICTURE_DRAG_TYPES.includes(type))) { event.preventDefault(); setDragging(true); } }}
         onDragLeave={event => { if (event.currentTarget === event.target) setDragging(false); }}
         onDrop={drop}>
      <input ref={picker} type="file" accept={ACCEPT.join(',')} multiple hidden
             onChange={event => { add([...(event.target.files ?? [])]); event.target.value = ''; }} />
      <div className="grid-scroller">
        <div className="sticker-grid">
          <button type="button" className="sticker-drop" onClick={choose} disabled={busy}
                  aria-label="添加表情包：把图片拖到这里（也可以从微信聊天里拖），按 Ctrl+V 粘贴，或点击选择">
            <Icon name="upload" size={22} />
            <span>{busy ? '正在添加…' : dragging ? '松开即可添加' : '拖入图片'}</span>
            <small>Ctrl+V 粘贴</small>
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
        {!stickers.length && <p className="empty-note">还没有表情包。把喜欢的图片或动图拖到这里（文件夹、微信聊天里的图片都可以），
          或者复制后按 Ctrl+V；之后点一下就能发送。</p>}
        {!!stickers.length && query && !shown.length && <p className="empty-note">没有名字里带“{search.trim()}”的表情包。</p>}
      </div>
    </div>
  );
}
