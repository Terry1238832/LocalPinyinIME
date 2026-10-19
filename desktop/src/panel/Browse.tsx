// 符号, 颜文字 and 表情: a category bar over one long grid. The bar follows the
// scrolling and jumps to a category when clicked, so every category is a
// scroll or a click away. Clicking an item adds it to the tray; right-click
// adds it to 收藏 or takes it out.
import { useEffect, useLayoutEffect, useMemo, useRef, useState, type CSSProperties } from 'react';
import { Icon } from '../icons';
import { codePoints, loadCatalog, matches, type Catalog, type Group, type Item } from './catalog';
import type { PanelKind, PanelState } from './types';

const LABELS: Record<PanelKind, string> = { symbol: '符号', kaomoji: '颜文字', emoji: '表情' };

export function Browse({ kind, state, search, onPick, onFavorite }: {
  kind: PanelKind; state: PanelState; search: string;
  onPick: (item: Item) => void; onFavorite: (item: Item, on: boolean) => void;
}) {
  const [catalog, setCatalog] = useState<Catalog | null>(null);
  const [active, setActive] = useState('');
  const scroller = useRef<HTMLDivElement>(null);
  const bar = useRef<HTMLDivElement>(null);
  const jumping = useRef(false);

  useEffect(() => {
    let live = true;
    setCatalog(null);
    void loadCatalog(kind).then(loaded => { if (live) setCatalog(loaded); });
    return () => { live = false; };
  }, [kind]);

  const favorites = useMemo(() => new Set(state.favorites[kind]), [state, kind]);
  const groups = useMemo<Group[]>(() => {
    if (!catalog) return [];
    const known = (value: string): Item => catalog.byValue.get(value) ?? { c: value };
    const extra: Group[] = [];
    // Emoji open on what was used last; symbols and kaomoji keep it one click away.
    if (state.recents[kind].length) extra.push({ id: 'recent', name: '最近使用', sections: [{ items: state.recents[kind].map(known) }] });
    if (state.favorites[kind].length) extra.push({ id: 'favorite', name: '收藏', sections: [{ items: state.favorites[kind].map(known) }] });
    return [...extra, ...catalog.groups];
  }, [catalog, state, kind]);

  const query = search.trim().toLowerCase();
  const results = useMemo(() => {
    if (!catalog || !query) return null;
    const found: Item[] = [];
    for (const item of catalog.byValue.values()) if (matches(item, query)) { found.push(item); if (found.length >= 600) break; }
    return found;
  }, [catalog, query]);

  // The category whose section is at the top of the grid.
  const follow = () => {
    const box = scroller.current;
    if (!box || jumping.current) return;
    let current = groups[0]?.id ?? '';
    for (const element of box.querySelectorAll<HTMLElement>('[data-group]')) {
      if (element.offsetTop - box.offsetTop <= box.scrollTop + 12) current = element.dataset.group!;
      else break;
    }
    setActive(current);
  };
  useLayoutEffect(() => {
    if (!scroller.current) return;
    scroller.current.scrollTop = 0;
    setActive(groups[0]?.id ?? '');
  }, [kind, catalog, query]);
  useEffect(() => {
    bar.current?.querySelector<HTMLElement>(`[data-tab="${active}"]`)?.scrollIntoView({ block: 'nearest', inline: 'nearest' });
  }, [active]);

  const jump = (id: string) => {
    const box = scroller.current;
    const target = box?.querySelector<HTMLElement>(`[data-group="${id}"]`);
    if (!box || !target) return;
    setActive(id);
    jumping.current = true;
    box.scrollTo({ top: target.offsetTop - box.offsetTop, behavior: 'smooth' });
    window.setTimeout(() => { jumping.current = false; }, 450);
  };
  const scrollBar = (direction: number) => bar.current?.scrollBy({ left: direction * 240, behavior: 'smooth' });

  const cell = (item: Item, index: number) => {
    const favorite = favorites.has(item.c);
    const title = kind === 'kaomoji' ? item.c : `${item.c}  ${item.n ?? ''}${kind === 'symbol' ? '  ' + codePoints(item.c) : ''}`;
    return (
      <button key={item.c + index} type="button" className={`cell ${kind}${favorite ? ' favorite' : ''}`} title={title}
              onClick={() => onPick(item)}
              onContextMenu={event => { event.preventDefault(); onFavorite(item, !favorite); }}>
        <span className="glyph">{item.c}</span>
      </button>
    );
  };

  if (!catalog) return <div className="browse loading" aria-busy="true"><div className="spinner" /></div>;
  return (
    <div className="browse">
      {results ? (
        <div className="category-bar"><span className="category-result">搜索“{search.trim()}” · {results.length} 个{LABELS[kind]}</span></div>
      ) : (
        <div className="category-bar">
          <button type="button" className="bar-arrow" aria-label="向左看分类" onClick={() => scrollBar(-1)}>
            <Icon name="chevron" size={16} style={{ transform: 'rotate(180deg)' }} /></button>
          <div className="category-tabs" ref={bar} role="tablist" aria-label={`${LABELS[kind]}分类`}
               onWheel={event => { if (bar.current && Math.abs(event.deltaY) > Math.abs(event.deltaX)) bar.current.scrollLeft += event.deltaY; }}>
            {groups.map(group => (
              <button key={group.id} type="button" role="tab" data-tab={group.id} aria-selected={active === group.id}
                      className={active === group.id ? 'category active' : 'category'} onClick={() => jump(group.id)}>
                {group.id === 'favorite' && <Icon name="star" size={13} />}
                {group.id === 'recent' && <Icon name="history" size={13} />}
                {group.name}
              </button>
            ))}
          </div>
          <button type="button" className="bar-arrow" aria-label="向右看分类" onClick={() => scrollBar(1)}>
            <Icon name="chevron" size={16} /></button>
        </div>
      )}
      <div className="grid-scroller" ref={scroller} onScroll={follow}>
        {results ? (
          results.length ? <div className={`grid ${kind}`}>{results.map(cell)}</div>
            : <p className="empty-note">没有找到“{search.trim()}”。{kind === 'kaomoji' ? '可以搜开心、难过、动物这样的心情。' : '换个说法试试，比如“箭头”“对勾”“爱心”。'}</p>
        ) : groups.map(group => (
          <section key={group.id} data-group={group.id} className="group"
                   style={{ containIntrinsicBlockSize: `auto ${Math.ceil(group.sections.reduce((n, s) => n + s.items.length, 0) / (kind === 'kaomoji' ? 4 : 13)) * 46 + 40}px` } as CSSProperties}>
            <h2 className="group-title">{group.name}
              {group.id === 'favorite' && <small>右键可取消收藏</small>}
              {group.id === 'recent' && <small>最近确定输入的在最前</small>}
            </h2>
            {group.sections.map((section, index) => (
              <div key={index} className="section">
                {section.name && group.sections.length > 1 && <h3 className="section-title">{section.name}</h3>}
                <div className={`grid ${kind}`}>{section.items.map(cell)}</div>
              </div>
            ))}
          </section>
        ))}
      </div>
    </div>
  );
}
