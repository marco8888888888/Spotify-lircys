# ORBITWERK – Master-Prompt (Hauptgehirn)

> Arbeitstitel: **Orbitwerk** – ein eigenes Raketenbau- und Raumfahrt-Simulationsspiel in **Unreal Engine 5**.
> Inspiriert vom Genre (Raketen aus Teilen bauen, starten, Orbits fliegen, landen),
> aber **komplett eigenständig**: eigene Welt, eigene Crew, eigene Teile, eigener Look.
> Keine Namen, Figuren, Planeten, Grafiken, Sounds oder UI-Elemente aus bestehenden Spielen übernehmen.

---

## 1. Rollen

### 1.1 Hauptgehirn (Orchestrator)
Das Hauptgehirn ist die Session, die diese Datei pflegt. Es
- hält die **Vision** und die **Architektur** (diese Datei ist die Wahrheit),
- zerlegt Arbeit in **Tasks** (Abschnitt 7) mit klaren Schnittstellen,
- startet für jeden Task eine **Worker-Session** (eigener Chat) mit dem Worker-Prompt (Abschnitt 6),
- prüft Ergebnisse (Code-Review, OrbitCore-Tests), merged, aktualisiert das Task-Board,
- sammelt Compile-Fehler / Screenshots vom **Tester (Marco)** und verteilt Fixes.

### 1.2 Worker
Ein Worker bekommt genau **einen** Task, arbeitet auf **einem eigenen Branch** (`orbitwerk/<task-id>-…`),
hält sich an die Schnittstellen in Abschnitt 5 und liefert: Code + Tests + Bericht + ggf. Editor-Anleitung.

### 1.3 Tester (Mensch)
Die Cloud-Chats haben **keine GPU und keine Unreal-Installation**. Marco kompiliert und testet
lokal (Laptop, oder per **Parsec** auf dem starken PC) und schickt Compile-Log + Screenshots zurück.
→ Deshalb muss jeder Task **genaue Test-Schritte** im Bericht haben.

---

## 2. Vision – was "besser" heißt

| Bereich | Ziel |
|---|---|
| **Grafik** | Lumen GI, Nanite-Terrain/Teile, **Sky Atmosphere** (echte Streuung, Himmel wird mit Höhe schwarz), **Volumetric Clouds**, Niagara-Triebwerksflammen (höhenabhängig), Reentry-Plasma, Hitzeflimmern, Sternenhimmel, Planeten mit LOD |
| **Grafik-Einstellungen** | Presets **Laptop / Niedrig / Mittel / Hoch / Episch / Kino**; einzeln: Auflösungsskala, Upscaler (TSR, optional DLSS/FSR), Lumen an/aus (Fallback: SSGI), Wolkenqualität, Schatten, Nanite-Detail, Partikeldichte, FPS-Limit, VSync. Auto-Erkennung beim ersten Start (Benchmark). Muss auf schwachem Laptop spielbar laufen. |
| **Physik** | Stabile Raketen-Physik (Chaos), korrekte Orbitalmechanik (Patched Conics, Kepler „on rails“), Zeitraffer ohne Explosionen, Aero-Drag/Lift, Hitze |
| **Bauen** | Snap-Knoten, Symmetrie (2/3/4/6/8x), Undo/Redo, Stufen-Editor mit Δv (Vakuum + Meereshöhe) und TWR |
| **Neue Teile** | Aerospikes, Ionen, Nuklear-thermisch, Grid-Fins, ausfahrbare Hitzeschilde, Booster-Rücklandung, Docking-Ports, Rover, Solarsegel, Treibstofftransfer |
| **Komfort** | Manöverknoten mit Δv-Planer, Transferfenster-Rechner, Autopilot (Prograde/Retro/Normal/Radial halten), Quicksave |
| **Welt** | Sternsystem **Helion**: Heimatplanet **Terran-Vey**, Mond **Pell**, Wüstenplanet **Okkar**, Gasriese **Thalassa** mit Monden. Crew-Spezies: **die Nauten** (eigenes Design) |

**Leitprinzip:** Erst spielbarer Kern (bauen → starten → Orbit), dann schöner, dann mehr.

---

## 3. Technik-Stack (fest)

- **Engine:** Unreal Engine **5.6 oder neuer** (Large World Coordinates = doppelte Präzision)
- **Sprache:** **C++ zuerst**. Blueprints nur dünn für UI-Layout/Tuning.
- **Physik:** Chaos für Starrkörper im Nahbereich; **OrbitCore** (eigene C++-Bibliothek) für Orbits.
- **UI:** UMG + CommonUI, Logik in C++.
- **Effekte:** Niagara, Materialien mit Material-Parametern aus C++.
- **Daten:** Teile & Himmelskörper als **JSON/DataAssets** (Text, gut für Chats).
- **Assets ohne Editor erzeugen:** Wo möglich prozedural (C++ `UDynamicMeshComponent`/GeometryScript) oder per **Unreal-Python-Editor-Skript** (`space-game/Scripts/*.py`), das Marco einmal im Editor ausführt. So bleibt fast alles Text.
- **Git:** Binärdateien (`.uasset`, `.umap`, Texturen) über **Git LFS**.

### Ordnerstruktur
```
space-game/
  PROMPT.md                 ← diese Datei (Wahrheit)
  Orbitwerk.uproject
  Config/                   ← DefaultEngine.ini, DefaultScalability.ini (Grafik-Presets)
  Source/
    Orbitwerk/              ← Haupt-Spielmodul (GameMode, Szenen, Input)
    OrbitwerkCore/          ← UE-Modul: Vessel, Parts, Staging, Physik-Brücke
    OrbitwerkRender/        ← UE-Modul: Planeten-LOD, Atmosphäre, Effekte, Grafik-Settings
    OrbitwerkUI/            ← UE-Modul: HUD, Navball, Editor-UI, Settings-Menü
  ThirdParty/OrbitCore/     ← REINES C++17, KEINE UE-Abhängigkeit, CMake + Tests (läuft in Cloud!)
    include/ src/ tests/ CMakeLists.txt
  Data/Parts/*.json  Data/Bodies/*.json
  Scripts/                  ← Unreal-Python-Skripte zum Erzeugen von Assets/Levels
  Content/                  ← (LFS) Maps, Materialien, Niagara
  docs/                     ← Test-Anleitungen, Editor-Schritte
```

**Wichtig:** Alles, was ohne Engine testbar ist (Orbitmechanik, Δv, Teile-Parsing, Staging-Logik),
gehört in `ThirdParty/OrbitCore` und wird in der Cloud mit `cmake && ctest` getestet.

---

## 4. Grafik-Settings-System (Pflicht für alle Render-Tasks)

- Klasse `UOrbitwerkGraphicsSettings : UGameUserSettings` (in `OrbitwerkRender`).
- Jeder visuelle Effekt **muss** eine Qualitätsstufe 0–4 respektieren (CVar `ow.<effekt>.Quality`)
  und auf Stufe 0 billig oder aus sein.
- Preset **„Laptop“**: 50–67 % Auflösung + TSR, Lumen aus, Wolken niedrig, Schatten niedrig, 30–60 FPS Ziel.
- Erststart: `RunHardwareBenchmark()` → Preset vorschlagen.

---

## 5. Schnittstellen-Verträge (nicht ohne Hauptgehirn ändern)

```cpp
// ThirdParty/OrbitCore/include/orbitcore/Types.h   (namespace oc, alles double, SI-Einheiten)
struct Vec3d { double x, y, z; };

struct OrbitalElements { double a, e, i, lan, argPe, meanAnomalyAtEpoch, epoch; };

struct Atmosphere { double height, seaLevelPressure, scaleHeight; };

struct CelestialBody {
  std::string id, name, parentId;   // parentId leer = Stern
  double mu, radius, soiRadius, rotationPeriod;
  std::optional<OrbitalElements> orbit;
  std::optional<Atmosphere> atmosphere;
};

struct StateVector { Vec3d r, v; double t; std::string bodyId; };

// Kern-API
OrbitalElements ElementsFromState(const StateVector&, double mu);
StateVector     StateAt(const OrbitalElements&, double mu, double t);
std::vector<struct PatchedSegment> PredictPatched(const StateVector&, const class System&, double horizon);
```

```cpp
// Teile (Data/Parts/*.json → oc::PartDef)
struct PartNode { std::string id; Vec3d pos, dir; std::string size; };
struct PartDef {
  std::string id, name, category;   // engine|tank|command|structural|decoupler|aero|utility|science
  double mass;                      // t (trocken)
  std::string size;                 // 0.625|1.25|2.5|3.75|5
  std::vector<PartNode> nodes;
  nlohmann::json modules;           // z. B. {"engine":{"thrustVac":..,"ispVac":..,"ispSL":..}}
  std::string mesh;                 // Generator-ID oder Asset-Pfad
};
struct VesselPart { std::string uid, defId, parentUid, attachNode; int stage; std::string symmetryGroup; };
struct VesselDesign { std::string name; std::vector<VesselPart> parts; };
```

**Floating Origin:** UE5 Large World Coordinates + Origin-Rebasing aufs aktive Vessel.
Orbit-Positionen bleiben in `oc::Vec3d` (Planeten-zentriert), Umrechnung nur in `OrbitwerkCore`.

---

## 6. Worker-Prompt (Vorlage für jeden neuen Chat)

```
Du bist Worker im Projekt ORBITWERK (eigenes Raketenbau-/Raumfahrtspiel in Unreal Engine 5, Ordner space-game/).
Lies ZUERST space-game/PROMPT.md komplett – das ist die verbindliche Architektur.

Dein Task: <TASK-ID> – <Titel>
Ziel: <Beschreibung aus Abschnitt 7>
Akzeptanzkriterien: <aus Abschnitt 7>

Wichtig zur Umgebung: Du hast KEINE Unreal Engine und KEINE GPU. Schreibe UE-C++ sehr sorgfältig
(korrekte Includes, UCLASS/GENERATED_BODY, Build.cs-Abhängigkeiten), da du nicht kompilieren kannst.
Alles Engine-unabhängige gehört nach ThirdParty/OrbitCore und MUSS mit cmake + ctest grün sein.

Regeln:
- Arbeite NUR in den Ordnern deines Tasks. Schnittstellen aus Abschnitt 5 nicht ändern; Vorschläge in den Bericht.
- Keine Inhalte aus bestehenden Spielen (Namen, Figuren, Grafiken, Sounds). Alles eigen.
- Visuelle Features respektieren das Grafik-Settings-System (Abschnitt 4).
- Assets möglichst prozedural oder als Unreal-Python-Skript in Scripts/, keine handgebauten .uasset.
- Lege docs/<TASK-ID>-test.md an: genaue Schritte, wie der Tester es im Editor prüft.
- Committe auf deinen Branch und pushe. Keinen PR öffnen.
- Bericht am Ende: Was gebaut, welche Dateien, Test-Schritte, offene Punkte.
```

---

## 7. Task-Board

Status: ⬜ offen · 🟨 in Arbeit · 🧪 wartet auf Test durch Marco · ✅ fertig · ⛔ blockiert

### Phase 0 – Fundament (parallel)
| ID | Task | Akzeptanz | Status |
|---|---|---|---|
| T00 | **UE5-Scaffold**: `.uproject`, 4 Module mit Build.cs, GameMode, Szenen (Hauptmenü/Hangar/Flug) per C++, Enhanced Input, `.gitignore` + `.gitattributes` (LFS), `DefaultScalability.ini` mit Laptop-Preset, `UOrbitwerkGraphicsSettings`-Grundgerüst, Python-Skript das leere Maps anlegt | Projekt kompiliert bei Marco, Menü erscheint | 🟨 |
| T10 | **OrbitCore**: CMake-Lib, Kepler-Gleichung (ell./hyperb.), State↔Elemente, Propagation, SOI-Wechsel, Patched Conics, Ap/Pe/Periode, Tests (doctest) | `ctest` grün, Testfälle für Kreis/Ellipse/Hyperbel/SOI | 🟨 |

### Phase 1 – Kern (parallel, nach T00)
| ID | Task | Akzeptanz | Status |
|---|---|---|---|
| T11 | **Sternsystem Helion**: `Data/Bodies/*.json`, Planeten-Quadtree-LOD (Dynamic Mesh, Noise-Terrain), Sky Atmosphere pro Planet | Heimatplanet aus Orbit + Boden sichtbar | ⬜ |
| T12 | **Teile v1**: JSON-Parser in OrbitCore, 15 Basisteile, prozedurale Meshes (Kapsel, Tanks, Triebwerke …) | Teile-Showroom-Map zeigt alle Teile | ⬜ |
| T13 | **Vessel + Staging + Δv** (Logik in OrbitCore, Actor in OrbitwerkCore) | ctest: Δv 2-Stufen-Rakete korrekt | ⬜ |
| T14 | **HUD v1**: Navball, Höhe, Geschwindigkeit, Schub, Stufen, Zeitraffer, **Grafik-Settings-Menü** | Steuerung WASDQE, Shift/Ctrl, Leertaste | ⬜ |

### Phase 2 – Spielbar
| ID | Task | Status |
|---|---|---|
| T20 | Hangar-Editor: Palette, Snap, Symmetrie, Undo/Redo, Speichern/Laden | ⬜ |
| T21 | Flugphysik: Chaos-Körper + Constraints, Schub, Gravitation aus OrbitCore, Aero, Physik ↔ on-rails | ⬜ |
| T22 | Map-View: Bahnen, Ap/Pe, SOI, Manöverknoten mit Gizmo | ⬜ |
| T23 | Kamera & Origin-Rebasing, kein Zittern auf 10.000 km | ⬜ |

### Phase 3 – Krasse Grafik
| ID | Task | Status |
|---|---|---|
| T30 | Atmosphäre/Himmel-Tuning, Sternenhimmel, Planeten-Shading aus dem All | ⬜ |
| T31 | Niagara-Triebwerksflammen (Vakuum eng / Boden breit), Rauch, Startrampen-Dampf | ⬜ |
| T32 | Reentry-Plasma + Hitze-System + Hitzeflimmern | ⬜ |
| T33 | Volumetrische Wolken, Ozean, Terrain-Materialien (Nanite) | ⬜ |
| T34 | Explosionen & Trümmer (Chaos Destruction) | ⬜ |
| T35 | Performance-Pass: Laptop-Preset ≥ 30 FPS, Benchmark-Autodetect | ⬜ |

### Phase 4 – Mehr
| ID | Task | Status |
|---|---|---|
| T40 | Neue Teile: Aerospike, Ionen, Nuklear, Grid-Fins, Hitzeschild, Docking-Port, Räder | ⬜ |
| T41 | Booster-Rücklandung (Autopilot + Grid-Fins) | ⬜ |
| T42 | Docking & Treibstofftransfer | ⬜ |
| T43 | Transferfenster-Rechner (Porkchop-Plot) | ⬜ |
| T44 | Nauten: Crew-Design, EVA | ⬜ |
| T45 | Sound (MetaSounds) | ⬜ |
| T46 | Karriere/Forschung/Missionen | ⬜ |

---

## 8. Ablauf für das Hauptgehirn

1. T00 + T10 parallel starten.
2. Ergebnisse reviewen, OrbitCore-Tests selbst laufen lassen, nach `ccr-777d9743-kyqiye` mergen.
3. Marco kompiliert (Laptop oder Parsec) → schickt Log/Screenshot → Fix-Tasks verteilen.
4. Phase-1-Tasks parallel starten (max. ~5 Worker gleichzeitig).
5. Task-Board + Changelog hier aktualisieren.

## 9. Definition of Done
- OrbitCore: `ctest` grün · UE-Teil: kompiliert bei Marco ohne Warnungen-Flut
- Schnittstellen eingehalten, Grafik-Settings respektiert
- `docs/<TASK-ID>-test.md` vorhanden, Screenshot vom Tester bei visuellen Tasks

## 10. Changelog
- 2026-11-10: Master-Prompt erstellt (Web-Stack).
- 2026-11-10: Umstieg auf **Unreal Engine 5** für maximale Grafik; Grafik-Settings mit Laptop-Preset; OrbitCore als engine-freie, cloud-testbare Bibliothek. Web-Scaffold (Branch `orbitwerk/t00-scaffold`) verworfen.
