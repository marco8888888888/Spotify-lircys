import RAPIER from '@dimforge/rapier3d-compat';
import type { Vec3d } from '../core/types';
import { FIXED_DT } from '../core/GameLoop';

let initPromise: Promise<typeof RAPIER> | null = null;

/** Rapier-WASM einmalig initialisieren. */
export function initRapier(): Promise<typeof RAPIER> {
  initPromise ??= RAPIER.init().then(() => RAPIER);
  return initPromise;
}

export type BodyHandle = RAPIER.RigidBody;

/**
 * Dünner Wrapper um eine Rapier-Welt mit festem Zeitschritt (FIXED_DT).
 * Hält pro Körper den vorherigen Zustand, damit das Rendering interpolieren kann.
 */
export class PhysicsWorld {
  readonly world: RAPIER.World;
  private prev = new Map<number, Vec3d>();

  private constructor(readonly rapier: typeof RAPIER, gravity: Vec3d) {
    this.world = new rapier.World(gravity);
    this.world.timestep = FIXED_DT;
  }

  static async create(gravity: Vec3d = { x: 0, y: -9.81, z: 0 }): Promise<PhysicsWorld> {
    const r = await initRapier();
    return new PhysicsWorld(r, gravity);
  }

  addStaticBox(pos: Vec3d, halfExtents: Vec3d): BodyHandle {
    const body = this.world.createRigidBody(this.rapier.RigidBodyDesc.fixed().setTranslation(pos.x, pos.y, pos.z));
    this.world.createCollider(this.rapier.ColliderDesc.cuboid(halfExtents.x, halfExtents.y, halfExtents.z), body);
    return body;
  }

  addDynamicSphere(pos: Vec3d, radius: number, restitution = 0.3): BodyHandle {
    const body = this.world.createRigidBody(this.rapier.RigidBodyDesc.dynamic().setTranslation(pos.x, pos.y, pos.z));
    this.world.createCollider(this.rapier.ColliderDesc.ball(radius).setRestitution(restitution), body);
    this.prev.set(body.handle, { ...pos });
    return body;
  }

  /** Einen fixen Schritt simulieren; merkt sich vorher die Positionen dynamischer Körper. */
  step(): void {
    this.world.forEachRigidBody((b) => {
      if (b.isDynamic()) {
        const t = b.translation();
        this.prev.set(b.handle, { x: t.x, y: t.y, z: t.z });
      }
    });
    this.world.step();
  }

  /** Interpolierte Position zwischen vorletztem und letztem Schritt. */
  interpolated(body: BodyHandle, alpha: number): Vec3d {
    const cur = body.translation();
    const p = this.prev.get(body.handle) ?? cur;
    return {
      x: p.x + (cur.x - p.x) * alpha,
      y: p.y + (cur.y - p.y) * alpha,
      z: p.z + (cur.z - p.z) * alpha,
    };
  }

  dispose(): void {
    this.prev.clear();
    this.world.free();
  }
}
