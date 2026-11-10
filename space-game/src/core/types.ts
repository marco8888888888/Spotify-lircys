// ORBITWERK – Schnittstellen-Verträge (siehe PROMPT.md, Abschnitt 4).
// Nicht ohne Hauptgehirn ändern.

export type Vec3d = { x: number; y: number; z: number }; // double precision, Meter

export interface CelestialBody {
  id: string;
  name: string;
  mu: number; // Gravitationsparameter m^3/s^2
  radius: number; // m
  soiRadius: number; // m (Infinity für den Stern)
  parentId: string | null;
  orbit: OrbitalElements | null;
  atmosphere?: { height: number; seaLevelPressure: number; scaleHeight: number; color: string };
  rotationPeriod: number; // s
}

export interface OrbitalElements {
  a: number;
  e: number;
  i: number;
  lan: number;
  argPe: number;
  meanAnomalyAtEpoch: number;
  epoch: number;
}

/**
 * Teile-Modul, z. B. { type: 'engine', thrustVac, ispVac, ispSL, propellants }.
 * In Abschnitt 4 referenziert, aber nicht ausdefiniert → bewusst offen gehalten
 * (Diskriminator `type` + beliebige Parameter). Konkretisierung in T12.
 */
export interface PartModuleDef {
  type: string;
  [param: string]: unknown;
}

export interface PartDef {
  id: string;
  name: string;
  category: 'engine' | 'tank' | 'command' | 'structural' | 'decoupler' | 'aero' | 'utility' | 'science';
  mass: number; // t (trocken)
  size: '0.625' | '1.25' | '2.5' | '3.75' | '5';
  nodes: { id: string; pos: [number, number, number]; dir: [number, number, number]; size: string }[];
  modules: PartModuleDef[]; // z. B. { type:'engine', thrustVac, ispVac, ispSL, propellants }
  mesh: string; // Generator-ID oder Asset-Pfad
}

export interface VesselPartInstance {
  uid: string;
  defId: string;
  parentUid: string | null;
  attachNode: string;
  transform: number[];
  stage: number;
  symmetryGroup?: string;
}
export interface VesselDesign {
  name: string;
  parts: VesselPartInstance[];
}

// ---------------------------------------------------------------------------
// Szenen & Events (EventBus)
// ---------------------------------------------------------------------------

export type SceneId = 'menu' | 'editor' | 'flight';

/** Event-Namen → Payload. Events laut Abschnitt 4. */
export interface GameEvents {
  'stage:activated': { vesselId: string; stage: number };
  'part:destroyed': { vesselId: string; partUid: string; cause?: string };
  'soi:changed': { vesselId: string; fromBodyId: string; toBodyId: string };
  'timewarp:changed': { rate: number; previous: number };
  'scene:change': { from: SceneId | null; to: SceneId };
}

export type GameEventName = keyof GameEvents;
