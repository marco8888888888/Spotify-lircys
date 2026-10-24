package com.marco.lyricstranslator;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.PlaybackState;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Gemeinsamer Zustand: welcher Song läuft, Position, Lyrics und Übersetzung.
 * Overlay und Player hören hier zu. Alle Methoden laufen auf dem UI-Thread.
 */
final class NowPlaying {
    interface Listener {
        void onNowPlayingChanged();
    }

    static final int DEFAULT_COLOR = 0xFF2A2A2A;
    private static NowPlaying instance;

    static synchronized NowPlaying get(Context c) {
        if (instance == null) instance = new NowPlaying(c.getApplicationContext());
        return instance;
    }

    private final Context ctx;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private final List<Listener> listeners = new ArrayList<>();
    private final Map<String, Translator.Result> cache = new LinkedHashMap<String, Translator.Result>(16, 0.75f, true) {
        @Override
        protected boolean removeEldestEntry(Map.Entry<String, Translator.Result> e) {
            return size() > 30;
        }
    };

    MediaController controller;
    String trackKey, title, artist;
    Bitmap art;
    int color = DEFAULT_COLOR;
    long durationMs;
    Lyrics lyrics;
    String[] translations;
    String status;
    PlaybackState playback;
    private int requestId;

    private NowPlaying(Context ctx) {
        this.ctx = ctx;
    }

    void addListener(Listener l) {
        if (!listeners.contains(l)) listeners.add(l);
    }

    void removeListener(Listener l) {
        listeners.remove(l);
    }

    private void changed() {
        for (Listener l : new ArrayList<>(listeners)) l.onNowPlayingChanged();
    }

    // ---------- Eingänge vom SpotifyListener ----------

    void setController(MediaController c) {
        main.post(() -> {
            controller = c;
            if (c == null) {
                playback = null;
                onMetadataNow(null);
            } else {
                playback = c.getPlaybackState();
                onMetadataNow(c.getMetadata());
            }
        });
    }

    void onMetadata(MediaMetadata m) {
        main.post(() -> onMetadataNow(m));
    }

    void onPlayback(PlaybackState s) {
        main.post(() -> {
            playback = s;
            changed();
        });
    }

    private void onMetadataNow(MediaMetadata m) {
        String t = m == null ? null : m.getString(MediaMetadata.METADATA_KEY_TITLE);
        String a = m == null ? null : m.getString(MediaMetadata.METADATA_KEY_ARTIST);
        if (t == null || a == null) {
            if (m != null) return; // unvollständige Zwischenstände ignorieren
            t = null;
            a = null;
        }
        Bitmap newArt = m == null ? null : m.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART);
        if (newArt == null && m != null) newArt = m.getBitmap(MediaMetadata.METADATA_KEY_ART);
        String key = t == null ? null : a + "|" + t;
        if (newArt != null && newArt != art) {
            art = newArt;
            color = dominantColor(newArt);
        }
        if (key != null && key.equals(trackKey)) {
            changed();
            return;
        }
        trackKey = key;
        title = t;
        artist = a == null ? null : a.split(",")[0].trim();
        durationMs = m == null ? 0 : m.getLong(MediaMetadata.METADATA_KEY_DURATION);
        if (key == null) {
            art = null;
            color = DEFAULT_COLOR;
        }
        lyrics = null;
        translations = null;
        status = key == null ? null : "Suche Lyrics…";
        int req = ++requestId;
        changed();
        if (key == null) return;
        String album = m.getString(MediaMetadata.METADATA_KEY_ALBUM);
        String fTitle = title, fArtist = artist;
        long fDuration = durationMs;
        io.execute(() -> {
            Lyrics found = Lyrics.find(fTitle, fArtist, album == null ? "" : album, fDuration);
            main.post(() -> {
                if (req != requestId) return;
                lyrics = found;
                if (found == null) {
                    status = "Keine Lyrics gefunden";
                    changed();
                } else {
                    translate();
                }
            });
        });
    }

    /** Nach Änderungen in den Einstellungen aufrufen. */
    void refreshSettings() {
        main.post(() -> {
            if (lyrics != null) translate();
            else changed();
        });
    }

    // ---------- Übersetzung ----------

    private void translate() {
        translations = null;
        String lang = Prefs.lang(ctx);
        if (lyrics == null || Prefs.MODE_ORIGINAL.equals(Prefs.mode(ctx))) {
            status = null;
            changed();
            return;
        }
        String key = trackKey + "|" + lang;
        Translator.Result cached = cache.get(key);
        if (cached != null) {
            applyTranslation(cached, lang);
            return;
        }
        status = "übersetze…";
        changed();
        int req = requestId;
        Lyrics l = lyrics;
        io.execute(() -> {
            try {
                Translator.Result r = Translator.translate(l.lines, lang);
                main.post(() -> {
                    cache.put(key, r);
                    if (req == requestId && lang.equals(Prefs.lang(ctx))) applyTranslation(r, lang);
                });
            } catch (Exception e) {
                main.post(() -> {
                    if (req != requestId) return;
                    status = "Übersetzung fehlgeschlagen – Internet?";
                    changed();
                });
            }
        });
    }

    private void applyTranslation(Translator.Result r, String lang) {
        boolean already = r.detected != null && r.detected.split("-")[0].equals(lang.split("-")[0]);
        translations = already ? null : r.lines;
        status = r.detected == null ? null
                : Prefs.langName(r.detected) + " → " + Prefs.langName(lang) + (already ? " (schon in dieser Sprache)" : "");
        changed();
    }

    // ---------- Abfragen & Steuerung ----------

    /** Zeile mit Quelle und Status, z. B. „LRCLIB · synchron · Englisch → Deutsch“. */
    String subline() {
        StringBuilder sub = new StringBuilder();
        if (lyrics != null) sub.append(lyrics.source).append(lyrics.synced ? " · synchron" : "");
        if (status != null) {
            if (sub.length() > 0) sub.append(" · ");
            sub.append(status);
        }
        return sub.toString();
    }

    boolean isActive() {
        if (playback == null) return false;
        int s = playback.getState();
        return s == PlaybackState.STATE_PLAYING || s == PlaybackState.STATE_PAUSED || s == PlaybackState.STATE_BUFFERING;
    }

    boolean isPlaying() {
        return playback != null && playback.getState() == PlaybackState.STATE_PLAYING;
    }

    long positionMs() {
        if (playback == null) return 0;
        long p = playback.getPosition();
        if (isPlaying()) {
            p += (long) ((SystemClock.elapsedRealtime() - playback.getLastPositionUpdateTime()) * playback.getPlaybackSpeed());
        }
        return durationMs > 0 ? Math.min(p, durationMs) : p;
    }

    /** Index der gerade gesungenen Zeile, -1 vor der ersten. */
    int activeIndex() {
        if (lyrics == null || !lyrics.synced) return -1;
        long pos = positionMs() + 250;
        int idx = -1;
        for (int i = 0; i < lyrics.lines.size(); i++) {
            if (lyrics.lines.get(i).timeMs <= pos) idx = i;
            else break;
        }
        return idx;
    }

    String translationAt(int i) {
        if (translations == null || i < 0 || i >= translations.length) return "";
        String t = translations[i];
        return t.equals(lyrics.lines.get(i).text) ? "" : t;
    }

    void playPause() {
        if (controller == null) return;
        if (isPlaying()) controller.getTransportControls().pause();
        else controller.getTransportControls().play();
    }

    void next() {
        if (controller != null) controller.getTransportControls().skipToNext();
    }

    void previous() {
        if (controller != null) controller.getTransportControls().skipToPrevious();
    }

    void seekTo(long ms) {
        if (controller != null) controller.getTransportControls().seekTo(Math.max(0, ms));
    }

    /** Kräftige, aber dunkle Farbe aus dem Album-Cover – wie der farbige Hintergrund bei Spotify. */
    static int dominantColor(Bitmap b) {
        Bitmap s = Bitmap.createScaledBitmap(b, 24, 24, true);
        float[] hsv = new float[3];
        double[] sum = new double[3];
        double weightSum = 0;
        double sinH = 0, cosH = 0;
        for (int x = 0; x < s.getWidth(); x++) {
            for (int y = 0; y < s.getHeight(); y++) {
                Color.colorToHSV(s.getPixel(x, y), hsv);
                double w = hsv[1] * hsv[2] + 0.02;
                sinH += Math.sin(Math.toRadians(hsv[0])) * w;
                cosH += Math.cos(Math.toRadians(hsv[0])) * w;
                sum[1] += hsv[1] * w;
                sum[2] += hsv[2] * w;
                weightSum += w;
            }
        }
        if (s != b) s.recycle();
        if (weightSum == 0) return DEFAULT_COLOR;
        float h = (float) ((Math.toDegrees(Math.atan2(sinH, cosH)) + 360) % 360);
        float sat = Math.min(0.75f, (float) (sum[1] / weightSum) * 1.2f);
        float val = Math.min(0.42f, Math.max(0.22f, (float) (sum[2] / weightSum)));
        return Color.HSVToColor(new float[] {h, sat, val});
    }
}
