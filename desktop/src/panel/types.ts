// What the tool panel and its main-process side exchange.
import type { ClipboardHistory, Phrase } from '../types';

export type { Phrase };

export type PanelKind = 'symbol' | 'kaomoji' | 'emoji';

export type Pick = { kind: PanelKind; value: string };

export type PanelState = {
  favorites: Record<PanelKind, string[]>;
  recents: Record<PanelKind, string[]>;
};

export type Sticker = { id: string; ext: string; name: string; added: number };

export type PanelData = { state: PanelState; stickers: Sticker[]; dark: boolean };

// One copy in the clipboard history. Text longer than the preview is kept
// whole in a file of its own (`long`); pictures carry the text read from
// them (`ocr`), so searching finds words inside screenshots too.
export type ClipItem = {
  id: string;
  kind: 'text' | 'image';
  time: number;
  text?: string;
  long?: boolean;
  ocr?: string;
  w?: number;
  h?: number;
};

export type ClipQuery = { search?: string; day?: string; limit?: number };

export type ClipList = {
  status: ClipboardHistory;
  retentionDays: number;
  items: ClipItem[];
  total: number;
  // Local dates (YYYY-MM-DD) that have copies, newest first, with counts.
  days: { day: string; count: number }[];
};

export type PanelApi = {
  bootstrap: { dark: boolean } | null;
  data(): Promise<PanelData>;
  close(): Promise<void>;
  // `picks` are what the text was made of, for 最近使用.
  insert(text: string, picks?: Pick[]): Promise<void>;
  paste(stickerId: string): Promise<void>;
  favorite(kind: PanelKind, value: string, on: boolean): Promise<PanelState>;
  tools(input: string): Promise<{ results: string[] }>;
  phrases(): Promise<{ phrases: Phrase[] }>;
  addPhrase(text: string, labels: string[]): Promise<{ phrases: Phrase[] }>;
  deletePhrase(pinyin: string, text: string): Promise<{ phrases: Phrase[] }>;
  // An emoji confirmed from the panel, with its names, to offer among candidates.
  learnEmoji(emoji: string, names: string[]): Promise<void>;
  addStickers(files: { name: string; data: Uint8Array }[]): Promise<Sticker[]>;
  deleteSticker(id: string): Promise<Sticker[]>;
  choosingFile(on: boolean): Promise<void>;
  keepOpen(on: boolean): Promise<void>;
  openSettings(): Promise<void>;
  clipboard(query: ClipQuery): Promise<ClipList>;
  clipboardText(id: string): Promise<string>;
  clipboardPaste(id: string): Promise<void>;
  clipboardDelete(id: string): Promise<void>;
  clipboardConsent(on: boolean): Promise<void>;
  onOpen(listener: () => void): () => void;
};

declare global {
  interface Window { localPinyinPanel: PanelApi }
}
