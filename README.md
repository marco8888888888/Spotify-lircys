# Spotify Lyrics Translator

Lyrics für (fast) jeden Song – synchron zur Musik und übersetzt in über 100 Sprachen.

| | Computer | Handy (Android & iPhone) |
|---|---|---|
| Was | Erweiterung direkt in der Spotify-Desktop-App ([Spicetify](https://spicetify.app)) | Web-App, installierbar auf dem Startbildschirm |
| Datei | [`spicetify/lyrics-translator.js`](spicetify/lyrics-translator.js) | [`docs/`](docs) |
| Song-Erkennung | automatisch | automatisch über Spotify-Login (oder manuelle Suche) |
| Mitlaufende Zeilen | ✅ (Klick auf Zeile = springen) | ✅ |

> Warum zwei Teile? Die offizielle Spotify-App auf dem Handy erlaubt keine Plugins. Deshalb gibt es fürs Handy
> eine eigene App, die erkennt, was gerade in Spotify läuft, und die Lyrics dazu anzeigt.

## Woher kommen die Lyrics?

Damit es für möglichst jeden Song Lyrics gibt, werden mehrere Quellen nacheinander gefragt:

1. **Spotify** selbst (nur am Computer)
2. **[LRCLIB](https://lrclib.net)** – große, freie Datenbank mit synchronisierten Lyrics
3. **[lyrics.ovh](https://lyrics.ovh)** – als letzte Rückfallebene (nur Text, nicht synchron)

Synchronisierte Lyrics werden bevorzugt. Wenn ein Song in keiner Quelle steht (z. B. sehr neue oder unbekannte
Titel), wird „keine Lyrics gefunden“ angezeigt – 100 % aller Songs kann leider keine Datenbank abdecken.

Übersetzt wird mit Google Translate. Die Originalsprache wird automatisch erkannt.

---

## Installation am Computer (Windows / macOS / Linux)

1. **Spicetify installieren** (Anleitung: <https://spicetify.app/docs/getting-started>)
   - Windows (PowerShell):
     `iwr -useb https://raw.githubusercontent.com/spicetify/cli/main/install.ps1 | iex`
   - macOS / Linux:
     `curl -fsSL https://raw.githubusercontent.com/spicetify/cli/main/install.sh | sh`
2. **Erweiterung kopieren:** `spicetify/lyrics-translator.js` in den Extensions-Ordner legen.
   Den Ordner findest du mit `spicetify config-dir` → Unterordner `Extensions`.
3. **Aktivieren:**
   ```sh
   spicetify config extensions lyrics-translator.js
   spicetify apply
   ```
4. In Spotify oben auf das **Übersetzen-Symbol** (文A) klicken → das Lyrics-Fenster öffnet sich.
   Dort Zielsprache und Anzeige (Original + Übersetzung / nur Übersetzung / nur Original) wählen.

Nach einem Spotify-Update ggf. erneut `spicetify apply` ausführen.

---

## Installation am Handy

### 1. Web-App veröffentlichen (einmalig)

Auf GitHub im Repo: **Settings → Pages → Source: „Deploy from a branch“ → Branch `main`, Ordner `/docs`**.
Die App ist danach erreichbar unter `https://<dein-github-name>.github.io/<repo-name>/`.

### 2. Auf dem Handy installieren

- **Android (Chrome):** Seite öffnen → Menü ⋮ → **„App installieren“** / „Zum Startbildschirm“
- **iPhone (Safari):** Seite öffnen → Teilen-Symbol → **„Zum Home-Bildschirm“**

### 3. Mit Spotify verbinden (damit der laufende Song automatisch erkannt wird)

1. Auf <https://developer.spotify.com/dashboard> einloggen → **Create app**
2. Bei **Redirect URI** genau die Adresse eintragen, die in der App unter ⚙︎ angezeigt wird
   (z. B. `https://<dein-github-name>.github.io/<repo-name>/`), bei „APIs used“ **Web API** ankreuzen, speichern.
3. Die **Client ID** der Spotify-App kopieren, in der Lyrics-App unter ⚙︎ einfügen → **Mit Spotify verbinden**.

Jetzt einfach in Spotify Musik hören und die Lyrics-App daneben öffnen (z. B. im Split-Screen oder
nebenher). Sie zeigt immer den aktuellen Song – auch wenn die Musik auf einem anderen Gerät läuft.

Ohne Spotify-Login kannst du unter ⚙︎ → **Song manuell suchen** jeden Song nach Künstler und Titel suchen.

> Hinweis: Neue Spotify-Developer-Apps sind im „Development Mode“. Wenn auch andere Personen die App nutzen
> sollen, musst du sie im Dashboard unter **User Management** mit ihrer Spotify-E-Mail hinzufügen.

---

## Datenschutz

Alles läuft lokal im Browser bzw. in Spotify. Der Spotify-Login (OAuth mit PKCE, ohne Passwort-Weitergabe)
wird nur auf deinem Gerät gespeichert. Künstler, Titel und Liedzeilen werden an LRCLIB, lyrics.ovh bzw.
Google Translate geschickt, um Lyrics und Übersetzungen zu laden.
