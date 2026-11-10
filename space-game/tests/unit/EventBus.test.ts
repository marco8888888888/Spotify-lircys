import { describe, it, expect, vi } from 'vitest';
import { EventBus } from '../../src/core/EventBus';

describe('EventBus', () => {
  it('liefert typisierte Payloads an Listener', () => {
    const bus = new EventBus();
    const fn = vi.fn();
    bus.on('stage:activated', fn);
    bus.emit('stage:activated', { vesselId: 'v1', stage: 2 });
    expect(fn).toHaveBeenCalledWith({ vesselId: 'v1', stage: 2 });
  });

  it('unsubscribe per Rückgabewert und off', () => {
    const bus = new EventBus();
    const a = vi.fn();
    const b = vi.fn();
    const offA = bus.on('timewarp:changed', a);
    bus.on('timewarp:changed', b);
    offA();
    bus.off('timewarp:changed', b);
    bus.emit('timewarp:changed', { rate: 4, previous: 1 });
    expect(a).not.toHaveBeenCalled();
    expect(b).not.toHaveBeenCalled();
    expect(bus.listenerCount('timewarp:changed')).toBe(0);
  });

  it('once feuert genau einmal', () => {
    const bus = new EventBus();
    const fn = vi.fn();
    bus.once('soi:changed', fn);
    bus.emit('soi:changed', { vesselId: 'v', fromBodyId: 'terran-vey', toBodyId: 'pell' });
    bus.emit('soi:changed', { vesselId: 'v', fromBodyId: 'pell', toBodyId: 'terran-vey' });
    expect(fn).toHaveBeenCalledTimes(1);
  });

  it('ein werfender Listener blockiert andere nicht', () => {
    const bus = new EventBus();
    const err = vi.spyOn(console, 'error').mockImplementation(() => {});
    const ok = vi.fn();
    bus.on('part:destroyed', () => {
      throw new Error('boom');
    });
    bus.on('part:destroyed', ok);
    bus.emit('part:destroyed', { vesselId: 'v', partUid: 'p1' });
    expect(ok).toHaveBeenCalled();
    err.mockRestore();
  });

  it('Listener-Entfernung während emit ist sicher', () => {
    const bus = new EventBus();
    const second = vi.fn();
    const off2 = bus.on('scene:change', second);
    bus.on('scene:change', () => off2());
    bus.emit('scene:change', { from: null, to: 'menu' });
    expect(second).toHaveBeenCalledTimes(1);
  });
});
