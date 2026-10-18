import { useEffect, useRef, useState } from 'react';
import { SettingDisclosure, SettingRow, SettingStatus, SettingsGroup, Switch } from './components';
import { KeyDemo, TypingDemo } from './FeatureDemo';
import { Icon } from './icons';
import type { DoublePinyin, FuzzyPair, Settings, UsageDomain } from './types';

// Usage scenes, spelling correction, Chinese-English mixed input, fuzzy
// pinyin and the double-pinyin scheme. All of them change only how typed
// letters become candidates; the native engine (src/engine) applies them in
// the IME and in the previews here alike.

// Usage scenes: word packs in resources/dictionary/domains that rank higher
// while chosen. The examples are words of each pack.
const scenes: { id: UsageDomain; label: string; examples: string }[] = [
  { id: 'coding', label: '编程开发', examples: '函数 · 接口 · 部署 · 回滚' },
  { id: 'work', label: '办公职场', examples: '会议 · 报销 · 审批 · 排期' },
  { id: 'math', label: '数学理科', examples: '导数 · 积分 · 矩阵 · 向量' },
  { id: 'study', label: '学习考试', examples: '作业 · 复习 · 绩点 · 答辩' },
  { id: 'daily', label: '日常聊天', examples: '其实 · 咋办 · 外卖 · 好嘞' },
  { id: 'games', label: '游戏娱乐', examples: '开黑 · 上分 · 段位 · 副本' },
];

const initialPairs: { id: FuzzyPair; a: string; b: string }[] = [
  { id: 'z-zh', a: 'z', b: 'zh' }, { id: 'c-ch', a: 'c', b: 'ch' }, { id: 's-sh', a: 's', b: 'sh' },
  { id: 'n-l', a: 'n', b: 'l' }, { id: 'f-h', a: 'f', b: 'h' }, { id: 'r-l', a: 'r', b: 'l' },
];
const finalPairs: { id: FuzzyPair; a: string; b: string }[] = [
  { id: 'an-ang', a: 'an', b: 'ang' }, { id: 'en-eng', a: 'en', b: 'eng' }, { id: 'in-ing', a: 'in', b: 'ing' },
  { id: 'ian-iang', a: 'ian', b: 'iang' }, { id: 'uan-uang', a: 'uan', b: 'uang' },
];
const allPairs = [...initialPairs, ...finalPairs];

const schemes: { value: DoublePinyin; label: string }[] = [
  { value: 'none', label: '全拼' },
  { value: 'xiaohe', label: '小鹤双拼' },
  { value: 'ziranma', label: '自然码' },
];

// What each key means after an initial (second key), per scheme.
const finals: Record<Exclude<DoublePinyin, 'none'>, Record<string, string>> = {
  xiaohe: {
    q: 'iu', w: 'ei', e: 'e', r: 'uan', t: 'ue', y: 'un', u: 'u', i: 'i', o: 'uo o', p: 'ie',
    a: 'a', s: 'ong', d: 'ai', f: 'en', g: 'eng', h: 'ang', j: 'an', k: 'ing uai', l: 'iang',
    z: 'ou', x: 'ia ua', c: 'ao', v: 'ui ü', b: 'in', n: 'iao', m: 'ian',
  },
  ziranma: {
    q: 'iu', w: 'ia ua', e: 'e', r: 'uan', t: 'ue', y: 'ing uai', u: 'u', i: 'i', o: 'uo o', p: 'un',
    a: 'a', s: 'ong', d: 'iang', f: 'en', g: 'eng', h: 'ang', j: 'an', k: 'ao', l: 'ai',
    z: 'ei', x: 'ie', c: 'iao', v: 'ui ü', b: 'ou', n: 'in', m: 'ian',
  },
};
const digraphs: Record<string, string> = { v: 'zh', i: 'ch', u: 'sh' };
const rows = ['qwertyuiop', 'asdfghjkl', 'zxcvbnm'];

function Keymap({ scheme }: { scheme: Exclude<DoublePinyin, 'none'> }) {
  const label = scheme === 'xiaohe' ? '小鹤双拼键位' : '自然码键位';
  return <figure className="keymap" aria-label={label}>
    {rows.map(row => <div className="keymap-row" key={row}>{[...row].map(key =>
      <span className="keymap-key" key={key}>
        <b>{key.toUpperCase()}</b>
        {digraphs[key] && <em>{digraphs[key]}</em>}
        <small>{finals[scheme][key]}</small>
      </span>)}
    </div>)}
    <figcaption>每个字两键：先按声母键（V=zh，I=ch，U=sh），再按韵母键。没有声母的音节按读音的头两个字母，如 AI=ai、AH=ang、EG=eng、OU=ou。</figcaption>
  </figure>;
}

type Props = { settings: Settings; saving: boolean; onChange: (next: Settings) => void };

export function InputOptions({ settings: saved, saving, onChange: save }: Props) {
  // Shown at once and saved in the background; the saved state wins again
  // once a save finishes (or fails).
  const [settings, setSettings] = useState(saved);
  useEffect(() => { if (!saving) setSettings(saved); }, [saved, saving]);
  const onChange = (next: Settings) => {
    setSettings(next);
    save(next);
  };
  const [fuzzyOpen, setFuzzyOpen] = useState(false);
  const [wordsOpen, setWordsOpen] = useState(false);
  const [sceneWords, setSceneWords] = useState<Partial<Record<UsageDomain, string[]>>>({});
  useEffect(() => {
    let current = true;
    window.localPinyin.domainWords().then(result => { if (current) setSceneWords(result.domains); }).catch(() => undefined);
    return () => { current = false; };
  }, []);
  const [keymapOpen, setKeymapOpen] = useState(false);
  // Choosing a double-pinyin scheme opens its key map once.
  const previousScheme = useRef(settings.doublePinyin);
  useEffect(() => {
    if (previousScheme.current === 'none' && settings.doublePinyin !== 'none') setKeymapOpen(true);
    previousScheme.current = settings.doublePinyin;
  }, [settings.doublePinyin]);

  const toggle = (id: FuzzyPair) => {
    const set = new Set(settings.fuzzyPinyin);
    if (set.has(id)) set.delete(id); else set.add(id);
    onChange({ ...settings, fuzzyPinyin: allPairs.map(pair => pair.id).filter(pair => set.has(pair)) });
  };
  const chip = (pair: { id: FuzzyPair; a: string; b: string }) => {
    const on = settings.fuzzyPinyin.includes(pair.id);
    return <button key={pair.id} type="button" className="fuzzy-chip" aria-pressed={on} aria-label={`${pair.a} 与 ${pair.b} 不区分`}
      disabled={saving} onClick={() => toggle(pair.id)}>
      <span>{pair.a}</span><i aria-hidden="true">=</i><span>{pair.b}</span>
    </button>;
  };
  const toggleScene = (id: UsageDomain) => {
    const set = new Set(settings.usageDomains);
    if (set.has(id)) set.delete(id); else set.add(id);
    onChange({ ...settings, usageDomains: scenes.map(scene => scene.id).filter(scene => set.has(scene)) });
  };
  const chosenScenes = scenes.filter(scene => settings.usageDomains.includes(scene.id));
  const chosenWords = chosenScenes.reduce((total, scene) => total + (sceneWords[scene.id]?.length ?? 0), 0);
  const enabledPairs = allPairs.filter(pair => settings.fuzzyPinyin.includes(pair.id));
  const double = settings.doublePinyin !== 'none';
  const paused = double ? '使用双拼时暂停。' : '';
  return <>
    <SettingsGroup id="group-scenes" title="常用场景" description="选择你常打字的场景，其中的词语会排得更靠前，词库里没有的场景词也会出现。可以多选；例如选“日常聊天”后，“所以整道题”后面输入 qishi 首选“其实”。"
      demo={<TypingDemo before="所以整道题" keys="qishi" candidates={['其实', '歧视', '骑士']} after="其实" />}>
      <div className="scene-picker" role="group" aria-label="常用场景">{scenes.map(scene => {
        const on = settings.usageDomains.includes(scene.id);
        return <button key={scene.id} type="button" className="scene-card" aria-pressed={on} disabled={saving} onClick={() => toggleScene(scene.id)}>
          <span className="scene-name">{scene.label}{on && <Icon name="check" size={14} />}</span>
          <span className="scene-examples">{scene.examples}</span>
        </button>;
      })}</div>
      <SettingDisclosure title="场景词条" description="选中的场景包含的词语。某个词不想要时，可以在“词库”页的个人词库里调整它的频率。"
        summary={chosenScenes.length ? `${chosenScenes.length} 个场景${chosenWords ? ` · ${chosenWords} 个词` : ''}` : '未选择'}
        open={wordsOpen} onToggle={setWordsOpen}>
        {chosenScenes.length ? chosenScenes.map(scene => <div className="scene-words" key={scene.id}>
          <span className="scene-words-label">{scene.label}</span>
          <p>{(sceneWords[scene.id] ?? []).map(word => <span key={word}>{word}</span>)}</p>
        </div>) : <p className="disclosure-note">先在上面选择场景。</p>}
      </SettingDisclosure>
    </SettingsGroup>
    <SettingsGroup id="group-spelling" title="拼写">
      <SettingRow title="拼写纠错" description={`打错、漏打、颠倒或按到相邻键时推荐正确的词，例如 nihso → 你好。${paused}`}
        demo={<TypingDemo keys="nihso" candidates={['你好', '你', '泥']} after="你好" />}>
        <Switch checked={settings.typoCorrection} disabled={saving} label="拼写纠错"
          onChange={typoCorrection => onChange({ ...settings, typoCorrection })} />
      </SettingRow>
      <SettingRow title="中英混输" description={`在拼音里直接输入英文单词和缩写，不用切换，例如 woyongpython → 我用 Python。${paused}`}
        demo={<TypingDemo keys="woyongpython" candidates={['我用 Python', '我用']} after="我用 Python" />}>
        <Switch checked={settings.mixedEnglish} disabled={saving} label="中英混输"
          onChange={mixedEnglish => onChange({ ...settings, mixedEnglish })} />
      </SettingRow>
      <SettingDisclosure title="模糊音" description="把分不清的读音当作相同，例如开启 z=zh 后 zongguo 也能打出“中国”。"
        summary={enabledPairs.length ? `已开启 ${enabledPairs.length} 项` : '关闭'} open={fuzzyOpen} onToggle={setFuzzyOpen}>
        <div className="fuzzy-groups">
          <div className="fuzzy-group" role="group" aria-label="声母模糊音"><span className="fuzzy-label">声母</span>{initialPairs.map(chip)}</div>
          <div className="fuzzy-group" role="group" aria-label="韵母模糊音"><span className="fuzzy-label">韵母</span>{finalPairs.map(chip)}</div>
        </div>
        <div className="disclosure-foot">
          <span>{enabledPairs.length ? `已开启：${enabledPairs.map(pair => `${pair.a}=${pair.b}`).join('、')}` : '可以选择多项。'}</span>
          <button type="button" className="text-button" disabled={saving || !enabledPairs.length}
            onClick={() => onChange({ ...settings, fuzzyPinyin: [] })}>全部关闭</button>
        </div>
      </SettingDisclosure>
    </SettingsGroup>
    <SettingsGroup id="group-scheme" title="输入方案">
      <SettingRow title="拼音方案" description={double ? '双拼每个字按两个键，例如小鹤双拼 nihc → 你好。中英混输和拼写纠错暂停，模糊音照常使用。' : '全拼逐个字母输入完整拼音；也可以换成小鹤双拼或自然码，每个字只按两个键。'}
        demo={<TypingDemo keys={settings.doublePinyin === 'ziranma' ? 'nihk' : 'nihc'} candidates={['你好', '你']} after="你好" />}>
        <div className="segmented" role="radiogroup" aria-label="输入方案">{schemes.map(item =>
          <label key={item.value} className={settings.doublePinyin === item.value ? 'active' : undefined}>
            <input type="radio" name="double-pinyin" value={item.value} checked={settings.doublePinyin === item.value} disabled={saving}
              onChange={() => onChange({ ...settings, doublePinyin: item.value })} />{item.label}
          </label>)}
        </div>
      </SettingRow>
      {double && <SettingDisclosure title="键位图" description="每个键代表的声母和韵母。"
        summary={settings.doublePinyin === 'xiaohe' ? '小鹤双拼' : '自然码'} open={keymapOpen} onToggle={setKeymapOpen}>
        <Keymap scheme={settings.doublePinyin as Exclude<DoublePinyin, 'none'>} />
      </SettingDisclosure>}
      <SettingRow title="简拼" description="只输入每个字的声母，例如 bjdx → 北京大学；zh、ch、sh 也可以两个字母一起输入。"
        demo={<TypingDemo keys="bjdx" candidates={['北京大学', '本届大学']} after="北京大学" />}>
        <SettingStatus>始终开启</SettingStatus>
      </SettingRow>
    </SettingsGroup>
    <SettingsGroup id="group-helpers" title="辅助功能">
      <SettingRow title="数字后打。变成 ." description="数字后面按句号键，打出英文句点：3.14、v1.2、192.168.1.1。其他地方仍是“。”。"
        demo={<KeyDemo before="π ≈ 3" keys={['。']} output=".14" />}>
        <Switch checked={settings.digitPeriod} disabled={saving} label="数字后打。变成 ."
          onChange={digitPeriod => onChange({ ...settings, digitPeriod })} />
      </SettingRow>
      <SettingRow title="数字后使用英文标点" description="数字后面的逗号、斜杠和冒号输入英文半角：1,000、1/2、12:30。其他地方仍是中文标点。"
        demo={<KeyDemo before="开会时间 12" keys={['：']} output=":30" />}>
        <Switch checked={settings.digitPunctuation} disabled={saving} label="数字后使用英文标点"
          onChange={digitPunctuation => onChange({ ...settings, digitPunctuation })} />
      </SettingRow>
      <SettingRow title="字母后使用英文标点"
        description="英文字母后面的标点都输入英文半角：Hello, world.、don't、OK!。默认关闭：开启后，“我用 Python。”结尾的句号也会变成英文句点。"
        demo={<KeyDemo before="Hello" keys={['，']} output=", world" />}>
        <Switch checked={settings.letterPunctuation} disabled={saving} label="字母后使用英文标点"
          onChange={letterPunctuation => onChange({ ...settings, letterPunctuation })} />
      </SettingRow>
      <SettingRow title="编程软件中使用英文标点"
        description="在 VS Code、Cursor、Visual Studio、JetBrains 系列、Android Studio、Sublime Text、Notepad++ 和终端里，标点按英文半角输入：< ( { [ ! ? , 等。编辑器自己的括号自动配对照常工作。"
        demo={<KeyDemo before="print" keys={['Shift', '9']} output="(" mono />}>
        <Switch checked={settings.codePunctuation} disabled={saving} label="编程软件中使用英文标点"
          onChange={codePunctuation => onChange({ ...settings, codePunctuation })} />
      </SettingRow>
    </SettingsGroup>
  </>;
}
