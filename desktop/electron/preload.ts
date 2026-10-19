import { contextBridge, ipcRenderer } from 'electron';
import type { AppState, BackupMode, BackupPart, Settings, UpdateStatus, UserEntry } from '../src/types';

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
  listPhrases: () => ipcRenderer.invoke('localpinyin:phrases'),
  addPhrase: (text: string, label: string) => ipcRenderer.invoke('localpinyin:phrase-add', text, label),
  deletePhrase: (pinyin: string, text: string) => ipcRenderer.invoke('localpinyin:phrase-delete', pinyin, text),
  listEmojiCandidates: () => ipcRenderer.invoke('localpinyin:emoji-candidates'),
  deleteEmojiCandidate: (pinyin: string, text: string) => ipcRenderer.invoke('localpinyin:emoji-candidate-delete', pinyin, text),
  clipboardSummary: () => ipcRenderer.invoke('localpinyin:clipboard-summary'),
  clearClipboard: () => ipcRenderer.invoke('localpinyin:clipboard-clear'),
  chooseBackupFile: (mode: 'save' | 'open', name?: string) => ipcRenderer.invoke('localpinyin:backup-choose', mode, name),
  exportBackup: (parts: BackupPart[], file: string) => ipcRenderer.invoke('localpinyin:backup-export', parts, file),
  inspectBackup: (file: string) => ipcRenderer.invoke('localpinyin:backup-inspect', file),
  importBackup: (file: string, choices: Partial<Record<BackupPart, BackupMode>>) => ipcRenderer.invoke('localpinyin:backup-import', file, choices),
  vaultStatus: () => ipcRenderer.invoke('localpinyin:vault-status'),
  vaultCreate: (password: string) => ipcRenderer.invoke('localpinyin:vault-create', password),
  vaultChangePassword: (current: string, next: string) => ipcRenderer.invoke('localpinyin:vault-password', current, next),
  vaultDestroy: () => ipcRenderer.invoke('localpinyin:vault-destroy'),
  onSettingsChanged: (listener: () => void) => {
    const forward = () => listener();
    ipcRenderer.on('localpinyin:settings-changed', forward);
    return () => { ipcRenderer.removeListener('localpinyin:settings-changed', forward); };
  },
  openDataDirectory: () => ipcRenderer.invoke('localpinyin:open-data'),
  windowControl: (action: 'minimize' | 'maximize' | 'close') => ipcRenderer.invoke('localpinyin:window', action),
  getUpdateStatus: (): Promise<UpdateStatus> => ipcRenderer.invoke('localpinyin:update-status'),
  checkForUpdate: (): Promise<UpdateStatus> => ipcRenderer.invoke('localpinyin:update-check'),
  downloadUpdate: (): Promise<UpdateStatus> => ipcRenderer.invoke('localpinyin:update-download'),
  installUpdate: (): Promise<{ started: boolean }> => ipcRenderer.invoke('localpinyin:update-install'),
  setAutoUpdateCheck: (enabled: boolean): Promise<UpdateStatus> => ipcRenderer.invoke('localpinyin:update-auto', enabled),
  openReleasePage: (): Promise<void> => ipcRenderer.invoke('localpinyin:update-page'),
  onUpdateStatus: (listener: (status: UpdateStatus) => void) => {
    const forward = (_event: unknown, status: UpdateStatus) => listener(status);
    ipcRenderer.on('localpinyin:update-status', forward);
    return () => { ipcRenderer.removeListener('localpinyin:update-status', forward); };
  },
}));
