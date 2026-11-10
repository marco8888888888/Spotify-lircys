# ORBITWERK – Master-Prompt (Hauptgehirn)

> Arbeitstitel: **Orbitwerk** – ein eigenes Raketenbau- und Raumfahrt-Simulationsspiel.
> Inspiriert vom Genre (Raketen aus Teilen bauen, starten, Orbits fliegen, landen),
> aber **komplett eigenständig**: eigene Welt, eigene Crew, eigene Teile, eigener Look.
> Keine Namen, Figuren, Planeten, Grafiken, Sounds oder UI-Elemente aus bestehenden Spielen übernehmen.

---

## 1. Rollen

### 1.1 Hauptgehirn (Orchestrator)
Das Hauptgehirn ist die Session, die diese Datei pflegt. Es
- hält die **Vision** und die **Architektur** (diese Datei ist die Wahrheit),
- zerlegt Arbeit in **Tasks** (Abschnitt 6) mit klaren Schnittstellen,
- startet für jeden Task eine **Worker-Session** (eigener Chat) mit dem Worker-Prompt (Abschnitt 5),
- prüft die Ergebnisse (Build, Tests, Screenshot), merged und aktualisiert den Task-Status,
- schreibt selbst **keinen großen Feature-Code**, nur Integration, Fixes und Schnittstellen.

### 1.2 Worker
Ein Worker bekommt genau **einen** Task, arbeitet auf **einem eigenen Branch**, hält sich an
die Schnittstellen in Abschnitt 4 und liefert: Code + Tests + kurzen Bericht.

---

## 2. Vision – was "besser" heißt

| Bereich | Ziel |
|---|---|
| **Grafik** | PBR-Materialien, atmosphärische Streuung (Himmel färbt sich mit Höhe), volumetrische Triebwerksflammen, Wolkenschicht, Sternenhimmel, Bloom/Tonemapping, Reentry-Plasmaeffekt, Planeten-LOD (Quadtree-Kugel) |
| **Physik** | Stabile Starrkörper-Physik der Raketen, korrekte Orbitalmechanik (Patched Conics + Kepler-Propagation „on rails“), Zeitraffer ohne Explosionen, aerodynamischer Widerstand & Auftrieb, Hitze |
| **Bauen** | Snap-Knoten, Symmetrie (2/3/4/6/8x), Undo/Redo, Stufen-Editor mit Δv-Anzeige pro Stufe (Vakuum + Meereshöhe), TWR-Anzeige |
| **Neue Teile** | Mehr als Standard: Aerospikes, Ionenantriebe, Nuklear-thermisch, Grid-Fins, ausfahrbare Hitzeschilde, Wiederverwendbare Booster mit Rücklandung, Docking-Ports, Rover-Räder, Solarsegel, Treibstofftransfer |
| **Komfort** | Manöverknoten mit Δv-Planer, Transferfenster-Rechner, Autopilot-Basics (Prograde/Retrograde/Normal/Radial halten), Quicksave |
| **Welt** | Eigenes Sternsystem „Helion“: Heimatplanet **Terran-Vey**, Mond **Pell**, roter Wüstenplanet **Okkar**, Gasriese **Thalassa** mit Monden. Eigene Crew-Spezies: **die Nauten** (eigenes Design, keine grünen Männchen) |

**Leitprinzip:** Erst ein spielbarer Kern (bauen → starten → Orbit), dann schöner, dann mehr.

---

## 3. Technik-Stack (fest)

- **Sprache:** TypeScript (strict)
- **Rendering:** Three.js (WebGL2, später optional WebGPU-Renderer)
- **Physik (Starrkörper):** Rapier (`@dimforge/rapier3d-compat`)
- **Orbitalmechanik:** eigene Implementierung in `src/orbit/` (Double-Precision, kein Rapier)
- **Build:** Vite · **Tests:** Vitest · **E2E/Screenshot:** Playwright
- **UI:** Plain DOM + CSS (kein React im Spiel-HUD), kleine eigene Komponenten
- **Läuft im Browser** → jede Session kann per Playwright starten und Screenshots machen.

### Ordnerstruktur
```
space-game/
  PROMPT.md            ← diese Datei (Wahrheit)
  package.json
  index.html
  src/
    main.ts            ← Bootstrapping, Szenenwechsel
    core/              ← Game-Loop, EventBus, Zeit/Zeitraffer, Input, Save/Load
    orbit/             ← Kepler, Patched Conics, SOI, Manöverknoten (reine Mathe, 100% testbar)
    physics/           ← Rapier-Wrapper, Vessel-Physik, Aero, Hitze
    world/             ← Sternsystem-Daten, Planeten-Mesh/LOD, Atmosphäre, Himmel
    parts/             ← Teile-Definitionen (JSON) + Teile-Module (Engine, Tank, Decoupler …)
    vessel/            ← Vessel-Graph (Teilebaum), Staging, Δv-Rechner
    editor/            ← Baueditor (Hangar)
    flight/            ← Flugszene, Kamera, Map-View
    render/            ← Post-Processing, Shader, Effekte (Flammen, Reentry)
    ui/                ← HUD, Navball, Menüs
  assets/              ← eigene, prozedural erzeugte oder selbst erstellte Assets
  tests/               ← Vitest + Playwright
```

---

## 4. Schnittstellen-Verträge (nicht ohne Hauptgehirn ändern)

```ts
// src/core/types.ts
export type Vec3d = { x: number; y: number; z: number };         // double precision, Meter

export interface CelestialBody {
  id: string; name: string;
  mu: number;            // Gravitationsparameter m^3/s^2
  radius: number;        // m
  soiRadius: number;     // m (Infinity für den Stern)
  parentId: string | null;
  orbit: OrbitalElements | null;
  atmosphere?: { height: number; seaLevelPressure: number; scaleHeight: number; color: string };
  rotationPeriod: number; // s
}

export interface OrbitalElements {
  a: number; e: number; i: number; lan: number; argPe: number; meanAnomalyAtEpoch: number; epoch: number;
}

export interface PartDef {
  id: string; name: string; category: 'engine'|'tank'|'command'|'structural'|'decoupler'|'aero'|'utility'|'science';
  mass: number;          // t (trocken)
  size: '0.625'|'1.25'|'2.5'|'3.75'|'5';
  nodes: { id: string; pos: [number,number,number]; dir: [number,number,number]; size: string }[];
  modules: PartModuleDef[];   // z. B. { type:'engine', thrustVac, ispVac, ispSL, propellants }
  mesh: string;          // Generator-ID oder Asset-Pfad
}

export interface VesselPartInstance { uid: string; defId: string; parentUid: string | null; attachNode: string; transform: number[]; stage: number; symmetryGroup?: string }
export interface VesselDesign { name: string; parts: VesselPartInstance[] }
```

**Events (EventBus):** `stage:activated`, `part:destroyed`, `soi:changed`, `timewarp:changed`, `scene:change`.

**Floating Origin:** Rendering immer relativ zum aktiven Vessel. Weltpositionen in `Vec3d`, Three.js bekommt nur float-sichere relative Koordinaten.

---

## 5. Worker-Prompt (Vorlage für jeden neuen Chat)

```
Du bist Worker im Projekt ORBITWERK (eigenes Raketenbau-/Raumfahrtspiel, Ordner space-game/).
Lies ZUERST space-game/PROMPT.md komplett – das ist die verbindliche Architektur.

Dein Task: <TASK-ID> – <Titel>
Ziel: <Beschreibung aus Abschnitt 6>
Akzeptanzkriterien: <aus Abschnitt 6>

Regeln:
- Arbeite NUR in den Ordnern, die zu deinem Task gehören. Schnittstellen aus Abschnitt 4 nicht ändern;
  wenn nötig, schreibe den Vorschlag in deinen Bericht.
- Keine Inhalte aus bestehenden Spielen (Namen, Figuren, Grafiken, Sounds). Alles eigen.
- Schreibe Vitest-Tests für Logik. Für Visuelles: Playwright-Screenshot unter tests/screenshots/<TASK-ID>.png.
- `npm run build` und `npm test` müssen grün sein.
- Committe auf deinen Branch und pushe. Keinen PR öffnen, außer es wird verlangt.
- Bericht am Ende: Was gebaut, welche Dateien, offene Punkte, Vorschläge für Schnittstellen.
```

---

## 6. Task-Board

Status: ⬜ offen · 🟨 in Arbeit · ✅ fertig · ⛔ blockiert

### Phase 0 – Fundament (seriell, zuerst)
| ID | Task | Akzeptanz | Status |
|---|---|---|---|
| T00 | **Scaffold**: Vite+TS+Three+Rapier+Vitest+Playwright, Ordnerstruktur, `core/types.ts`, EventBus, Game-Loop mit fixem Physik-Tick (50 Hz) + Render-Loop, leere Szenen Hauptmenü/Editor/Flug | `npm run dev` zeigt Menü; Build+Test grün; Screenshot | ⬜ |

### Phase 1 – Kern (parallel, nach T00)
| ID | Task | Ordner | Akzeptanz | Status |
|---|---|---|---|---|
| T10 | **Orbitalmechanik**: Kepler-Gleichung, State↔Elemente, Propagation, SOI-Wechsel, Patched Conics, Ap/Pe/Periode | `orbit/` | Tests gegen bekannte Werte (Kreis-/Ellipsen-/Hyperbelbahn, SOI-Übergang) | ⬜ |
| T11 | **Sternsystem Helion**: Daten aller Körper, Planeten-Mesh als Quadtree-Kugel mit Noise-Terrain + LOD | `world/` | Heimatplanet aus Orbit und am Boden sichtbar, 60 FPS | ⬜ |
| T12 | **Teile-System v1**: PartDef-JSON, 15 Basisteile (Kapsel, 3 Tanks, 3 Triebwerke, Decoupler, Nasenkonus, Fallschirm, Finnen, Landebeine, Batterie, Solarpanel, Strebe), prozedurale Meshes | `parts/` | Alle Teile laden & rendern im Teile-Showroom | ⬜ |
| T13 | **Vessel + Staging + Δv**: Teilebaum, Masse/Schwerpunkt, Stufen, Δv & TWR pro Stufe | `vessel/` | Tests: Δv-Berechnung einer 2-Stufen-Rakete korrekt | ⬜ |
| T14 | **HUD-Grundgerüst**: Navball, Höhe, Geschwindigkeit, Schub-Regler, Stufenliste, Zeitraffer | `ui/` | Screenshot, Tastatursteuerung (WASDQE, Shift/Ctrl, Leertaste) | ⬜ |

### Phase 2 – Spielbar (nach Phase 1)
| ID | Task | Akzeptanz | Status |
|---|---|---|---|
| T20 | **Editor/Hangar**: Teile-Palette, Snap an Knoten, Symmetrie, Undo/Redo, Speichern/Laden | Rakete bauen und speichern | ⬜ |
| T21 | **Flugphysik**: Vessel als Rapier-Körper(gelenke), Schub, Gravitation, Aero-Drag, Übergang Physik ↔ „on rails“ bei Zeitraffer | Start bis Orbit möglich | ⬜ |
| T22 | **Map-View**: Bahnen zeichnen, Ap/Pe-Marker, SOI-Übergänge, Manöverknoten mit Gizmo | Orbit-Planung zum Mond | ⬜ |
| T23 | **Kamera & Floating Origin** | Kein Zittern bei 10.000 km Entfernung | ⬜ |

### Phase 3 – Schöner (parallel)
| ID | Task | Status |
|---|---|---|
| T30 | Atmosphärische Streuung + Himmel-Shader + Sterne | ⬜ |
| T31 | Triebwerksflammen (höhenabhängig: eng im Vakuum, breit am Boden), Rauch, Bloom | ⬜ |
| T32 | Reentry-Plasma + Hitzesystem | ⬜ |
| T33 | Wolken, Ozean-Shader, Terrain-Texturen | ⬜ |
| T34 | Explosionen & Trümmer | ⬜ |

### Phase 4 – Mehr (parallel)
| ID | Task | Status |
|---|---|---|
| T40 | Neue Teile: Aerospike, Ionen, Nuklear, Grid-Fins, Hitzeschild, Docking-Port, Räder | ⬜ |
| T41 | Booster-Rücklandung (Autopilot „Suicide Burn“ + Grid-Fins) | ⬜ |
| T42 | Docking & Treibstofftransfer | ⬜ |
| T43 | Transferfenster-Rechner (Porkchop-Plot) | ⬜ |
| T44 | Nauten: Crew-Design, EVA | ⬜ |
| T45 | Sound (prozedural/eigene Samples) | ⬜ |
| T46 | Karriere/Forschung/Missionen | ⬜ |

---

## 7. Ablauf für das Hauptgehirn

1. Phase 0 (T00) als einzelne Worker-Session starten → prüfen → in `ccr-777d9743-kyqiye` mergen.
2. Phase-1-Tasks **parallel** als je eine Session starten (jeweils Worker-Prompt + Task-Zeile).
3. Ergebnisse prüfen: Branch auschecken, `npm ci && npm run build && npm test`, Screenshot ansehen.
4. Mergen, Konflikte selbst lösen, Task-Board hier aktualisieren, Schnittstellen nachziehen.
5. Nächste Phase. Nie mehr als ~5 Worker gleichzeitig.

## 8. Definition of Done (für jeden Task)
- Build + Tests grün, keine TS-Fehler
- Schnittstellen eingehalten
- Screenshot bei visuellen Tasks
- Kurzer Eintrag im Changelog unten

## 9. Changelog
- 2026-11-10: Master-Prompt erstellt (Hauptgehirn).
