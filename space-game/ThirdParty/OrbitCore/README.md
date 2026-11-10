# OrbitCore

Engine-freie Orbitmechanik-Bibliothek für **ORBITWERK** (C++17, keine Unreal-Abhängigkeit).
`namespace oc`, alles `double`, SI-Einheiten (m, s, rad, m³/s²). Wird später in das
UE-Modul `OrbitwerkCore` eingebunden und läuft bis dahin in der Cloud mit `cmake` + `ctest`.

## Bauen & Testen

```bash
cmake -S space-game/ThirdParty/OrbitCore -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Option `-DORBITCORE_BUILD_TESTS=OFF` baut nur die statische Bibliothek `OrbitCore` (Alias-Target
`OrbitCore::OrbitCore`). Tests nutzen [doctest](https://github.com/doctest/doctest) 2.4.11
(MIT, Header vendored in `tests/doctest.h`).

## Konventionen

* **Bezugssystem:** rechtshändig, Bezugsebene = XY, Referenzrichtung = +X, +Z = Norden
  (Drehimpuls einer prograden Äquatorbahn). Unreal ist linkshändig (cm, Z-up) – die Umrechnung
  passiert ausschließlich in `OrbitwerkCore`.
* `StateVector` ist immer relativ zum Mittelpunkt von `bodyId` (nicht rotierend).
* `OrbitalElements`: `a > 0` Ellipse, `a < 0` Hyperbel. `meanAnomalyAtEpoch` gilt zur Zeit `epoch`
  (Ellipse in [-π, π), Hyperbel: hyperbolische mittlere Anomalie `e·sinh H − H`).
* Winkel: `i ∈ [0, π]`, `lan`, `argPe ∈ [0, 2π)`.
* Unendliche Größen (Apoapsis/Periode offener Bahnen) werden als `+inf` gemeldet, nie `NaN`.

## API-Übersicht

| Header | Inhalt |
|---|---|
| `orbitcore/Types.h` | Verträge aus PROMPT.md Abschnitt 5: `Vec3d`, `OrbitalElements`, `Atmosphere`, `CelestialBody`, `StateVector`, Deklaration der Kern-API |
| `orbitcore/Vec3.h` | Inline-Vektormathe: `+ - * /`, `Dot`, `Cross`, `Length`, `Normalize`, `IsFinite` |
| `orbitcore/Kepler.h` | `SolveKeplerElliptic(M,e)`, `SolveKeplerHyperbolic(M,e)`, Anomalie-Umrechnungen (`TrueFromMean`, `MeanFromTrue`, …), `WrapPi`, `WrapTwoPi` |
| `orbitcore/Orbit.h` | `ElementsFromState`, `StateAt`, `Propagate`, `Periapsis`, `Apoapsis`, `Period`, `MeanMotion`, `SemiLatusRectum`, `SpecificEnergy`, `SpecificAngularMomentum`, `OrbitalSpeed` (Vis-Viva), `SpeedAt`, `CircularSpeed`, `EscapeSpeed`, `MeanAnomalyAt`, `TrueAnomalyAt`, `TimeAtTrueAnomaly`, `NextTimeAtRadius`, `TimeToPeriapsis`, `GravityAcceleration` |
| `orbitcore/System.h` | `class System` (Körperhierarchie), `ComputeSoiRadius` |
| `orbitcore/Patched.h` | `PatchedSegment`, `SegmentEnd`, `PatchedOptions`, `PredictPatched`, `FindSoiEntry` |
| `orbitcore/OrbitCore.h` | Sammel-Header |

### Kern-API (Abschnitt 5)

```cpp
oc::OrbitalElements oc::ElementsFromState(const StateVector&, double mu);
oc::StateVector     oc::StateAt(const OrbitalElements&, double mu, double t);
std::vector<oc::PatchedSegment> oc::PredictPatched(const StateVector&, const System&, double horizon);
```

* `StateAt` setzt `bodyId` nicht (Elemente kennen ihren Körper nicht); `Propagate` übernimmt sie.
* `horizon` ist eine **Dauer** in Sekunden ab `state.t`.

### Kepler-Solver

Beide Solver sind abgesicherte Newton-Verfahren mit Einschlussintervall (fällt bei schlechtem
Schritt auf Bisektion zurück) → Konvergenz für jedes `M` und jedes `e`, auch `e → 1`.
`E − e·sin E` bzw. `e·sinh H − H` werden auslöschungsfrei ausgewertet (Reihen für kleine
Winkel), und `StateAt` nutzt `1 − cos E = 2 sin²(E/2)` – dadurch bleiben fast-parabolische
Bahnen nahe der Periapsis präzise.

### `class System` (Ergänzung zu Abschnitt 5)

```cpp
oc::System sys({star, planet, moon});           // Reihenfolge egal
sys.Validate(&err);                             // genau eine Wurzel, Eltern existieren, keine Zyklen
sys.Get("moon"); sys.Find("x"); sys.ChildrenOf("planet"); sys.RootId(); sys.Depth("moon");
sys.BodyStateRelativeToParent("moon", t);       // Ephemeride "on rails"
sys.BodyStateAbsolute("moon", t);               // relativ zur Wurzel (Stern)
sys.Transform(state, "planet");                 // Bezugskörper wechseln
sys.FindDominantBody(rAbs, t);                  // tiefster Körper, in dessen SOI rAbs liegt
sys.ToDominantBody(state);                      // State in den "richtigen" Körper umhängen
sys.IsOutsideSoi(state);
sys.GravityAcceleration(state);                 // Patched-Conic-Gravitation (nur bodyId)
```

* `soiRadius <= 0` wird automatisch per Laplace-Formel `a·(μ_kind/μ_eltern)^(2/5)` gesetzt.
* Die Wurzel (Stern) hat unendliche SOI; Körper ohne Eltern müssen kein `orbit` haben.

### Patched Conics (Ergänzung zu Abschnitt 5)

```cpp
enum class SegmentEnd { Horizon, SoiExit, SoiEntry, Impact };
struct PatchedSegment {
  std::string bodyId; double mu; OrbitalElements elements;
  double startTime, endTime; StateVector startState, endState;
  SegmentEnd endReason; std::string nextBodyId;
  StateVector StateAtTime(double t) const;   // für die Bahnzeichnung in der Map-View
};
struct PatchedOptions { int maxSegments = 16; bool stopAtImpact = true; ... };
```

Ablauf pro Segment: State in den dominanten Körper umhängen → Elemente →
früheste Ereigniszeit aus
1. **SOI-Austritt** (analytisch über `NextTimeAtRadius(soi, outbound)`),
2. **Aufschlag** auf `radius` (analytisch, inbound),
3. **SOI-Eintritt** in ein Kind (`FindSoiEntry`: Schnellausschluss über Radienbereiche,
   adaptive Schrittweite `0.5·(d − soi)/v_rel`, begrenzt durch Bahnperioden, dann Bisektion).
   Ein Eintritt zählt nur als Übergang von außen nach innen – direkt nach einem Austritt wird
   der verlassene Körper also nicht sofort wieder betreten.

`Horizon` und `Impact` beenden die Vorhersage. Atmosphäre ist (noch) kein Ereignis.

### Gravitation für die Physik

`oc::GravityAcceleration(r, mu)` = `−μ·r/|r|³` (bei `r = 0` → Nullvektor).
`System::GravityAcceleration(state)` nutzt den Bezugskörper des States (konsistent mit den
Patched Conics, also kein Sprung beim „on rails“ ↔ Physik-Wechsel).

## Grenzfälle

| Fall | Verhalten |
|---|---|
| `e ≈ 0` (< 1e-11) | `e = 0`, `argPe = 0`, Anomalie = Argument der Breite |
| `i ≈ 0` / `i ≈ π` | `lan = 0`, `argPe` = Länge der Periapsis (Vorzeichen für retrograd korrekt) |
| `|e − 1| < 1e-13` | `e` wird auf `1 ± 1e-13` geschoben (sonst `a = ∞`) – Fehler weit unter 1 mm |
| rein radial (`r × v = 0`, z. B. senkrechter Start ohne Rotation, Ruhe) | extrem schlanker Kegelschnitt mit `e = 1 ∓ 1e-15`; Positionsfehler < 5 m. Exakt radial **und** exakt Fluchtgeschwindigkeit: `|a|` auf `1e6·r` gekappt, relativer Fehler < 1e-4 |
| ungültige Eingaben (`r = 0`, NaN) | endliche Ersatzwerte, nie NaN |

## Tests (`tests/`)

* `test_kepler.cpp` – Solver (Ellipse/Hyperbel inkl. `e → 1`, riesige `M`), Anomalie-Roundtrips
* `test_orbit.cpp` – Kreisbahn, inklinierte Ellipse (Energie-/Drehimpulserhaltung über ~3000
  Umläufe, 8000 verkettete Propagationen), Vergleich mit RK4-Integration (Ellipse + Hyperbel),
  Hyperbel, 5000 zufällige State→Elemente→State-Roundtrips (inkl. retrograd/hyperbolisch),
  Grenzfälle `e≈0`, `i≈0`, `i=π`, `e≈1`, parabolisch, radial, keine NaNs, Zeit-bei-Radius
* `test_system.cpp` – Hierarchie/SOI/Transformationen, dominanter Körper, Patched Conics:
  stabile LEO-Bahn, **Planet → Mond → Planet (Swing-by)**, Mond-Aufschlag, Flucht zum Stern,
  suborbitaler Hüpfer und senkrechter Start mit Aufschlag

## Offene Punkte / Ideen

* Atmosphären-Eintritt als `SegmentEnd` (für Aerobraking-Vorhersage).
* Manöverknoten (Δv an Zeitpunkt → neues Segment) – gehört zu T22.
* Körperrotation (Oberflächengeschwindigkeit beim Start) – `rotationPeriod` ist vorhanden,
  aber noch ungenutzt.
* Begegnungssuche ist schrittbasiert: extrem streifende SOI-Kontakte (Eindringtiefe ≪
  `minStepSoiFraction·soi`) können übersehen werden.
