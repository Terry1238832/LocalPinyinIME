export type Theme = 'system' | 'light' | 'dark';
// Candidate window styles: Cursor Rail, Modern Type, Floating Glass, Adaptive
// Flow, Capsule, Column List, Split-Flap and Highlighter, by their native ids.
export const candidateStyleIds = ['rail', 'type', 'glass', 'flow', 'capsule', 'column', 'flap', 'marker'] as const;
export type CandidateStyle = (typeof candidateStyleIds)[number];
// Fuzzy pinyin pairs, written as the native settings store them.
export type FuzzyPair = 'z-zh' | 'c-ch' | 's-sh' | 'n-l' | 'f-h' | 'r-l' | 'an-ang' | 'en-eng' | 'in-ing' | 'ian-iang' | 'uan-uang';
export type DoublePinyin = 'none' | 'xiaohe' | 'ziranma';
// Usage scenes (常用场景): word packs that rank higher while chosen.
export type UsageDomain = 'coding' | 'work' | 'math' | 'study' | 'daily' | 'games';
// Clipboard history: asked once after installing or updating, then on or off.
export type ClipboardHistory = 'ask' | 'on' | 'off';
// Days copies are kept; 0 keeps them until deleted.
export const clipboardRetentionChoices = [1, 7, 30, 90, 365, 0] as const;

export interface Settings {
  theme: Theme;
  textSize: number;
  candidateStyle: CandidateStyle;
  // Spelling correction and Chinese-English mixed input (on by default),
  // fuzzy pinyin pairs (none by default) and the double-pinyin scheme.
  typoCorrection: boolean;
  mixedEnglish: boolean;
  fuzzyPinyin: FuzzyPair[];
  doublePinyin: DoublePinyin;
  usageDomains: UsageDomain[];
  // Helpers (on by default): 3. after a digit, ASCII punctuation in code editors.
  digitPeriod: boolean;
  digitPunctuation: boolean;
  letterPunctuation: boolean;
  codePunctuation: boolean;
  // Tab translates what was just typed into English (online, opt-in).
  tabTranslate: boolean;
  // The tool panel: Tab twice opens it; it may wait in the background
  // instead of leaving ten minutes after its last use. Emoji used from it
  // are offered among the candidates for their names.
  panelDoubleTab: boolean;
  panelResident: boolean;
  emojiCandidates: boolean;
  clipboardHistory: ClipboardHistory;
  clipboardRetentionDays: number;
  shiftToggleEnabled: boolean;
  predictionEnabled: boolean;
}

// 输入习惯: typing `label` (read as `pinyin`) offers `text` first.
export interface Phrase { pinyin: string; text: string; label: string; created: number }

// 记录: what the user keeps to type again, encrypted (src/vault). Passwords
// and addresses have their own fields; anything else is "other".
export interface VaultField { name: string; value: string; secret: boolean }
export interface VaultItem {
  id: string;
  kind: 'password' | 'address' | 'other';
  title: string;
  fields: VaultField[];
  created: number;
  updated: number;
}
export interface VaultStatus { exists: boolean; password: boolean; unlocked: boolean }

// Parts of the user's data that export and import handle one by one.
export const backupParts = ['settings', 'lexicon', 'learning', 'phrases', 'panel', 'stickers', 'clipboard', 'records'] as const;
export type BackupPart = (typeof backupParts)[number];
export type BackupMode = 'merge' | 'overwrite';
export interface BackupSummary {
  file: string;
  created: string;
  version: string;
  // Present parts with a short description of what each holds.
  parts: Partial<Record<BackupPart, { files: number; bytes: number; items: number }>>;
}

export interface UserEntry {
  pinyin: string;
  word: string;
  frequency: number;
}

export interface DictionaryEntry extends UserEntry {
  source: string;
}

export interface AppState {
  version: string;
  settings: Settings;
  dictionary: { total: number; core: number; local: number; standard: number };
  learning: { count: number; capacity: number };
  runtime: { active: boolean; mode: string; available: boolean };
  dataDirectory: string;
  userLexiconCount: number;
}

// The settings app's view of updates (electron/updates.ts).
export interface UpdateStatus {
  state: 'idle' | 'checking' | 'current' | 'available' | 'downloading' | 'ready' | 'error';
  current: string;
  autoCheck: boolean;
  checkedAt?: string;
  latest?: { version: string; notes: string; published: string; size: number; downloadable: boolean };
  // 0..1 while downloading.
  progress?: number;
  error?: string;
}
export interface LocalPinyinApi {
  readonly initialState: AppState | null;
  getState(): Promise<AppState>;
  getUpdateStatus(): Promise<UpdateStatus>;
  checkForUpdate(): Promise<UpdateStatus>;
  downloadUpdate(): Promise<UpdateStatus>;
  installUpdate(): Promise<{ started: boolean }>;
  setAutoUpdateCheck(enabled: boolean): Promise<UpdateStatus>;
  openReleasePage(): Promise<void>;
  // Returns a function that stops listening.
  onUpdateStatus(listener: (status: UpdateStatus) => void): () => void;
  saveSettings(settings: Settings): Promise<AppState>;
  listUserLexicon(): Promise<{ entries: UserEntry[] }>;
  upsertUserEntry(entry: UserEntry, previous?: UserEntry): Promise<{ entries: UserEntry[] }>;
  deleteUserEntry(entry: UserEntry): Promise<{ entries: UserEntry[] }>;
  searchDictionary(query: string, limit?: number): Promise<{ entries: DictionaryEntry[]; total: number }>;
  clearLearning(): Promise<{ cleared: boolean }>;
  // The words of every usage-scene pack.
  domainWords(): Promise<{ domains: Record<UsageDomain, string[]> }>;
  listPhrases(): Promise<{ phrases: Phrase[] }>;
  addPhrase(text: string, label: string): Promise<{ phrases: Phrase[] }>;
  deletePhrase(pinyin: string, text: string): Promise<{ phrases: Phrase[] }>;
  listEmojiCandidates(): Promise<{ phrases: Phrase[] }>;
  deleteEmojiCandidate(pinyin: string, text: string): Promise<{ phrases: Phrase[] }>;
  clipboardSummary(): Promise<{ items: number; images: number; bytes: number }>;
  clearClipboard(): Promise<{ items: number; images: number; bytes: number }>;
  chooseBackupFile(mode: 'save' | 'open', name?: string): Promise<string | null>;
  exportBackup(parts: BackupPart[], file: string): Promise<BackupSummary>;
  inspectBackup(file: string): Promise<BackupSummary>;
  importBackup(file: string, choices: Partial<Record<BackupPart, BackupMode>>): Promise<{ imported: BackupPart[] }>;
  vaultStatus(): Promise<VaultStatus>;
  vaultCreate(password: string): Promise<VaultStatus>;
  // An empty new password removes it; setting one on a vault without passes ''.
  vaultChangePassword(current: string, next: string): Promise<VaultStatus>;
  vaultDestroy(): Promise<VaultStatus>;
  // The panel changed a setting (the clipboard question): read them again.
  onSettingsChanged(listener: () => void): () => void;
  openDataDirectory(): Promise<void>;
  windowControl(action: 'minimize' | 'maximize' | 'close'): Promise<void>;
}

declare global {
  interface Window {
    localPinyin: LocalPinyinApi;
  }
}
