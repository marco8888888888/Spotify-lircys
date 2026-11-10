/** Fixer Physik-Tick: 50 Hz. */
export const PHYSICS_HZ = 50;
export const FIXED_DT = 1 / PHYSICS_HZ;
/** Toleranz gegen Rundungsfehler im Akkumulator (z. B. 0.1 / 0.02 = 4.999…). */
const EPS = 1e-9;

export interface LoopCallbacks {
  /** Ein fixer Simulationsschritt, dt immer FIXED_DT (Sekunden Spielzeit). */
  fixedUpdate(dt: number): void;
  /**
   * Render-Aufruf pro Frame.
   * alpha ∈ [0,1): Interpolationsfaktor zwischen vorletztem und letztem Physikzustand.
   * frameDt: echte vergangene Zeit seit letztem Frame (s).
   */
  render(alpha: number, frameDt: number): void;
}

export interface LoopOptions {
  /** Zeitquelle in Millisekunden. */
  now?: () => number;
  /** Frame-Scheduler (Standard: requestAnimationFrame). */
  schedule?: (cb: () => void) => number;
  cancel?: (handle: number) => void;
  /** Max. Physik-Schritte pro Frame (Schutz gegen "Spiral of Death"). */
  maxStepsPerFrame?: number;
}

/**
 * Game-Loop mit Akkumulator: Physik läuft in festen 20-ms-Schritten,
 * Rendering so oft wie der Browser Frames liefert und interpoliert mit alpha.
 * timeScale (physikalischer Zeitraffer) erhöht die Zahl fixer Schritte, nie deren Größe.
 */
export class GameLoop {
  private accumulator = 0;
  private last = 0;
  private handle: number | null = null;
  private _timeScale = 1;
  private _simTime = 0;
  private _ticks = 0;
  private readonly now: () => number;
  private readonly schedule: (cb: () => void) => number;
  private readonly cancel: (h: number) => void;
  private readonly maxSteps: number;

  constructor(
    private readonly cb: LoopCallbacks,
    opts: LoopOptions = {},
  ) {
    this.now = opts.now ?? (() => performance.now());
    this.schedule = opts.schedule ?? ((f) => requestAnimationFrame(f));
    this.cancel = opts.cancel ?? ((h) => cancelAnimationFrame(h));
    this.maxSteps = opts.maxStepsPerFrame ?? 10;
  }

  get running(): boolean {
    return this.handle !== null;
  }
  get simTime(): number {
    return this._simTime;
  }
  get ticks(): number {
    return this._ticks;
  }
  get timeScale(): number {
    return this._timeScale;
  }
  set timeScale(v: number) {
    if (!(v > 0) || !Number.isFinite(v)) throw new RangeError(`timeScale muss > 0 sein, war ${v}`);
    this._timeScale = v;
  }

  start(): void {
    if (this.handle !== null) return;
    this.last = this.now();
    this.accumulator = 0;
    this.handle = this.schedule(this.frame);
  }

  stop(): void {
    if (this.handle !== null) this.cancel(this.handle);
    this.handle = null;
  }

  /**
   * Einen Frame mit gegebener echter Zeitdifferenz (s) verarbeiten.
   * Öffentlich für Tests und deterministische Steuerung.
   */
  advance(realDt: number): void {
    // Lange Pausen (Tab im Hintergrund) nicht nachholen.
    const clamped = Math.min(Math.max(realDt, 0), 0.25);
    this.accumulator += clamped * this._timeScale;
    let steps = 0;
    while (this.accumulator >= FIXED_DT - EPS && steps < this.maxSteps) {
      this.cb.fixedUpdate(FIXED_DT);
      this.accumulator = Math.max(0, this.accumulator - FIXED_DT);
      this._simTime += FIXED_DT;
      this._ticks++;
      steps++;
    }
    // Rückstand verwerfen, wenn das Budget erschöpft ist.
    if (steps >= this.maxSteps && this.accumulator >= FIXED_DT) this.accumulator %= FIXED_DT;
    this.cb.render(this.accumulator / FIXED_DT, clamped);
  }

  private frame = (): void => {
    if (this.handle === null) return;
    const t = this.now();
    const dt = (t - this.last) / 1000;
    this.last = t;
    this.advance(dt);
    if (this.handle !== null) this.handle = this.schedule(this.frame);
  };
}
