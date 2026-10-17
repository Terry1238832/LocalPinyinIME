import type { CSSProperties } from 'react';

// Small looping demonstrations shown in a setting's details panel. With
// reduced motion the demonstration rests on its middle frame (keys typed,
// candidates shown) instead of animating.

// Pinyin is typed after `before`, candidates appear, then the first one is
// committed and stays in the line as `after`.
export function TypingDemo({ before = '', keys, candidates, after }: {
  before?: string; keys: string; candidates: string[]; after: string;
}) {
  return <div className="feature-demo typing-demo" aria-hidden="true" style={{ '--keys': Math.max(1, keys.length) } as CSSProperties}>
    <div className="demo-line">
      <span>{before}</span>
      <span className="demo-result">{after}</span>
      {keys && <span className="demo-keys">{keys}</span>}
      <span className="demo-caret" />
    </div>
    <div className="demo-candidates">{candidates.map((word, index) =>
      <span key={`${word}-${index}`} className={index === 0 ? 'demo-first' : undefined}><b>{index + 1}</b>{word}</span>)}
    </div>
  </div>;
}

// Keys are pressed after `before`; `output` then appears in the line, or the
// mode badge turns from the first mode to the second.
export function KeyDemo({ before = '', keys, output, modes, mono = false }: {
  before?: string; keys: string[]; output?: string; modes?: [string, string]; mono?: boolean;
}) {
  return <div className={`feature-demo key-demo${mono ? ' mono' : ''}`} aria-hidden="true">
    <div className="demo-line">
      <span>{before}</span>
      {output && <span className="demo-output">{output}</span>}
      <span className="demo-caret" />
      {modes && <span className="demo-modes"><i>{modes[0]}</i><i>{modes[1]}</i></span>}
    </div>
    <div className="demo-keycaps">{keys.map((key, index) =>
      <kbd key={`${key}-${index}`}>{key}</kbd>)}
    </div>
  </div>;
}
