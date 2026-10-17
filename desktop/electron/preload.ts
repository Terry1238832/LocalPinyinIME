import { contextBridge, ipcRenderer } from 'electron';
import type { AppState, Settings, UserEntry } from '../src/types';

const initialState = ipcRenderer.sendSync('localpinyin:bootstrap') as AppState | null;
contextBridge.exposeInMainWorld('localPinyin', Object.freeze({
  initialState,
  getState: (): Promise<AppState> => ipcRenderer.invoke('localpinyin:state'),
  saveSettings: (settings: Settings): Promise<AppState> => ipcRenderer.invoke('localpinyin:save-settings', settings),
  listUserLexicon: () => ipcRenderer.invoke('localpinyin:user-list'),
  upsertUserEntry: (entry: UserEntry, previous?: Pick<UserEntry, 'pinyin' | 'word'>) => ipcRenderer.invoke('localpinyin:user-upsert', entry, previous),
  deleteUserEntry: (entry: Pick<UserEntry, 'pinyin' | 'word'>) => ipcRenderer.invoke('localpinyin:user-delete', entry),
  searchDictionary: (query: string, limit = 100) => ipcRenderer.invoke('localpinyin:dictionary-search', query, limit),
  clearLearning: () => ipcRenderer.invoke('localpinyin:clear-learning'),
  domainWords: () => ipcRenderer.invoke('localpinyin:domain-words'),
  openDataDirectory: () => ipcRenderer.invoke('localpinyin:open-data'),
  windowControl: (action: 'minimize' | 'maximize' | 'close') => ipcRenderer.invoke('localpinyin:window', action),
}));
