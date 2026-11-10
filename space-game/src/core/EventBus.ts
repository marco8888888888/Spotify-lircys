import type { GameEvents } from './types';

export type Listener<P> = (payload: P) => void;

/**
 * Typisierter, synchroner EventBus.
 * `on` liefert eine Unsubscribe-Funktion. Fehler in einem Listener stoppen die anderen nicht.
 */
export class EventBus<E extends object = GameEvents> {
  private listeners = new Map<keyof E, Set<Listener<never>>>();

  on<K extends keyof E>(name: K, fn: Listener<E[K]>): () => void {
    let set = this.listeners.get(name);
    if (!set) {
      set = new Set();
      this.listeners.set(name, set);
    }
    set.add(fn as Listener<never>);
    return () => this.off(name, fn);
  }

  once<K extends keyof E>(name: K, fn: Listener<E[K]>): () => void {
    const off = this.on(name, (p) => {
      off();
      fn(p);
    });
    return off;
  }

  off<K extends keyof E>(name: K, fn: Listener<E[K]>): void {
    this.listeners.get(name)?.delete(fn as Listener<never>);
  }

  emit<K extends keyof E>(name: K, payload: E[K]): void {
    const set = this.listeners.get(name);
    if (!set) return;
    for (const fn of [...set]) {
      try {
        (fn as Listener<E[K]>)(payload);
      } catch (err) {
        console.error(`[EventBus] Listener für "${String(name)}" warf:`, err);
      }
    }
  }

  listenerCount(name: keyof E): number {
    return this.listeners.get(name)?.size ?? 0;
  }

  clear(): void {
    this.listeners.clear();
  }
}
