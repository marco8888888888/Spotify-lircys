# T00 – UE5-Scaffold: Test-Anleitung (Windows)

Ziel: Das Projekt kompiliert ohne Fehler, das **Hauptmenü** erscheint und der Szenenwechsel
Hauptmenü → Hangar / Flug → Hauptmenü funktioniert.

---

## 0. Voraussetzungen (einmalig)

| Was | Wie |
|---|---|
| Unreal Engine **5.6** (oder neuer) | Epic Games Launcher → Unreal Engine → Bibliothek → Engine-Version hinzufügen |
| Visual Studio **2022** | Workloads: **„Spieleentwicklung mit C++“** + **„Desktopentwicklung mit C++“**; Einzelkomponenten: aktuelles **Windows 11 SDK**, **MSVC v143**, **.NET-Framework-4.8-SDK** |
| Git LFS | `git lfs install` (einmalig pro Rechner) |

> Bei UE 5.7+: `.uproject` → Rechtsklick → *Switch Unreal Engine version…* → 5.7 wählen.
> Die Target-Dateien nutzen `BuildSettingsVersion.Latest`, sollten also ohne Änderung bauen.

---

## 1. Branch holen

```bat
git fetch origin orbitwerk/t00-ue5-scaffold
git checkout orbitwerk/t00-ue5-scaffold
git lfs pull
cd space-game
```

---

## 2. Projektdateien generieren

**Variante A (Explorer):** Rechtsklick auf `space-game\Orbitwerk.uproject` → **„Generate Visual Studio project files“**.
(Fehlt der Eintrag: unter *Weitere Optionen anzeigen* suchen.)

**Variante B (Konsole):** Pfad zur Engine ggf. anpassen.

```bat
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="%CD%\Orbitwerk.uproject" -game -rocket -progress
```

Ergebnis: `Orbitwerk.sln` liegt in `space-game\`.

> Falls `ThirdParty/OrbitCore` (Task T10) schon im Branch ist, legt dieser Schritt automatisch
> `Source/OrbitwerkCore/Private/OrbitCoreSources/OC_*.cpp` an (git-ignoriert). Kommen später neue
> OrbitCore-Dateien dazu: Schritt 2 wiederholen.

---

## 3. Kompilieren

**Variante A (Visual Studio):**
1. `Orbitwerk.sln` öffnen.
2. Oben: Konfiguration **Development Editor**, Plattform **Win64**.
3. Im Projektmappen-Explorer Rechtsklick auf **Games → Orbitwerk** → **Erstellen** (Strg+Umschalt+B baut alles).

**Variante B (Konsole):**

```bat
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" OrbitwerkEditor Win64 Development -Project="%CD%\Orbitwerk.uproject" -WaitMutex
```

Erwartet: `Build succeeded` / `0 Fehler`. Bei Fehlern: **komplettes Ausgabefenster** (bzw. Konsolenausgabe)
kopieren und zurückschicken.

---

## 4. Editor starten

Doppelklick auf `Orbitwerk.uproject` (oder in VS **F5** mit *Development Editor*).

- Beim allerersten Start werden Shader kompiliert (kann einige Minuten dauern).
- Meldung *„The following modules are missing or built with a different engine version … Rebuild?“* → **Ja**.
- Warnung, dass `/Game/Maps/MainMenu` fehlt, ist beim ersten Start **normal** (Maps kommen in Schritt 5).

---

## 5. Maps per Python anlegen

1. Im Editor: Menü **Tools → Execute Python Script…** (bei manchen Versionen unter **File**).
2. Datei `space-game\Scripts\create_maps.py` auswählen.
3. **Output Log** (Window → Output Log) prüfen. Erwartet:
   ```
   [Orbitwerk] /Game/Maps/MainMenu angelegt.
   [Orbitwerk] /Game/Maps/Hangar angelegt.
   [Orbitwerk] /Game/Maps/Flight angelegt.
   [Orbitwerk] Fertig: MainMenu, Hangar, Flight sind vorhanden.
   ```
4. Im Content Browser gibt es jetzt `Content/Maps/MainMenu`, `Hangar`, `Flight`. Der Editor hat `MainMenu` geöffnet.

Alternative: Output Log → links unten von **Cmd** auf **Python** umstellen und eingeben:
```
py "C:/Pfad/zu/space-game/Scripts/create_maps.py"
```

Das Skript ist wiederholbar; vorhandene Maps werden nicht überschrieben.

Danach die Maps committen (gehen über Git LFS):
```bat
git add Content/Maps
git commit -m "T00: leere Maps MainMenu/Hangar/Flight"
```

---

## 6. Testen im Editor (PIE)

1. `MainMenu` ist geöffnet → **Play** (Alt+P).
2. **Erwartet:** dunkelblauer Bildschirm mit **ORBITWERK**, Untertitel und Buttons
   *Hangar · Flug starten · Grafik: … · Grafik automatisch erkennen · Beenden*. Mauszeiger sichtbar.
3. **Hangar** klicken → Szene wechselt, oben links Overlay „Hangar“ mit Hinweistext.
   WASD + Maus bewegen die Debug-Kamera (Szene ist leer/dunkel – das ist korrekt).
4. **Backspace** → zurück ins Hauptmenü.
   (**Esc** beendet im Editor PIE selbst – deshalb im PIE Backspace benutzen; im Standalone-Spiel geht Esc.)
5. **Flug starten** → Overlay „Flug“ → Backspace → Hauptmenü.
6. **Grafik: …** mehrfach klicken → Preset wechselt Laptop → Niedrig → Mittel → Hoch → Episch → Kino → Laptop.
   Im Output Log: `LogOrbitwerkRender: Grafik-Preset angewendet: …`.
7. **Grafik automatisch erkennen** → nach 1–3 s steht unten z. B. `Benchmark: CPU 180 / GPU 240 -> Preset Hoch`.
8. **Beenden** → PIE endet.

Zusätzlich im Output Log prüfen (Filter „Orbitwerk“):
```
LogOrbitwerkCore: OrbitwerkCore gestartet. OrbitCore-Bibliothek: eingebunden | nicht gefunden (...)
LogOrbitwerk: Hauptmenue angezeigt.
```

Konsole (Taste `^` oder `~`) – optionale Checks:
```
ow.Clouds.Quality          -> zeigt Wert (Laptop 0, Episch 3)
r.Lumen.DiffuseIndirect.Allow -> 0 bei Laptop/Niedrig/Mittel, 1 ab Hoch
```

---

## 7. Standalone-Test (Erststart-Benchmark, Esc)

1. Play-Dropdown (⋮ neben Play) → **Standalone Game**.
2. Beim ersten Start läuft der Benchmark automatisch (kurzes Stocken), danach Hauptmenü;
   der Grafik-Button zeigt das vorgeschlagene Preset.
3. **Esc** in Hangar/Flug → zurück ins Hauptmenü.
4. Die Einstellungen landen in `space-game\Saved\Config\Windows\GameUserSettings.ini`
   (Sektion `[/Script/OrbitwerkRender.OrbitwerkGraphicsSettings]`, `bInitialBenchmarkDone=True`).
   Zum erneuten Testen des Erststarts diese Datei löschen.

---

## 8. Was bitte zurückschicken

- [ ] Compile-Ergebnis (bei Fehlern das komplette Log)
- [ ] Screenshot vom Hauptmenü
- [ ] Screenshot Hangar- oder Flug-Overlay
- [ ] Zeile mit dem Benchmark-Ergebnis + Laptop/PC-Angabe
- [ ] `space-game\Saved\Logs\Orbitwerk.log` (bei Problemen)

---

## Fehlerbehebung

| Symptom | Lösung |
|---|---|
| „Could not find a part of the path …OrbitCoreSources“ o. Ä. | Schritt 2 erneut ausführen. Notfalls OrbitCore abschalten: `set ORBITWERK_DISABLE_ORBITCORE=1`, dann Schritt 2 + 3 aus **derselben** Konsole. |
| Compile-Fehler in `ThirdParty/OrbitCore/...` | Ist ein T10-Problem → Log schicken; vorübergehend `ORBITWERK_DISABLE_ORBITCORE=1` (s. o.). |
| Menü „Execute Python Script“ fehlt | Edit → Plugins → „Python Editor Script Plugin“ und „Editor Scripting Utilities“ aktivieren, Editor neu starten. |
| Roter Text „Map /Game/Maps/… fehlt“ | Schritt 5 ausführen. |
| Kein Menü, nur schwarzes Bild | Output Log nach `LogOrbitwerk` durchsuchen; prüfen, dass in *World Settings* der Map kein anderer GameMode-Override gesetzt ist. |
| Backspace/Esc ohne Wirkung | Log-Zeile „InputComponent ist kein UEnhancedInputComponent“? → `Config/DefaultInput.ini` prüfen. |
| `.umap` sind nur kleine Textdateien nach dem Klonen | `git lfs install` und `git lfs pull` ausführen. |
