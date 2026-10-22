package com.marco.lyricstranslator;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Sucht Lyrics bei LRCLIB (synchron) und lyrics.ovh (Rückfallebene). */
final class Lyrics {
    static final class Line {
        final long timeMs; // -1 = nicht synchron
        final String text;

        Line(long timeMs, String text) {
            this.timeMs = timeMs;
            this.text = text;
        }
    }

    final boolean synced;
    final String source;
    final List<Line> lines;

    private Lyrics(boolean synced, String source, List<Line> lines) {
        this.synced = synced;
        this.source = source;
        this.lines = lines;
    }

    private static final Pattern STAMP = Pattern.compile("\\[(\\d+):(\\d+(?:\\.\\d+)?)\\]");

    static List<Line> parseLrc(String lrc) {
        List<Line> lines = new ArrayList<>();
        for (String raw : lrc.split("\\r?\\n")) {
            Matcher m = STAMP.matcher(raw);
            List<Long> times = new ArrayList<>();
            while (m.find()) {
                times.add(Math.round((Long.parseLong(m.group(1)) * 60 + Double.parseDouble(m.group(2))) * 1000));
            }
            if (times.isEmpty()) continue;
            String text = STAMP.matcher(raw).replaceAll("").trim();
            for (long t : times) lines.add(new Line(t, text));
        }
        Collections.sort(lines, (a, b) -> Long.compare(a.timeMs, b.timeMs));
        return lines;
    }

    static List<Line> plainToLines(String text) {
        List<Line> lines = new ArrayList<>();
        for (String t : text.split("\\r?\\n")) lines.add(new Line(-1, t.trim()));
        return lines;
    }

    private static Lyrics fromLrclibJson(JSONObject r) {
        if (r == null) return null;
        String synced = r.optString("syncedLyrics", "");
        if (!synced.isEmpty() && !"null".equals(synced)) return new Lyrics(true, "LRCLIB", parseLrc(synced));
        String plain = r.optString("plainLyrics", "");
        if (!plain.isEmpty() && !"null".equals(plain)) return new Lyrics(false, "LRCLIB", plainToLines(plain));
        return null;
    }

    private static Lyrics fromLrclib(String title, String artist, String album, long durationMs) throws Exception {
        String base = "https://lrclib.net/api";
        if (!album.isEmpty() && durationMs > 0) {
            String body = Http.get(base + "/get?artist_name=" + Http.enc(artist) + "&track_name=" + Http.enc(title)
                    + "&album_name=" + Http.enc(album) + "&duration=" + Math.round(durationMs / 1000.0));
            Lyrics hit = body == null ? null : fromLrclibJson(new JSONObject(body));
            if (hit != null) return hit;
        }
        // Unscharfe Suche: Titel ohne Zusätze wie "(feat. …)" oder "- Remastered".
        String clean = title.replaceAll("\\s*[(\\[].*?[)\\]]\\s*", " ").replaceAll("\\s+-\\s+.*$", "").trim();
        String body = Http.get(base + "/search?track_name=" + Http.enc(clean) + "&artist_name=" + Http.enc(artist));
        JSONArray list = body == null ? new JSONArray() : new JSONArray(body);
        if (list.length() == 0) {
            body = Http.get(base + "/search?q=" + Http.enc(artist + " " + clean));
            list = body == null ? new JSONArray() : new JSONArray(body);
        }
        List<JSONObject> items = new ArrayList<>();
        for (int i = 0; i < list.length(); i++) items.add(list.getJSONObject(i));
        double d = durationMs / 1000.0;
        Collections.sort(items, (a, b) -> {
            int s = Boolean.compare(hasSynced(b), hasSynced(a));
            if (s != 0 || durationMs <= 0) return s;
            return Double.compare(Math.abs(a.optDouble("duration", 0) - d), Math.abs(b.optDouble("duration", 0) - d));
        });
        for (JSONObject r : items) {
            Lyrics hit = fromLrclibJson(r);
            if (hit != null) return hit;
        }
        return null;
    }

    private static boolean hasSynced(JSONObject r) {
        String s = r.optString("syncedLyrics", "");
        return !s.isEmpty() && !"null".equals(s);
    }

    private static Lyrics fromLyricsOvh(String title, String artist) throws Exception {
        String body = Http.get("https://api.lyrics.ovh/v1/" + Http.enc(artist).replace("+", "%20") + "/" + Http.enc(title).replace("+", "%20"));
        if (body == null) return null;
        String text = new JSONObject(body).optString("lyrics", "");
        return text.isEmpty() ? null : new Lyrics(false, "lyrics.ovh", plainToLines(text));
    }

    private static boolean hasText(Lyrics l) {
        if (l == null) return false;
        for (Line line : l.lines) if (!line.text.isEmpty()) return true;
        return false;
    }

    /** Blockiert – nicht auf dem UI-Thread aufrufen. */
    static Lyrics find(String title, String artist, String album, long durationMs) {
        Lyrics fallback = null;
        try {
            Lyrics l = fromLrclib(title, artist, album, durationMs);
            if (hasText(l)) {
                if (l.synced) return l;
                fallback = l;
            }
        } catch (Exception e) {
            Log.w("LyricsTranslator", "LRCLIB", e);
        }
        if (fallback != null) return fallback;
        try {
            Lyrics l = fromLyricsOvh(title, artist);
            if (hasText(l)) return l;
        } catch (Exception e) {
            Log.w("LyricsTranslator", "lyrics.ovh", e);
        }
        return null;
    }
}
