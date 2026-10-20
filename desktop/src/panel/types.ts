// What the tool panel and its main-process side exchange.
import type { ClipboardHistory, Phrase, VaultField, VaultItem, VaultStatus } from '../types';

export type { Phrase, VaultField, VaultItem, VaultStatus };

// The pages, as the input method's small panel names them (--page=).
export const panelPages = ['tools', 'clipboard', 'symbol', 'kaomoji', 'emoji', 'stickers', 'records'] as const;
export type PanelPageId = (typeof panelPages)[number];

export type PanelKind = 'symbol' | 'kaomoji' | 'emoji';

export type Pick = { kind: PanelKind; value: string };

export type PanelState = {
  favorites: Record<PanelKind, string[]>;
  recents: Record<PanelKind, string[]>;
};

export type Sticker = { id: string; ext: string; name: string; added: number };

// `page`: where the small panel was when it opened this one.
export type PanelData = { state: PanelState; stickers: Sticker[]; dark: boolean; page: PanelPageId | null };

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
  // Pictures dropped as file:// links, or copied (Ctrl+V).
  addStickerLinks(links: string[]): Promise<Sticker[]>;
  addStickerFromClipboard(): Promise<Sticker[]>;
  keepOpen(on: boolean): Promise<void>;
  openSettings(): Promise<void>;
  clipboard(query: ClipQuery): Promise<ClipList>;
  clipboardText(id: string): Promise<string>;
  clipboardPaste(id: string): Promise<void>;
  clipboardDelete(id: string): Promise<void>;
  clipboardConsent(on: boolean): Promise<void>;
  vaultStatus(): Promise<VaultStatus>;
  vaultCreate(password: string): Promise<VaultItem[]>;
  vaultUnlock(password: string): Promise<VaultItem[]>;
  vaultItems(): Promise<VaultItem[]>;
  vaultSave(item: VaultItem): Promise<VaultItem[]>;
  vaultDelete(id: string): Promise<VaultItem[]>;
  vaultLock(): Promise<void>;
  vaultCopy(text: string): Promise<void>;
  onOpen(listener: () => void): () => void;
};

declare global {
  interface Window { localPinyinPanel: PanelApi }
}
