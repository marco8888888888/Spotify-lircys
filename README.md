# Spotify Lyrics Translator

Lyrics für (fast) jeden Song – direkt in Spotify, synchron zur Musik und übersetzt in über 100 Sprachen.
**Ohne Spicetify**, ohne Zusatzprogramme.

| Gerät | Wie es funktioniert | Ordner |
|---|---|---|
| **Computer** (Windows, Mac, Linux) | Ein Installer baut den Mod direkt in die Spotify-App ein. Neben Spotifys Lyrics-Knopf erscheint ein Übersetzen-Knopf. | [`desktop/`](desktop) |
| **Android** | Eigene App: Übersetzung direkt unter Spotifys Lyrics, Lyrics-Fenster nur in Spotify, dazu ein eigener Player im Spotify-Stil. | [`android/`](android) |
| **iPhone** | Web-App zum Installieren auf dem Home-Bildschirm. Läuft neben Spotify, weil iOS keine Fenster über anderen Apps erlaubt. | [`docs/`](docs) |

## Woher kommen die Lyrics?

Damit es für möglichst jeden Song Lyrics gibt, werden mehrere Quellen nacheinander gefragt:

1. **[LRCLIB](https://lrclib.net)** – große, freie Datenbank mit synchronisierten Lyrics (mitlaufende Zeilen)
2. **[lyrics.ovh](https://lyrics.ovh)** – Rückfallebene (nur Text, nicht synchron)

Synchronisierte Lyrics werden bevorzugt. Bei sehr neuen oder unbekannten Songs kann es trotzdem vorkommen,
dass keine Quelle Lyrics hat – dann steht da „keine Lyrics gefunden“.

Übersetzt wird mit Google Translate. Die Originalsprache wird automatisch erkannt.
Anzeige wählbar: **Original + Übersetzung**, **nur Übersetzung** oder **nur Original**.

---

## 💻 Computer: Spotify modden

Voraussetzung: die normale Spotify-App von <https://www.spotify.com/download>
(unter Windows **nicht** die Microsoft-Store-Version – die lässt sich nicht verändern).

### Windows

1. Dieses Repo herunterladen: grüner Knopf **Code → Download ZIP**, entpacken.
2. Im Ordner `desktop` doppelt auf **`install-windows.bat`** klicken.
3. Spotify startet neu. Unten rechts neben dem Lyrics-Knopf ist jetzt der **Übersetzen-Knopf (文A)**.
   Alternativ: **Strg + Umschalt + L**.

Entfernen: `uninstall-windows.bat` doppelklicken.

### Mac / Linux

```sh
cd desktop
bash install-mac-linux.sh            # installieren
bash install-mac-linux.sh --uninstall # entfernen
```

Unter Linux ggf. mit `sudo` starten (Spotify liegt dort oft in `/usr/share/spotify` oder `/opt/spotify`).
Die Snap-Version von Spotify lässt sich nicht verändern; Flatpak und die normale Version funktionieren.

> **Nach einem Spotify-Update** ist der Mod wieder weg (Spotify überschreibt seine Dateien).
> Einfach den Installer nochmal starten.

**Was der Installer macht:** Er öffnet Spotifys App-Paket `Apps/xpui.spa`, legt `lyrics-translator.js`
hinein und lädt es in `index.html`. Vorher wird eine Sicherung `xpui.spa.bak` angelegt, die der Uninstaller
zurückspielt.

---

## 📱 Android: Lyrics direkt in Spotify

### Was die App kann

- **In Spotify:** Das Lyrics-Fenster erscheint nur, wenn Spotify offen ist, in der Farbe vom Album-Cover.
- **Unter Spotifys eigenen Lyrics:** Öffnest du in Spotify die Lyrics, steht die Übersetzung direkt unter jeder Zeile.
- **Eigener Player im Spotify-Stil:** Cover, Lyrics groß und mitlaufend mit Übersetzung, Fortschrittsbalken,
  ⏮ ⏯ ⏭. Die Musik läuft dabei weiter in Spotify, der Player steuert sie nur. Tipp auf eine Zeile = dorthin springen.

### Installieren

1. Auf dem Handy die neueste **`LyricsTranslator.apk`** herunterladen:
   **Releases → „Lyrics Translator (Android)“** hier im Repo.
   (Die APK wird bei jeder Änderung automatisch von GitHub gebaut. Neue Versionen einfach drüber installieren.)
2. APK öffnen → „Installation aus unbekannten Quellen“ für den Browser erlauben → **Installieren**.
3. App **Lyrics Translator** öffnen und die Berechtigungen erteilen:
   - **Benachrichtigungszugriff** – nur darüber erlaubt Android, zu sehen, welcher Song in Spotify läuft, und ihn zu steuern.
   - **Über anderen Apps einblenden** – damit die Lyrics in Spotify erscheinen können.
   - **Bedienungshilfe** (empfohlen) – damit das Fenster nur in Spotify erscheint und die Übersetzung unter
     Spotifys eigenen Lyrics stehen kann. Einstellungen → Bedienungshilfen → Installierte Apps →
     „Lyrics Translator – in Spotify anzeigen“ → Ein.

   Sind Schalter grau („Eingeschränkte Einstellung“, ab Android 13):
   In der App auf **„App-Info öffnen“** tippen → oben rechts **⋮** → **„Eingeschränkte Einstellungen zulassen“** → nochmal versuchen.
4. Sprache und Anzeige wählen, **Spotify öffnen**, Song abspielen – fertig.

### Bedienung

- Fenster am Titel **nach oben/unten ziehen**
- **⛶** öffnet den Vollbild-Player, **⚙** die Einstellungen
- **—** macht es zu einem kleinen grünen Knopf, Tippen darauf öffnet es wieder
- **✕** blendet es bis zum nächsten Song aus

### Selbst bauen (optional)

```sh
cd android
./gradlew assembleRelease   # → app/build/outputs/apk/release/app-release.apk
```

---

## 🍏 iPhone

iOS erlaubt weder veränderte Spotify-Apps noch Fenster über anderen Apps. Deshalb gibt es für das iPhone eine
Web-App, die erkennt, was in Spotify läuft, und die Lyrics dazu anzeigt:

1. Auf GitHub: **Settings → Pages → Source: „Deploy from a branch“ → Branch `main`, Ordner `/docs`**.
2. Auf dem iPhone in Safari `https://<github-name>.github.io/<repo-name>/` öffnen → Teilen → **„Zum Home-Bildschirm“**.
3. Für die automatische Song-Erkennung einmalig eine App auf <https://developer.spotify.com/dashboard> anlegen,
   als **Redirect URI** die Adresse eintragen, die in der Web-App unter ⚙︎ steht, und die **Client ID** dort einfügen.
   Ohne Login geht auch die manuelle Suche nach Künstler und Titel.

---

## Datenschutz

Alles läuft auf deinem Gerät. Gesendet werden nur Künstler und Titel (an LRCLIB/lyrics.ovh) und die Liedzeilen
(an Google Translate), um Lyrics und Übersetzung zu laden. Die Android-App liest keine Benachrichtigungsinhalte,
sondern nur die Wiedergabe-Infos von Spotify; die Bedienungshilfe liest nur Texte, die Spotify gerade anzeigt.

Hinweis: Das Verändern der Spotify-Desktop-App ist von Spotify nicht offiziell vorgesehen. Der Mod ändert nur die
Anzeige und greift nicht in Konto, Wiedergabe oder Premium-Funktionen ein. Nutzung auf eigene Verantwortung.
