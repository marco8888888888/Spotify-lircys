package com.marco.lyricstranslator;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.media.session.PlaybackState;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Schwebendes Lyrics-Fenster, das über Spotify angezeigt wird. Alle Methoden laufen auf dem UI-Thread. */
final class Overlay {
    private static final int GREEN = 0xFF1ED760;
    private static Overlay instance;

    static synchronized Overlay get(Context c) {
        if (instance == null) instance = new Overlay(c.getApplicationContext());
        return instance;
    }

    private final Context ctx;
    private final WindowManager wm;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private final Map<String, Translator.Result> cache = new LinkedHashMap<String, Translator.Result>(16, 0.75f, true) {
        @Override
        protected boolean removeEldestEntry(Map.Entry<String, Translator.Result> e) {
            return size() > 30;
        }
    };

    private FrameLayout cardRoot;
    private TextView titleView, subView, bubble;
    private ScrollView scroll;
    private LinearLayout linesBox;
    private WindowManager.LayoutParams cardParams, bubbleParams;

    private boolean minimized;
    private String dismissedKey;
    private String trackKey, title, artist;
    private Lyrics lyrics;
    private String[] translations;
    private String detected;
    private int requestId;
    private int activeIndex = -1;
    private PlaybackState playback;

    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            updateActiveLine();
            if (cardRoot != null && cardRoot.isAttachedToWindow()) main.postDelayed(this, 200);
        }
    };

    private Overlay(Context ctx) {
        this.ctx = ctx;
        this.wm = ctx.getSystemService(WindowManager.class);
    }

    // ---------- Eingänge ----------

    void onTrack(String title, String artist, String album, long durationMs) {
        main.post(() -> {
            String key = title == null ? null : artist + "|" + title;
            if (key != null && key.equals(trackKey)) return;
            trackKey = key;
            this.title = title;
            this.artist = artist;
            lyrics = null;
            translations = null;
            detected = null;
            int req = ++requestId;
            updateVisibility();
            if (key == null) return;
            setHeader("Suche Lyrics…");
            renderLines();
            io.execute(() -> {
                Lyrics found = Lyrics.find(title, artist, album, durationMs);
                main.post(() -> {
                    if (req != requestId) return;
                    lyrics = found;
                    if (found == null) {
                        setHeader("Keine Lyrics gefunden");
                        renderLines();
                    } else {
                        translate();
                    }
                });
            });
        });
    }

    void onPlayback(PlaybackState state) {
        main.post(() -> {
            playback = state;
            updateVisibility();
            activeIndex = -1;
            updateActiveLine();
        });
    }

    /** Nach Änderungen in den Einstellungen aufrufen. */
    void refreshSettings() {
        main.post(() -> {
            dismissedKey = null;
            updateVisibility();
            if (lyrics != null) translate();
        });
    }

    // ---------- Übersetzung ----------

    private void translate() {
        translations = null;
        detected = null;
        String mode = Prefs.mode(ctx);
        String lang = Prefs.lang(ctx);
        renderLines();
        if (lyrics == null || Prefs.MODE_ORIGINAL.equals(mode)) {
            setHeader(null);
            return;
        }
        String key = trackKey + "|" + lang;
        Translator.Result cached = cache.get(key);
        if (cached != null) {
            applyTranslation(cached, lang);
            return;
        }
        setHeader("übersetze…");
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
                    if (req == requestId) setHeader("Übersetzung fehlgeschlagen – Internet?");
                });
            }
        });
    }

    private void applyTranslation(Translator.Result r, String lang) {
        detected = r.detected;
        boolean already = detected != null && detected.split("-")[0].equals(lang.split("-")[0]);
        translations = already ? null : r.lines;
        setHeader(detected == null ? null
                : Prefs.langName(detected) + " → " + Prefs.langName(lang) + (already ? " (schon in dieser Sprache)" : ""));
        renderLines();
    }

    // ---------- Fenster ----------

    private boolean shouldShow() {
        if (trackKey == null || trackKey.equals(dismissedKey)) return false;
        if (!Prefs.enabled(ctx) || !Settings.canDrawOverlays(ctx)) return false;
        if (playback == null) return false;
        int s = playback.getState();
        return s == PlaybackState.STATE_PLAYING || s == PlaybackState.STATE_PAUSED || s == PlaybackState.STATE_BUFFERING;
    }

    private void updateVisibility() {
        boolean show = shouldShow();
        ensureViews();
        setAttached(cardRoot, cardParams, show && !minimized);
        setAttached(bubble, bubbleParams, show && minimized);
        if (show && !minimized) {
            main.removeCallbacks(tick);
            main.post(tick);
        }
    }

    private void setAttached(View v, WindowManager.LayoutParams p, boolean attach) {
        try {
            if (attach && !v.isAttachedToWindow() && v.getParent() == null) wm.addView(v, p);
            else if (!attach && v.isAttachedToWindow()) wm.removeView(v);
        } catch (RuntimeException e) {
            // z. B. wenn die Overlay-Berechtigung gerade entzogen wurde
        }
    }

    private int dp(float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, ctx.getResources().getDisplayMetrics()));
    }

    private static WindowManager.LayoutParams params(int width, int gravity) {
        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
                width, WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT);
        p.gravity = gravity;
        return p;
    }

    private TextView button(String label) {
        TextView b = new TextView(ctx);
        b.setText(label);
        b.setTextColor(0xFFB3B3B3);
        b.setTextSize(18);
        b.setPadding(dp(10), dp(2), dp(4), dp(2));
        return b;
    }

    private void ensureViews() {
        if (cardRoot != null) return;
        int y = Prefs.overlayY(ctx);

        cardRoot = new FrameLayout(ctx);
        cardRoot.setPadding(dp(8), 0, dp(8), 0);
        LinearLayout card = new LinearLayout(ctx);
        card.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(0xF2121212);
        bg.setCornerRadius(dp(16));
        card.setBackground(bg);
        card.setPadding(dp(14), dp(10), dp(10), dp(10));
        cardRoot.addView(card);

        LinearLayout header = new LinearLayout(ctx);
        header.setGravity(Gravity.CENTER_VERTICAL);
        titleView = new TextView(ctx);
        titleView.setTextColor(0xFFFFFFFF);
        titleView.setTypeface(Typeface.DEFAULT_BOLD);
        titleView.setTextSize(13);
        titleView.setSingleLine(true);
        titleView.setEllipsize(TextUtils.TruncateAt.END);
        header.addView(titleView, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        TextView settings = button("⚙");
        TextView minimize = button("—");
        TextView close = button("✕");
        header.addView(settings);
        header.addView(minimize);
        header.addView(close);
        card.addView(header);

        subView = new TextView(ctx);
        subView.setTextColor(0xFFB3B3B3);
        subView.setTextSize(11);
        subView.setSingleLine(true);
        subView.setEllipsize(TextUtils.TruncateAt.END);
        card.addView(subView);

        scroll = new ScrollView(ctx);
        scroll.setVerticalScrollBarEnabled(false);
        linesBox = new LinearLayout(ctx);
        linesBox.setOrientation(LinearLayout.VERTICAL);
        linesBox.setPadding(0, dp(6), 0, dp(80));
        scroll.addView(linesBox);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(170));
        sp.topMargin = dp(4);
        card.addView(scroll, sp);

        cardParams = params(WindowManager.LayoutParams.MATCH_PARENT, Gravity.TOP);
        cardParams.y = y;

        bubble = new TextView(ctx);
        bubble.setText("文A");
        bubble.setTextColor(0xFF000000);
        bubble.setTypeface(Typeface.DEFAULT_BOLD);
        bubble.setTextSize(15);
        bubble.setGravity(Gravity.CENTER);
        GradientDrawable circle = new GradientDrawable();
        circle.setShape(GradientDrawable.OVAL);
        circle.setColor(GREEN);
        bubble.setBackground(circle);
        bubble.setElevation(dp(6));
        bubbleParams = params(dp(52), Gravity.TOP | Gravity.END);
        bubbleParams.height = dp(52);
        bubbleParams.x = dp(12);
        bubbleParams.y = y;

        settings.setOnClickListener(v -> ctx.startActivity(
                new Intent(ctx, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)));
        minimize.setOnClickListener(v -> {
            minimized = true;
            updateVisibility();
        });
        close.setOnClickListener(v -> {
            // Bis zum nächsten Song ausblenden.
            dismissedKey = trackKey;
            updateVisibility();
        });
        draggable(header, cardRoot, cardParams, null);
        draggable(titleView, cardRoot, cardParams, null);
        draggable(bubble, bubble, bubbleParams, () -> {
            minimized = false;
            updateVisibility();
        });
        setHeader(null);
    }

    /** Fenster am Element vertikal verschiebbar machen; ein kurzer Tipp löst onTap aus. */
    @SuppressLint("ClickableViewAccessibility")
    private void draggable(View handle, View window, WindowManager.LayoutParams p, Runnable onTap) {
        handle.setOnTouchListener(new DragListener(window, p, onTap));
    }

    private final class DragListener implements View.OnTouchListener {
        private final View window;
        private final WindowManager.LayoutParams p;
        private final Runnable onTap;
        private final int slop = ViewConfiguration.get(ctx).getScaledTouchSlop();
        private float startRaw;
        private int startY;
        private boolean dragging;

        DragListener(View window, WindowManager.LayoutParams p, Runnable onTap) {
            this.window = window;
            this.p = p;
            this.onTap = onTap;
        }

        @Override
        public boolean onTouch(View v, MotionEvent e) {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    startRaw = e.getRawY();
                    startY = p.y;
                    dragging = false;
                    return true;
                case MotionEvent.ACTION_MOVE:
                    float dy = e.getRawY() - startRaw;
                    if (Math.abs(dy) > slop) dragging = true;
                    if (dragging && window.isAttachedToWindow()) {
                        p.y = Math.max(0, startY + Math.round(dy));
                        wm.updateViewLayout(window, p);
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    if (dragging) {
                        cardParams.y = bubbleParams.y = p.y;
                        Prefs.setOverlayY(ctx, p.y);
                    } else if (onTap != null) {
                        onTap.run();
                    }
                    return true;
                default:
                    return false;
            }
        }
    }

    private void setHeader(String status) {
        if (titleView == null) return;
        titleView.setText(title == null ? "Lyrics Translator" : title + " – " + artist);
        StringBuilder sub = new StringBuilder();
        if (lyrics != null) sub.append(lyrics.source).append(lyrics.synced ? " · synchron" : "");
        if (status != null) {
            if (sub.length() > 0) sub.append(" · ");
            sub.append(status);
        }
        subView.setText(sub);
    }

    private void renderLines() {
        if (linesBox == null) return;
        linesBox.removeAllViews();
        activeIndex = -1;
        if (lyrics == null) return;
        String mode = Prefs.mode(ctx);
        for (int i = 0; i < lyrics.lines.size(); i++) {
            Lyrics.Line line = lyrics.lines.get(i);
            LinearLayout row = new LinearLayout(ctx);
            row.setOrientation(LinearLayout.VERTICAL);
            row.setPadding(0, dp(4), 0, dp(4));
            if (line.text.isEmpty()) {
                row.setMinimumHeight(dp(10));
                linesBox.addView(row);
                continue;
            }
            String trans = translations == null ? "" : translations[i];
            boolean hasTrans = !trans.isEmpty() && !trans.equals(line.text);
            if (!Prefs.MODE_TRANSLATION.equals(mode) || !hasTrans) row.addView(lineText(line.text, false));
            if (!Prefs.MODE_ORIGINAL.equals(mode) && hasTrans) {
                row.addView(lineText(trans, !Prefs.MODE_TRANSLATION.equals(mode)));
            }
            row.setAlpha(lyrics.synced && playback != null ? 0.45f : 1f);
            linesBox.addView(row);
        }
        updateActiveLine();
    }

    private TextView lineText(String text, boolean translation) {
        TextView t = new TextView(ctx);
        t.setText(text);
        t.setTextColor(translation ? GREEN : 0xFFFFFFFF);
        t.setTextSize(translation ? 14 : 17);
        if (!translation) t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private long positionMs() {
        if (playback == null) return 0;
        long p = playback.getPosition();
        if (playback.getState() == PlaybackState.STATE_PLAYING) {
            p += (long) ((SystemClock.elapsedRealtime() - playback.getLastPositionUpdateTime()) * playback.getPlaybackSpeed());
        }
        return p;
    }

    private void updateActiveLine() {
        if (lyrics == null || !lyrics.synced || playback == null || linesBox == null || minimized) return;
        long pos = positionMs() + 250;
        int idx = -1;
        for (int i = 0; i < lyrics.lines.size(); i++) {
            if (lyrics.lines.get(i).timeMs <= pos) idx = i;
            else break;
        }
        if (idx == activeIndex) return;
        activeIndex = idx;
        for (int i = 0; i < linesBox.getChildCount(); i++) {
            linesBox.getChildAt(i).setAlpha(i == idx ? 1f : i < idx ? 0.7f : 0.45f);
        }
        if (idx >= 0 && idx < linesBox.getChildCount()) {
            View row = linesBox.getChildAt(idx);
            scroll.post(() -> scroll.smoothScrollTo(0, Math.max(0, row.getTop() - scroll.getHeight() / 3)));
        }
    }
}
