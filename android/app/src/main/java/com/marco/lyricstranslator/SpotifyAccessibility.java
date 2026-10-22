package com.marco.lyricstranslator;

import android.accessibilityservice.AccessibilityService;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Optionale Bedienungshilfe. Sie erkennt, ob Spotify gerade im Vordergrund ist (dann erscheint das Fenster nur dort)
 * und wo Spotifys eigene Lyrics-Zeilen auf dem Bildschirm stehen (für die Übersetzung direkt darunter).
 * Sie liest nur Texte aus Spotify und verändert nichts.
 */
public class SpotifyAccessibility extends AccessibilityService {
    private final Handler main = new Handler(Looper.getMainLooper());
    private Overlay overlay;
    private NowPlaying now;
    private boolean spotifyInFront;
    private boolean scanQueued;

    private final Runnable scan = new Runnable() {
        @Override
        public void run() {
            scanQueued = false;
            scanSpotifyLines();
            // Spotify scrollt die Lyrics beim Mitsingen selbst – solange Spotify vorne ist, regelmäßig nachsehen.
            if (spotifyInFront && Prefs.inline(SpotifyAccessibility.this)) queueScan(400);
        }
    };

    @Override
    protected void onServiceConnected() {
        overlay = Overlay.get(this);
        now = NowPlaying.get(this);
        overlay.start();
        updateForeground();
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
        int type = event.getEventType();
        if (type == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED || type == AccessibilityEvent.TYPE_WINDOWS_CHANGED) {
            updateForeground();
        }
        if (spotifyInFront && Prefs.inline(this)) queueScan(120);
    }

    @Override
    public void onInterrupt() {}

    @Override
    public boolean onUnbind(android.content.Intent intent) {
        main.removeCallbacks(scan);
        if (overlay != null) overlay.setSpotifyInFront(null);
        return super.onUnbind(intent);
    }

    private void queueScan(long delayMs) {
        if (scanQueued) return;
        scanQueued = true;
        main.postDelayed(scan, delayMs);
    }

    /** Paket der App, die gerade den Bildschirm hat (eigene Overlays und Tastatur zählen nicht). */
    private String foregroundPackage() {
        try {
            for (AccessibilityWindowInfo w : getWindows()) {
                if (w.getType() != AccessibilityWindowInfo.TYPE_APPLICATION || !w.isActive()) continue;
                AccessibilityNodeInfo root = w.getRoot();
                if (root != null && root.getPackageName() != null) return root.getPackageName().toString();
            }
            AccessibilityNodeInfo root = getRootInActiveWindow();
            return root == null || root.getPackageName() == null ? null : root.getPackageName().toString();
        } catch (RuntimeException e) {
            return null;
        }
    }

    private void updateForeground() {
        String pkg = foregroundPackage();
        if (pkg == null) return;
        boolean inFront = SpotifyListener.SPOTIFY.equals(pkg);
        if (inFront == spotifyInFront && overlay != null) {
            overlay.setSpotifyInFront(inFront);
            return;
        }
        spotifyInFront = inFront;
        overlay.setSpotifyInFront(inFront);
        if (inFront && Prefs.inline(this)) queueScan(150);
    }

    static String normalize(CharSequence s) {
        return s.toString().replaceAll("\\s+", " ").trim().toLowerCase(Locale.ROOT);
    }

    private void scanSpotifyLines() {
        if (!spotifyInFront || now.lyrics == null || now.translations == null) {
            overlay.setSpotifyLines(new ArrayList<>());
            return;
        }
        Map<String, Integer> wanted = new HashMap<>();
        for (int i = 0; i < now.lyrics.lines.size(); i++) {
            String t = now.lyrics.lines.get(i).text;
            if (!t.isEmpty()) wanted.putIfAbsent(normalize(t), i);
        }
        List<Overlay.SpotifyLine> found = new ArrayList<>();
        AccessibilityNodeInfo root = null;
        try {
            for (AccessibilityWindowInfo w : getWindows()) {
                if (w.getType() != AccessibilityWindowInfo.TYPE_APPLICATION || !w.isActive()) continue;
                root = w.getRoot();
            }
            if (root == null) root = getRootInActiveWindow();
        } catch (RuntimeException ignored) {
        }
        if (root != null && SpotifyListener.SPOTIFY.contentEquals(String.valueOf(root.getPackageName()))) {
            collect(root, wanted, found, 0);
        }
        overlay.setSpotifyLines(found);
    }

    private void collect(AccessibilityNodeInfo node, Map<String, Integer> wanted, List<Overlay.SpotifyLine> out, int depth) {
        if (node == null || depth > 60 || out.size() > 40) return;
        CharSequence text = node.getText();
        if (text != null && text.length() > 0) {
            Integer index = wanted.get(normalize(text));
            if (index != null) {
                Rect r = new Rect();
                node.getBoundsInScreen(r);
                if (r.width() > 0 && r.height() > 0) out.add(new Overlay.SpotifyLine(index, r));
            }
        }
        for (int i = 0; i < node.getChildCount(); i++) collect(node.getChild(i), wanted, out, depth + 1);
    }
}
