export type Theme = 'system' | 'light' | 'dark';

export interface Settings {
  theme: Theme;
  textSize: number;
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
  learning: { count: number };
  runtime: { active: boolean; mode: string; available: boolean };
  dataDirectory: string;
  userLexiconCount: number;
}

export interface LocalPinyinApi {
  readonly initialState: AppState | null;
  getState(): Promise<AppState>;
  saveSettings(settings: Settings): Promise<AppState>;
  lookup(pinyin: string, precedingText?: string): Promise<{ candidates: string[] }>;
  predict(precedingText: string): Promise<{ candidates: string[] }>;
  listUserLexicon(): Promise<{ entries: UserEntry[] }>;
  upsertUserEntry(entry: UserEntry, previous?: UserEntry): Promise<{ entries: UserEntry[] }>;
  deleteUserEntry(entry: UserEntry): Promise<{ entries: UserEntry[] }>;
  searchDictionary(query: string, limit?: number): Promise<{ entries: DictionaryEntry[]; total: number }>;
  clearLearning(): Promise<{ cleared: boolean }>;
  openDataDirectory(): Promise<void>;
  windowControl(action: 'minimize' | 'maximize' | 'close'): Promise<void>;
}

declare global {
  interface Window {
    localPinyin: LocalPinyinApi;
  }
}
