// 工具: what is typed here turns into results at once — a sum, a number in
// Chinese words or capitals, a date, the current time. The same tools work
// in any input box after a v (v55+15).
import { useEffect, useRef, useState } from 'react';
import { Icon } from '../icons';

const EXAMPLES = [
  { title: '数字计算', input: '55+15', hint: '加减乘除、括号、百分号' },
  { title: '大小写转换', input: '123', hint: '中文数字、大写金额' },
  { title: '日期格式', input: '12.19', hint: '月.日 或 年.月.日' },
  { title: '当前时间', input: 'sj', hint: 'sj 时间 · rq 日期 · xq 星期' },
];

export function Tools({ onInsert, focusKey }: { onInsert: (text: string) => void; focusKey: number }) {
  const [input, setInput] = useState('');
  const [results, setResults] = useState<string[]>([]);
  const [selected, setSelected] = useState(0);
  const [previews, setPreviews] = useState<Record<string, string>>({});
  const field = useRef<HTMLInputElement>(null);

  useEffect(() => { field.current?.focus(); }, [focusKey]);
  // What each example turns into; the clock ticks here rather than asking
  // the input method every second.
  useEffect(() => {
    let live = true;
    void Promise.all(EXAMPLES.filter(example => example.input !== 'sj').map(example => window.localPinyinPanel.tools(example.input)
      .then(result => [example.input, result.results[0] ?? ''] as const).catch(() => [example.input, ''] as const)))
      .then(entries => { if (live) setPreviews(previous => ({ ...previous, ...Object.fromEntries(entries) })); });
    const two = (value: number) => String(value).padStart(2, '0');
    const tick = () => {
      const now = new Date();
      setPreviews(previous => ({ ...previous, sj: `${now.getFullYear()}-${two(now.getMonth() + 1)}-${two(now.getDate())} ` +
        `${two(now.getHours())}:${two(now.getMinutes())}:${two(now.getSeconds())}` }));
    };
    tick();
    const timer = window.setInterval(tick, 1000);
    return () => { live = false; window.clearInterval(timer); };
  }, []);
  useEffect(() => {
    const value = input.trim();
    if (!value) { setResults([]); return; }
    let live = true;
    const timer = window.setTimeout(() => {
      window.localPinyinPanel.tools(value).then(result => { if (live) { setResults(result.results); setSelected(0); } })
        .catch(() => { if (live) setResults([]); });
    }, 60);
    return () => { live = false; window.clearTimeout(timer); };
  }, [input]);

  return (
    <div className="tools">
      <label className="tool-field">
        <Icon name="tools" size={18} />
        <input ref={field} value={input} spellCheck={false} placeholder="输入 55+15、123、12.19 或 sj，自动转换"
               aria-label="要转换的内容" maxLength={120}
               onChange={event => setInput(event.target.value)}
               onKeyDown={event => {
                 if (event.key === 'ArrowDown') { event.preventDefault(); setSelected(index => Math.min(index + 1, results.length - 1)); }
                 else if (event.key === 'ArrowUp') { event.preventDefault(); setSelected(index => Math.max(index - 1, 0)); }
                 else if (event.key === 'Enter' && results[selected]) { event.preventDefault(); onInsert(results[selected]); }
               }} />
        {input && <button type="button" className="icon-button" aria-label="清空" onClick={() => { setInput(''); field.current?.focus(); }}>
          <Icon name="close" size={14} /></button>}
      </label>
      {input.trim() ? (
        results.length ? (
          <ol className="tool-results" aria-label="转换结果">
            {results.map((result, index) => (
              <li key={result + index}>
                <button type="button" className={index === selected ? 'tool-result selected' : 'tool-result'}
                        onMouseEnter={() => setSelected(index)} onClick={() => onInsert(result)}>
                  <span className="tool-index">{index + 1}</span><span className="tool-value">{result}</span>
                  {index === selected && <span className="tool-enter"><Icon name="enter" size={14} />输入</span>}
                </button>
              </li>
            ))}
          </ol>
        ) : <p className="empty-note">这个还不会转换。可以试试算式（3*(4+5)）、数字（2024）、日期（2024.12.19）或 sj。</p>
      ) : (
        <div className="tool-cards">
          {EXAMPLES.map(example => (
            <button key={example.title} type="button" className="tool-card"
                    onClick={() => { setInput(example.input); field.current?.focus(); }}>
              <span className="tool-card-title">{example.title}</span>
              <span className="tool-card-line"><kbd>{example.input}</kbd><Icon name="arrow" size={14} />
                <span className="tool-card-result">{previews[example.input] || '…'}</span></span>
              <span className="tool-card-hint">{example.hint}</span>
            </button>
          ))}
          <p className="tool-tip">在任何输入框里先打 <kbd>v</kbd> 再输入这些，候选里就有结果，例如 <kbd>v55+15</kbd>。</p>
        </div>
      )}
    </div>
  );
}
