import { describe, it, expect } from 'vitest';
import { PhysicsWorld } from '../../src/physics/PhysicsWorld';
import { FIXED_DT } from '../../src/core/GameLoop';

describe('PhysicsWorld (Rapier)', () => {
  it('nutzt den fixen 50-Hz-Schritt', async () => {
    const pw = await PhysicsWorld.create();
    expect(pw.world.timestep).toBeCloseTo(FIXED_DT);
    pw.dispose();
  });

  it('Kugel fällt frei gemäß g und landet auf dem Boden', async () => {
    const pw = await PhysicsWorld.create();
    pw.addStaticBox({ x: 0, y: -0.5, z: 0 }, { x: 10, y: 0.5, z: 10 });
    const ball = pw.addDynamicSphere({ x: 0, y: 10, z: 0 }, 1, 0);
    for (let i = 0; i < 25; i++) pw.step(); // 0.5 s
    const y = ball.translation().y;
    expect(10 - y).toBeGreaterThan(0.5 * 9.81 * 0.25 * 0.9);
    expect(10 - y).toBeLessThan(0.5 * 9.81 * 0.25 * 1.1);
    for (let i = 0; i < 250; i++) pw.step(); // 5 s mehr
    expect(ball.translation().y).toBeGreaterThan(0.9);
    expect(ball.translation().y).toBeLessThan(1.1);
    pw.dispose();
  });

  it('interpoliert zwischen vorherigem und aktuellem Zustand', async () => {
    const pw = await PhysicsWorld.create();
    const ball = pw.addDynamicSphere({ x: 0, y: 10, z: 0 }, 1);
    pw.step();
    pw.step();
    const cur = ball.translation().y;
    const p0 = pw.interpolated(ball, 0).y;
    const p1 = pw.interpolated(ball, 1).y;
    const mid = pw.interpolated(ball, 0.5).y;
    expect(p1).toBeCloseTo(cur);
    expect(p0).toBeGreaterThan(cur);
    expect(mid).toBeCloseTo((p0 + p1) / 2);
    pw.dispose();
  });
});
