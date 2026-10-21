import { Keys, PageHeader, SaveStatus, SettingRow, SettingStatus, SettingsGroup, Switch } from './components';
import { KeyDemo, TypingDemo } from './FeatureDemo';
import type { AppState, Settings } from './types';

type EnglishPageProps = { state: AppState; saving: boolean; saveSettings: (settings: Settings) => Promise<void> };

export const imeSwitchKeys: { value: Settings['imeSwitchKey']; label: string; keys: string[] }[] = [
  { value: 'alt+capslock', label: 'Alt + CapsLock', keys: ['Alt', 'CapsLock'] },
  { value: 'ctrl+capslock', label: 'Ctrl + CapsLock', keys: ['Ctrl', 'CapsLock'] },
  { value: 'ctrl+alt+space', label: 'Ctrl + Alt + 空格', keys: ['Ctrl', 'Alt', 'Space'] },
  { value: 'off', label: '不用', keys: [] },
];

// 本地英文, the second input method: whether it is in Windows' list, how to
// switch to it, how it types, and its keys.
export function EnglishPage({ state, saving, saveSettings }: EnglishPageProps) {
  const settings = state.settings;
  const on = settings.englishIme;
  const switchKeys = imeSwitchKeys.find(item => item.value === settings.imeSwitchKey)?.keys ?? [];
  const sentence = settings.englishSentence;
  const shortcuts: { title: string; keys: string[]; joiner?: string }[] = [
    ...(on && switchKeys.length ? [{ title: '切换到本地拼音', keys: switchKeys }] : []),
    { title: '选择输入法', keys: ['Win', 'Space'] },
    sentence ? { title: '空格，句子接着打', keys: ['Space'] } : { title: '放进单词和空格', keys: ['Space'] },
    ...(sentence ? [{ title: '放进改好的整句', keys: ['Space', 'Space'], joiner: '' }] : []),
    { title: '选择候选', keys: ['1', '9'], joiner: '–' },
    { title: '放进候选，不加空格', keys: ['Tab'] },
    { title: '原样放进去', keys: ['Enter'] },
    { title: '刚改过时换回原样', keys: ['Backspace'] },
    { title: '翻页', keys: ['←', '→'], joiner: '' },
    { title: '取消输入', keys: ['Esc'] },
    { title: '辅助拼写开或关', keys: ['Ctrl', 'Space'] },
    ...(settings.panelDoubleTab ? [{ title: '打开工具面板', keys: ['Tab', 'Tab'], joiner: '' }] : []),
  ];
  return <>
    <PageHeader kicker="ying'wen" title="本地英文" description="第二个输入法，专门打英文：整句输入，拼写和常见语法当场改好，全部在本机完成。" action={<SaveStatus saving={saving} />} />
    <section className={`english-hero${on ? ' on' : ''}`} aria-label="本地英文输入法" data-tour="english-ime">
      <span className="english-mark" aria-hidden="true">英</span>
      <div className="english-hero-copy">
        <h2>本地英文输入法</h2>
        <p>{on
          ? <>已在 Windows 的输入法列表中{switchKeys.length ? <>，按 <Keys keys={switchKeys} /> 和本地拼音互相切换</> : '，用 Win + 空格切换'}</>
          : '已关闭：不在输入法列表中，切换快捷键也不占用。本地拼音不受影响。'}</p>
      </div>
      <Switch checked={on} disabled={saving} label="本地英文输入法" onChange={englishIme => void saveSettings({ ...settings, englishIme })} />
    </section>
    <SettingsGroup id="group-english-switch" title="切换">
      <SettingRow title="在本地拼音和本地英文之间切换"
        description="按下就换到另一个输入法，和 Win + 空格一样；用 CapsLock 的组合不会改变大小写状态。关闭本地英文时不占用这个快捷键。"
        demo={<KeyDemo before="" keys={switchKeys.length ? switchKeys : ['Win', 'Space']} modes={['中', '英']} />}>
        <div className="segmented" role="radiogroup" aria-label="切换输入法的快捷键">{imeSwitchKeys.map(item =>
          <label key={item.value} className={settings.imeSwitchKey === item.value ? 'active' : undefined}>
            <input type="radio" name="ime-switch-key" value={item.value} checked={settings.imeSwitchKey === item.value} disabled={saving || !on}
              onChange={() => void saveSettings({ ...settings, imeSwitchKey: item.value })} />{item.label}
          </label>)}
        </div>
      </SettingRow>
      <SettingRow title="切换辅助拼写" description="在本地英文里按 Ctrl + Space，或单独按 Shift（开启“输入 › 按 Shift 切换中英文”时），在“辅”（候选与纠错）和“英”（直接输入，不纠正）之间切换。"
        demo={<KeyDemo before="hello" keys={['Ctrl', 'Space']} modes={['辅', '英']} />}><SettingStatus>始终可用</SettingStatus></SettingRow>
    </SettingsGroup>
    <SettingsGroup id="group-english-typing" title="打字">
      <SettingRow title="整句输入"
        description="打完一整句再放进去：按一下空格只是空格，句子接着打；连按两下空格放进整句，拼写、句首大写、a / an、连打两遍的词等一起改好，紧接着按退格换回原样；Enter 原样放进去。关闭后按空格就把当前单词放进去。"
        demo={<TypingDemo before="" keys="i saw a owl" candidates={['I saw an owl', 'i saw a owl']} after="I saw an owl" />}>
        <Switch checked={sentence} disabled={saving} label="整句输入"
          onChange={englishSentence => void saveSettings({ ...settings, englishSentence })} />
      </SettingRow>
      <SettingRow title="自动纠正拼写"
        description="打错的单词在放进去时改成正确的（teh → the，dont → don't），紧接着按退格改回原样，以后不再改它。关闭后保持原样，正确的写法仍在候选里。"
        demo={<TypingDemo before="" keys="teh" candidates={['the', 'teh', 'ten']} after="the" />}>
        <Switch checked={settings.englishAutocorrect} disabled={saving} label="自动纠正拼写"
          onChange={englishAutocorrect => void saveSettings({ ...settings, englishAutocorrect })} />
      </SettingRow>
    </SettingsGroup>
    <section className="legend" aria-labelledby="english-legend-title">
      <h2 className="block-title" id="english-legend-title">快捷键<span>在本地英文里可用</span></h2>
      <ul className="legend-grid">{shortcuts.map(item =>
        <li key={item.title}><Keys keys={item.keys} joiner={item.joiner ?? '+'} /><span>{item.title}</span></li>)}
      </ul>
    </section>
  </>;
}
