import { describe, it, expect } from 'vitest';
import { GameLoop, FIXED_DT, PHYSICS_HZ } from '../../src/core/GameLoop';

function makeLoop(maxStepsPerFrame?: number) {
  const ticks: number[] = [];
  const renders: number[] = [];
  const loop = new GameLoop(
    { fixedUpdate: (dt) => ticks.push(dt), render: (a) => renders.push(a) },
    { now: () => 0, schedule: () => 1, cancel: () => {}, ...(maxStepsPerFrame ? { maxStepsPerFrame } : {}) },
  );
  return { loop, ticks, renders };
}

describe('GameLoop', () => {
  it('läuft mit 50 Hz fixem Tick', () => {
    expect(PHYSICS_HZ).toBe(50);
    expect(FIXED_DT).toBeCloseTo(0.02);
  });

  it('1 s echte Zeit in 60 Frames → 50 Physik-Ticks, 60 Renders', () => {
    const { loop, ticks, renders } = makeLoop();
    for (let i = 0; i < 60; i++) loop.advance(1 / 60);
    expect(ticks.length).toBe(50);
    expect(renders.length).toBe(60);
    expect(ticks.every((dt) => dt === FIXED_DT)).toBe(true);
  });

  it('alpha ist der Akkumulator-Rest in [0,1)', () => {
    const { loop, ticks, renders } = makeLoop();
    loop.advance(0.03); // 1 Tick, Rest 0.01 → alpha 0.5
    expect(ticks.length).toBe(1);
    expect(renders[0]).toBeCloseTo(0.5);
    for (const a of renders) expect(a).toBeGreaterThanOrEqual(0);
  });

  it('kleine Frames akkumulieren', () => {
    const { loop, ticks } = makeLoop();
    loop.advance(0.012);
    expect(ticks.length).toBe(0);
    loop.advance(0.012);
    expect(ticks.length).toBe(1);
  });

  it('timeScale erhöht die Anzahl, nicht die Größe der Schritte', () => {
    const { loop, ticks } = makeLoop();
    loop.timeScale = 4;
    loop.advance(0.1);
    expect(ticks.length).toBe(10); // 0.4 s Spielzeit begrenzt durch maxSteps=10
    expect(ticks.every((dt) => dt === FIXED_DT)).toBe(true);
    expect(loop.simTime).toBeCloseTo(0.2);
  });

  it('lange Pausen werden gekappt (keine Spiral of Death)', () => {
    const { loop, ticks, renders } = makeLoop(5);
    loop.advance(10);
    expect(ticks.length).toBe(5);
    expect(renders.at(-1)!).toBeLessThan(1);
  });

  it('ungültiger timeScale wirft', () => {
    const { loop } = makeLoop();
    expect(() => (loop.timeScale = 0)).toThrow(RangeError);
    expect(() => (loop.timeScale = Number.NaN)).toThrow(RangeError);
  });

  it('start/stop über Scheduler', () => {
    let t = 0;
    let pending: (() => void) | null = null;
    let ticks = 0;
    const loop = new GameLoop(
      { fixedUpdate: () => ticks++, render: () => {} },
      { now: () => t, schedule: (cb) => ((pending = cb), 1), cancel: () => (pending = null) },
    );
    loop.start();
    expect(loop.running).toBe(true);
    t = 100;
    pending!();
    expect(ticks).toBe(5);
    loop.stop();
    expect(loop.running).toBe(false);
  });
});
