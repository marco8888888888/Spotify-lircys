#!/usr/bin/env bash
# Baut den Lyrics Translator direkt in die Spotify-Desktop-App ein (ohne Spicetify).
#   Installieren: bash install-mac-linux.sh
#   Entfernen:    bash install-mac-linux.sh --uninstall
set -euo pipefail

MARKER='<!-- lyrics-translator -->'
NAME='lyrics-translator.js'
HERE="$(cd "$(dirname "$0")" && pwd)"
SOURCE="$HERE/$NAME"

SPA=""
for c in \
  "${SPOTIFY_APPS_DIR:-}/xpui.spa" \
  "/Applications/Spotify.app/Contents/Resources/Apps/xpui.spa" \
  "$HOME/Applications/Spotify.app/Contents/Resources/Apps/xpui.spa" \
  "/usr/share/spotify/Apps/xpui.spa" \
  "/opt/spotify/Apps/xpui.spa" \
  "/usr/lib/spotify/Apps/xpui.spa" \
  "$HOME/.local/share/spotify-launcher/install/usr/share/spotify/Apps/xpui.spa" \
  "$HOME/.local/share/flatpak/app/com.spotify.Client/x86_64/stable/active/files/extra/share/spotify/Apps/xpui.spa" \
  "/var/lib/flatpak/app/com.spotify.Client/x86_64/stable/active/files/extra/share/spotify/Apps/xpui.spa"; do
  if [ -f "$c" ]; then SPA="$c"; break; fi
done
if [ -z "$SPA" ]; then
  echo "Spotify wurde nicht gefunden. Installiere Spotify von https://www.spotify.com/download" >&2
  echo "oder gib den Apps-Ordner an: SPOTIFY_APPS_DIR=/pfad/zu/Spotify/Apps bash $0" >&2
  exit 1
fi
if [ ! -w "$SPA" ]; then
  echo "Keine Schreibrechte für $SPA – starte das Script mit sudo." >&2
  echo "(Die Snap-Version von Spotify lässt sich nicht verändern.)" >&2
  exit 1
fi
BACKUP="$SPA.bak"
echo "Spotify gefunden: $SPA"

pkill -x Spotify 2>/dev/null || pkill -x spotify 2>/dev/null || true
sleep 1

if [ "${1:-}" = "--uninstall" ]; then
  if [ -f "$BACKUP" ]; then
    mv -f "$BACKUP" "$SPA"
    echo "Lyrics Translator wurde entfernt."
  else
    echo "Keine Sicherung gefunden – der Mod ist vermutlich nicht installiert."
  fi
  exit 0
fi

[ -f "$SOURCE" ] || { echo "$NAME fehlt neben diesem Script." >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if command -v unzip >/dev/null && command -v zip >/dev/null; then
  unzip -p "$SPA" index.html > "$WORK/index.html"
  # Sicherung nur vom Original anlegen. Nach einem Spotify-Update ist xpui.spa wieder original -> neue Sicherung.
  grep -qF "$MARKER" "$WORK/index.html" || cp -f "$SPA" "$BACKUP"
  if ! grep -qF "$MARKER" "$WORK/index.html"; then
    # Direkt nach <head>, damit das Script vor Spotifys eigenem Code geladen wird.
    perl -0pi -e "s|(<head[^>]*>)|\$1$MARKER<script src=\"$NAME\"></script>|" "$WORK/index.html"
  fi
  cp "$SOURCE" "$WORK/$NAME"
  (cd "$WORK" && zip -q "$SPA" index.html "$NAME")
elif command -v python3 >/dev/null; then
  python3 - "$SPA" "$BACKUP" "$SOURCE" "$MARKER" "$NAME" <<'PY'
import re, shutil, sys, zipfile
spa, backup, source, marker, name = sys.argv[1:]
with zipfile.ZipFile(spa) as z:
    entries = {i.filename: z.read(i.filename) for i in z.infolist()}
html = entries["index.html"].decode("utf-8")
if marker not in html:
    shutil.copyfile(spa, backup)
    html = re.sub(r"(<head[^>]*>)", lambda m: m.group(1) + marker + f'<script src="{name}"></script>', html, count=1)
entries["index.html"] = html.encode("utf-8")
entries[name] = open(source, "rb").read()
with zipfile.ZipFile(spa, "w", zipfile.ZIP_DEFLATED) as z:
    for n, data in entries.items():
        z.writestr(n, data)
PY
else
  echo "Bitte 'zip' und 'unzip' (oder python3) installieren." >&2
  exit 1
fi

echo
echo "Fertig! Lyrics Translator ist in Spotify eingebaut."
echo "Unten rechts neben dem Lyrics-Knopf ist jetzt ein Übersetzen-Knopf (oder Strg+Umschalt+L)."
echo "Nach einem Spotify-Update einfach dieses Script nochmal starten."
