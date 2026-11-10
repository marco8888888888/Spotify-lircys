import { describe, it, expect } from 'vitest';
import { EventBus } from '../../src/core/EventBus';
import { SceneManager } from '../../src/core/SceneManager';
import type { GameScene } from '../../src/core/Scene';
import type { SceneId } from '../../src/core/types';

function fakeScene(id: SceneId, log: string[]): GameScene {
  return {
    id,
    enter: async () => {
      await Promise.resolve();
      log.push(`enter:${id}`);
    },
    exit: () => log.push(`exit:${id}`),
    fixedUpdate: () => log.push(`tick:${id}`),
    render: () => log.push(`render:${id}`),
  };
}

function setup() {
  const bus = new EventBus();
  const log: string[] = [];
  let cleared = 0;
  const ui = { replaceChildren: () => cleared++ } as unknown as HTMLElement;
  const mgr = new SceneManager(bus, () => ({ renderer: null as never, ui, bus }))
    .register('menu', () => fakeScene('menu', log))
    .register('editor', () => fakeScene('editor', log))
    .register('flight', async () => fakeScene('flight', log));
  return { bus, log, mgr, cleared: () => cleared };
}

describe('SceneManager', () => {
  it('wechselt Szenen, ruft exit/enter und feuert scene:change', async () => {
    const { bus, log, mgr, cleared } = setup();
    const changes: unknown[] = [];
    bus.on('scene:change', (p) => changes.push(p));
    await mgr.goTo('menu');
    await mgr.goTo('flight');
    expect(log).toEqual(['enter:menu', 'exit:menu', 'enter:flight']);
    expect(changes).toEqual([
      { from: null, to: 'menu' },
      { from: 'menu', to: 'flight' },
    ]);
    expect(mgr.current?.id).toBe('flight');
    expect(cleared()).toBe(2);
  });

  it('leitet Loop-Aufrufe an die aktive Szene weiter', async () => {
    const { log, mgr } = setup();
    mgr.fixedUpdate(0.02); // keine Szene → nichts
    await mgr.goTo('editor');
    mgr.fixedUpdate(0.02);
    mgr.render(0.5, 0.016);
    expect(log).toEqual(['enter:editor', 'tick:editor', 'render:editor']);
  });

  it('serialisiert parallele Wechsel', async () => {
    const { log, mgr } = setup();
    await Promise.all([mgr.goTo('menu'), mgr.goTo('editor'), mgr.goTo('flight')]);
    expect(log).toEqual(['enter:menu', 'exit:menu', 'enter:editor', 'exit:editor', 'enter:flight']);
  });

  it('unbekannte Szene wirft', async () => {
    const bus = new EventBus();
    const mgr = new SceneManager(bus, () => ({ renderer: null as never, ui: null as never, bus }));
    await expect(mgr.goTo('menu')).rejects.toThrow('Unbekannte Szene');
  });
});
