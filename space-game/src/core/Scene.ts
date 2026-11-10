import type * as THREE from 'three';
import type { EventBus } from './EventBus';
import type { SceneId } from './types';

/** Gemeinsame Ressourcen, die jede Szene beim Betreten bekommt. */
export interface SceneContext {
  renderer: THREE.WebGLRenderer;
  /** DOM-Container für das Szenen-UI (wird bei Szenenwechsel geleert). */
  ui: HTMLElement;
  bus: EventBus;
  /** Szenenwechsel anfordern. */
  goTo(id: SceneId): Promise<void>;
}

export interface GameScene {
  readonly id: SceneId;
  enter(ctx: SceneContext): void | Promise<void>;
  exit(): void;
  /** Fixer Physik-Tick (50 Hz). */
  fixedUpdate(dt: number): void;
  /** Rendern mit Interpolationsfaktor alpha. */
  render(alpha: number, frameDt: number): void;
  resize?(width: number, height: number): void;
}
