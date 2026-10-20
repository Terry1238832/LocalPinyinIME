// 工具: what is typed here turns into results at once — a sum, a number in
// Chinese words or capitals, a date, the current time. The same tools work
// in any input box after a v (v55+15). Pinyin letters get tone marks, one
// vowel at a time (ai: a's tone, then i's).
import { useEffect, useRef, useState } from 'react';
import { Icon } from '../icons';

const EXAMPLES = [
  { title: '数字计算', input: '55+15', hint: '加减乘除、括号、百分号' },
  { title: '大小写转换', input: '123', hint: '中文数字、大写金额' },
  { title: '日期格式', input: '12.19', hint: '月.日 或 年.月.日' },
  { title: '当前时间', input: 'sj', hint: 'sj 时间 · rq 日期 · xq 星期' },
  { title: '拼音声调', input: 'ai', hint: '逐个选每个韵母的声调：1–4 声，5 不标；v 是 ü' },
];

// Each vowel unmarked, then in tones one to four (as the input method's tool).
const TONE_ROWS: Record<string, string> = { a: 'aāáǎà', e: 'eēéěè', i: 'iīíǐì', o: 'oōóǒò', u: 'uūúǔù', v: 'üǖǘǚǜ', ü: 'üǖǘǚǜ' };
const TONE_ORDER = [1, 2, 3, 4, 0];
const TONE_NAMES = ['不标声调', '第一声', '第二声', '第三声', '第四声'];
const isToneText = (text: string) => /^[a-zü]{1,32}$/.test(text) && /[aeiouvü]/.test(text);
const toneVowels = (text: string) => [...text].filter(ch => ch in TONE_ROWS);
function toneText(text: string, tones: number[]) {
  let vowel = 0;
  return [...text].map(ch => {
    const row = TONE_ROWS[ch];
    if (!row) return ch;
    const tone = tones[vowel++] ?? 0;
    return row[tone >= 1 && tone <= 4 ? tone : 0];
  }).join('');
}

export function Tools({ onInsert, focusKey }: { onInsert: (text: string) => void; focusKey: number }) {
  const [input, setInput] = useState('');
  const [results, setResults] = useState<string[]>([]);
  const [selected, setSelected] = useState(0);
  const [previews, setPreviews] = useState<Record<string, string>>({ ai: 'ā á ǎ à' });
  // 声调: the tones chosen so far for the vowels, in order.
  const [tones, setTones] = useState<number[]>([]);
  const field = useRef<HTMLInputElement>(null);
  const word = input.trim().toLowerCase();
  const toning = isToneText(word);
  const vowels = toning ? toneVowels(word) : [];
  const toneRows = toning ? TONE_ORDER.map(tone => ({ tone, value: toneText(word, [...tones, tone]) })) : [];

  useEffect(() => { field.current?.focus(); }, [focusKey]);
  // What each example turns into; the clock ticks here rather than asking
  // the input method every second.
  useEffect(() => {
    let live = true;
    void Promise.all(EXAMPLES.filter(example => example.input !== 'sj' && example.input !== 'ai').map(example => window.localPinyinPanel.tools(example.input)
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
  // Changed letters keep the tones of the vowels still there, one left to choose.
  useEffect(() => {
    setTones(list => list.slice(0, Math.max(0, toneVowels(input.trim().toLowerCase()).length - 1)));
    setSelected(0);
  }, [input]);
  useEffect(() => {
    const value = input.trim();
    if (!value || isToneText(value.toLowerCase())) { setResults([]); return; }
    let live = true;
    const timer = window.setTimeout(() => {
      window.localPinyinPanel.tools(value).then(result => { if (live) { setResults(result.results); setSelected(0); } })
        .catch(() => { if (live) setResults([]); });
    }, 60);
    return () => { live = false; window.clearTimeout(timer); };
  }, [input]);

  // A tone for this vowel; the pinyin goes in after the last one.
  const chooseTone = (tone: number) => {
    const next = [...tones, tone];
    if (next.length < vowels.length) { setTones(next); setSelected(0); return; }
    setTones([]);
    onInsert(toneText(word, next));
  };
  const rows = toning ? toneRows.length : results.length;

  return (
    <div className="tools">
      <label className="tool-field">
        <Icon name="tools" size={18} />
        <input ref={field} value={input} spellCheck={false} placeholder="输入 55+15、123、12.19、sj，或拼音 ai 标声调"
               aria-label="要转换的内容" maxLength={120}
               onChange={event => setInput(event.target.value)}
               onKeyDown={event => {
                 if (event.key === 'ArrowDown') { event.preventDefault(); setSelected(index => Math.min(index + 1, rows - 1)); }
                 else if (event.key === 'ArrowUp') { event.preventDefault(); setSelected(index => Math.max(index - 1, 0)); }
                 else if (toning && /^[1-5]$/.test(event.key)) { event.preventDefault(); chooseTone(TONE_ORDER[Number(event.key) - 1]); }
                 else if (toning && event.key === 'Backspace' && tones.length) { event.preventDefault(); setTones(list => list.slice(0, -1)); setSelected(0); }
                 else if (event.key === 'Enter' && toning && toneRows[selected]) { event.preventDefault(); chooseTone(toneRows[selected].tone); }
                 else if (event.key === 'Enter' && !toning && results[selected]) { event.preventDefault(); onInsert(results[selected]); }
               }} />
        {input && <button type="button" className="icon-button" aria-label="清空" onClick={() => { setInput(''); field.current?.focus(); }}>
          <Icon name="close" size={14} /></button>}
      </label>
      {toning ? (
        <div className="tone-step">
          <p className="tone-progress">选 <b>{vowels[tones.length] === 'v' ? 'ü' : vowels[tones.length]}</b> 的声调
            <span>第 {tones.length + 1} 个，共 {vowels.length} 个{tones.length ? ' · Backspace 上一个' : ''}</span></p>
          <ol className="tool-results" aria-label="声调">
            {toneRows.map((row, index) => (
              <li key={row.tone}>
                <button type="button" className={index === selected ? 'tool-result selected' : 'tool-result'}
                        onMouseEnter={() => setSelected(index)} onClick={() => chooseTone(row.tone)}>
                  <span className="tool-index">{index + 1}</span><span className="tool-value">{row.value}</span>
                  <span className="tone-name">{TONE_NAMES[row.tone]}</span>
                </button>
              </li>
            ))}
          </ol>
        </div>
      ) : input.trim() ? (
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
        ) : <p className="empty-note">这个还不会转换。可以试试算式（3*(4+5)）、数字（2024）、日期（2024.12.19）、sj，或拼音（ai）标声调。</p>
      ) : (
        <div className="tool-cards">
          {EXAMPLES.map(example => (
            <button key={example.title} type="button" className={example.input === 'ai' ? 'tool-card wide' : 'tool-card'}
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
