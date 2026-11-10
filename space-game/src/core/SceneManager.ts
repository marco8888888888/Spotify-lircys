import type { GameScene, SceneContext } from './Scene';
import type { EventBus } from './EventBus';
import type { SceneId } from './types';

/** Fabrik darf async sein (z. B. für Code-Splitting per dynamic import). */
export type SceneFactory = () => GameScene | Promise<GameScene>;

/**
 * Verwaltet registrierte Szenen, wechselt sie und leitet Loop-Aufrufe an die aktive Szene weiter.
 * Nach jedem Wechsel wird `scene:change` gefeuert.
 */
export class SceneManager {
  private factories = new Map<SceneId, SceneFactory>();
  private _current: GameScene | null = null;
  private switching: Promise<void> = Promise.resolve();

  constructor(
    private readonly bus: EventBus,
    private readonly makeContext: (mgr: SceneManager) => Omit<SceneContext, 'goTo'>,
  ) {}

  register(id: SceneId, factory: SceneFactory): this {
    this.factories.set(id, factory);
    return this;
  }

  get current(): GameScene | null {
    return this._current;
  }

  /** Wechsel serialisieren, damit sich parallele Aufrufe nicht überholen. */
  goTo(id: SceneId): Promise<void> {
    this.switching = this.switching.then(() => this.doSwitch(id));
    return this.switching;
  }

  private async doSwitch(id: SceneId): Promise<void> {
    const factory = this.factories.get(id);
    if (!factory) throw new Error(`Unbekannte Szene: ${id}`);
    const from = this._current?.id ?? null;
    if (this._current) {
      const old = this._current;
      this._current = null;
      old.exit();
    }
    const base = this.makeContext(this);
    base.ui.replaceChildren();
    const scene = await factory();
    await scene.enter({ ...base, goTo: (to) => this.goTo(to) });
    this._current = scene;
    this.bus.emit('scene:change', { from, to: id });
  }

  fixedUpdate(dt: number): void {
    this._current?.fixedUpdate(dt);
  }

  render(alpha: number, frameDt: number): void {
    this._current?.render(alpha, frameDt);
  }

  resize(w: number, h: number): void {
    this._current?.resize?.(w, h);
  }
}
