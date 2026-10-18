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
  shiftToggleEnabled: boolean;
  predictionEnabled: boolean;
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
  openDataDirectory(): Promise<void>;
  windowControl(action: 'minimize' | 'maximize' | 'close'): Promise<void>;
}

declare global {
  interface Window {
    localPinyin: LocalPinyinApi;
  }
}
