// 记录 in the settings app: the settings bridge opens and writes the
// encrypted vault (src/vault); secrets go to it on stdin. After unlocking,
// the password stays in this process for five minutes from the last use, so
// adding or editing a few items does not ask again.
import { execFile } from 'node:child_process';
import type { VaultItem, VaultStatus } from '../src/types';

type Run = <T>(command: string, args: string[], input?: unknown) => Promise<T>;

const UNLOCK_MS = 5 * 60 * 1000;
const CLEAR_SECRET_MS = 45 * 1000;
const KINDS = ['password', 'address', 'other'];

// The bridge with a JSON request on stdin.
export function bridgeWithInput(bridgePath: string): Run {
  return <T>(command: string, args: string[], input?: unknown) => new Promise<T>((resolve, reject) => {
    const child = execFile(bridgePath, [command, ...args], { windowsHide: true, encoding: 'utf8', timeout: 30_000,
      maxBuffer: 16 * 1024 * 1024 }, (error, stdout) => {
      let result: T & { error?: string };
      try { result = JSON.parse(stdout); } catch { reject(new Error('本地服务暂时不可用，请使用完整安装包启动设置。')); return; }
      if (error || result.error) reject(new Error(result.error || '本地操作失败，请重试。'));
      else resolve(result);
    });
    child.stdin?.end(input === undefined ? '' : JSON.stringify(input), 'utf8');
  });
}

function text(value: unknown, maximum: number, empty = false): string {
  if (typeof value !== 'string' || value.length > maximum || (!empty && !value)) throw new Error('输入内容无效。');
  return value;
}

export function checkedItem(value: unknown): VaultItem {
  if (!value || typeof value !== 'object') throw new Error('记录内容无效。');
  const item = value as Partial<VaultItem>;
  const kind = KINDS.includes(String(item.kind)) ? item.kind as VaultItem['kind'] : 'other';
  if (!Array.isArray(item.fields) || item.fields.length > 32) throw new Error('一条记录最多 32 项。');
  return {
    id: typeof item.id === 'string' && /^[0-9a-f]{0,16}$/.test(item.id) ? item.id : '',
    kind,
    title: text(item.title?.trim(), 100),
    fields: item.fields.map(field => ({ name: text(field?.name, 32), value: text(field?.value ?? '', 4000, true), secret: field?.secret === true })),
    created: 0,
    updated: 0,
  };
}

export class VaultSession {
  private password: string | null = null;
  private timer: NodeJS.Timeout | null = null;
  private clearTimer: NodeJS.Timeout | null = null;

  constructor(private readonly run: Run) {}

  get unlocked() { return this.password !== null; }

  private touch() {
    if (this.timer) clearTimeout(this.timer);
    this.timer = setTimeout(() => this.lock(), UNLOCK_MS);
  }

  lock() {
    this.password = null;
    if (this.timer) { clearTimeout(this.timer); this.timer = null; }
  }

  async status(): Promise<VaultStatus> {
    const status = await this.run<{ exists: boolean; password: boolean }>('vault', ['status']);
    return { ...status, unlocked: this.unlocked };
  }

  async create(password: unknown): Promise<VaultItem[]> {
    const value = text(password, 128, true);
    await this.run('vault', ['create'], { password: value });
    return this.unlock(value);
  }

  async unlock(password: unknown): Promise<VaultItem[]> {
    const value = text(password, 128, true);
    const result = await this.run<{ items: VaultItem[] }>('vault', ['open'], { password: value });
    this.password = value;
    this.touch();
    return result.items;
  }

  private session(): string {
    if (this.password === null) throw new Error('记录已锁定，请重新输入密码。');
    this.touch();
    return this.password;
  }

  async items(): Promise<VaultItem[]> {
    return (await this.run<{ items: VaultItem[] }>('vault', ['open'], { password: this.session() })).items;
  }

  async save(item: unknown): Promise<VaultItem[]> {
    return (await this.run<{ items: VaultItem[] }>('vault', ['save'], { password: this.session(), item: checkedItem(item) })).items;
  }

  async remove(id: unknown): Promise<VaultItem[]> {
    const key = text(id, 16);
    return (await this.run<{ items: VaultItem[] }>('vault', ['delete'], { password: this.session(), id: key })).items;
  }

  // Settings: set, change or remove the password (an empty new one removes it).
  async changePassword(current: unknown, next: unknown): Promise<VaultStatus> {
    const old = text(current, 128, true), value = text(next, 128, true);
    await this.run('vault', ['password'], { password: old, newPassword: value });
    if (this.password !== null) this.password = value;
    return this.status();
  }

  async destroy(): Promise<VaultStatus> {
    this.lock();
    await this.run('vault', ['destroy']);
    return this.status();
  }

  // Copied for pasting elsewhere, out of clipboard histories, gone after a while.
  async copy(value: unknown): Promise<void> {
    const content = text(value, 4000);
    this.session();
    const { sequence } = await this.run<{ sequence: number }>('copy-secret', [], { text: content });
    if (this.clearTimer) clearTimeout(this.clearTimer);
    this.clearTimer = setTimeout(() => {
      this.clearTimer = null;
      void this.run('clear-secret', [String(sequence)]).catch(() => undefined);
    }, CLEAR_SECRET_MS);
  }
}
