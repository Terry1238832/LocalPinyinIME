import { contextBridge, ipcRenderer } from 'electron';
import type { ClipQuery, PanelApi, PanelKind, Pick, VaultItem } from '../src/panel/types';

const invoke = (channel: string, ...args: unknown[]) => ipcRenderer.invoke('localpinyin:panel:' + channel, ...args);

const api: PanelApi = {
  bootstrap: ipcRenderer.sendSync('localpinyin:panel:bootstrap') as PanelApi['bootstrap'],
  data: () => invoke('data'),
  close: () => invoke('close'),
  insert: (text: string, picks?: Pick[]) => invoke('insert', text, picks),
  paste: (stickerId: string) => invoke('paste', stickerId),
  favorite: (kind: PanelKind, value: string, on: boolean) => invoke('favorite', kind, value, on),
  tools: (input: string) => invoke('tools', input),
  phrases: () => invoke('phrases'),
  addPhrase: (text: string, labels: string[]) => invoke('phrase-add', text, labels),
  deletePhrase: (pinyin: string, text: string) => invoke('phrase-delete', pinyin, text),
  learnEmoji: (emoji: string, names: string[]) => invoke('learn-emoji', emoji, names),
  addStickers: (files: { name: string; data: Uint8Array }[]) => invoke('sticker-add', files),
  deleteSticker: (id: string) => invoke('sticker-delete', id),
  choosingFile: (on: boolean) => invoke('choosing-file', on),
  keepOpen: (on: boolean) => invoke('keep-open', on),
  openSettings: () => invoke('settings'),
  clipboard: (query: ClipQuery) => invoke('clipboard', query),
  clipboardText: (id: string) => invoke('clipboard-text', id),
  clipboardPaste: (id: string) => invoke('clipboard-paste', id),
  clipboardDelete: (id: string) => invoke('clipboard-delete', id),
  clipboardConsent: (on: boolean) => invoke('clipboard-consent', on),
  vaultStatus: () => invoke('vault-status'),
  vaultCreate: (password: string) => invoke('vault-create', password),
  vaultUnlock: (password: string) => invoke('vault-unlock', password),
  vaultItems: () => invoke('vault-items'),
  vaultSave: (item: VaultItem) => invoke('vault-save', item),
  vaultDelete: (id: string) => invoke('vault-delete', id),
  vaultLock: () => invoke('vault-lock'),
  vaultCopy: (text: string) => invoke('vault-copy', text),
  onOpen: (listener: () => void) => {
    const forward = () => listener();
    ipcRenderer.on('localpinyin:panel-open', forward);
    return () => { ipcRenderer.removeListener('localpinyin:panel-open', forward); };
  },
};
contextBridge.exposeInMainWorld('localPinyinPanel', Object.freeze(api));
