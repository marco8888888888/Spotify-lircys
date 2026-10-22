package com.marco.lyricstranslator;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
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

import java.util.Collections;
import java.util.List;

/**
 * Lyrics über Spotify. Zwei Darstellungen:
 * - Karte: schwebendes Fenster mit mitlaufenden Lyrics (oder kleiner Knopf, wenn minimiert)
 * - Inline: Übersetzung direkt unter den Zeilen von Spotifys eigener Lyrics-Ansicht (braucht Bedienungshilfe)
 * Alle Methoden laufen auf dem UI-Thread.
 */
final class Overlay implements NowPlaying.Listener {
    private static Overlay instance;

    static synchronized Overlay get(Context c) {
        if (instance == null) instance = new Overlay(c.getApplicationContext());
        return instance;
    }

    /** Eine Zeile aus Spotifys Lyrics-Ansicht, gefunden über die Bedienungshilfe. */
    static final class SpotifyLine {
        final int index;
        final Rect bounds;

        SpotifyLine(int index, Rect bounds) {
            this.index = index;
            this.bounds = bounds;
        }
    }

    private final Context ctx;
    private final NowPlaying now;
    private final WindowManager wm;
    private final Handler main = new Handler(Looper.getMainLooper());

    private FrameLayout cardRoot, inlineLayer;
    private LinearLayout card;
    private GradientDrawable cardBg;
    private TextView titleView, subView, bubble;
    private ScrollView scroll;
    private LinearLayout linesBox;
    private WindowManager.LayoutParams cardParams, bubbleParams, inlineParams;

    private boolean started, minimized;
    private String dismissedKey, renderedKey;
    private int activeIndex = -1;
    /** null = keine Bedienungshilfe aktiv, dann wird nicht nach Vordergrund-App gefiltert. */
    private Boolean spotifyInFront;
    private List<SpotifyLine> spotifyLines = Collections.emptyList();

    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            int idx = now.activeIndex();
            if (idx != activeIndex && linesBox != null) {
                activeIndex = idx;
                LyricsRenderer.highlight(linesBox, scroll, idx);
            }
            if (cardRoot != null && cardRoot.isAttachedToWindow()) main.postDelayed(this, 200);
        }
    };

    private Overlay(Context ctx) {
        this.ctx = ctx;
        this.now = NowPlaying.get(ctx);
        this.wm = ctx.getSystemService(WindowManager.class);
    }

    void start() {
        main.post(() -> {
            if (started) return;
            started = true;
            now.addListener(this);
            update();
        });
    }

    @Override
    public void onNowPlayingChanged() {
        if (now.trackKey == null || !now.trackKey.equals(dismissedKey)) dismissedKey = null;
        update();
    }

    /** Von der Bedienungshilfe: ist Spotify gerade die App im Vordergrund? null = Dienst aus. */
    void setSpotifyInFront(Boolean inFront) {
        main.post(() -> {
            spotifyInFront = inFront;
            if (!Boolean.TRUE.equals(inFront)) spotifyLines = Collections.emptyList();
            update();
        });
    }

    /** Von der Bedienungshilfe: sichtbare Zeilen von Spotifys Lyrics-Ansicht mit ihrer Position. */
    void setSpotifyLines(List<SpotifyLine> lines) {
        main.post(() -> {
            spotifyLines = lines;
            update();
        });
    }

    void refreshSettings() {
        main.post(() -> {
            dismissedKey = null;
            renderedKey = null;
            update();
        });
    }

    // ---------- Sichtbarkeit ----------

    private boolean shouldShow() {
        if (now.trackKey == null || now.trackKey.equals(dismissedKey)) return false;
        if (!Prefs.enabled(ctx) || !Settings.canDrawOverlays(ctx)) return false;
        if (spotifyInFront != null && !spotifyInFront) return false;
        return now.isActive();
    }

    /** Inline-Modus greift nur, wenn Spotifys Lyrics-Ansicht wirklich offen ist und Übersetzungen da sind. */
    private boolean inlineActive() {
        return Prefs.inline(ctx) && Boolean.TRUE.equals(spotifyInFront) && spotifyLines.size() >= 2 && now.translations != null;
    }

    private void update() {
        ensureViews();
        boolean show = shouldShow();
        boolean inline = show && inlineActive();
        setAttached(inlineLayer, inlineParams, inline);
        setAttached(cardRoot, cardParams, show && !inline && !minimized);
        setAttached(bubble, bubbleParams, show && !inline && minimized);
        if (inline) renderInline();
        if (show && !inline && !minimized) renderCard();
    }

    private void setAttached(View v, WindowManager.LayoutParams p, boolean attach) {
        try {
            if (attach && !v.isAttachedToWindow() && v.getParent() == null) wm.addView(v, p);
            else if (!attach && v.isAttachedToWindow()) wm.removeView(v);
        } catch (RuntimeException e) {
            // z. B. wenn die Overlay-Berechtigung gerade entzogen wurde
        }
    }

    // ---------- Karte ----------

    private void renderCard() {
        cardBg.setColor(blend(now.color, 0xFF121212, 0.35f) & 0x00FFFFFF | 0xF2000000);
        titleView.setText(now.title == null ? "Lyrics Translator" : now.title + " – " + now.artist);
        subView.setText(now.subline());
        String key = now.trackKey + "|" + (now.lyrics != null) + "|" + (now.translations != null) + "|" + Prefs.mode(ctx);
        if (!key.equals(renderedKey)) {
            renderedKey = key;
            LyricsRenderer.render(ctx, linesBox, now, 17, 14, null);
            activeIndex = -2;
        }
        main.removeCallbacks(tick);
        main.post(tick);
    }

    // ---------- Inline unter Spotifys Zeilen ----------

    private void renderInline() {
        inlineLayer.removeAllViews();
        int[] origin = new int[2];
        inlineLayer.getLocationOnScreen(origin);
        int bg = blend(now.color, 0xFF000000, 0.25f) & 0x00FFFFFF | 0xE6000000;
        for (SpotifyLine l : spotifyLines) {
            String trans = now.translationAt(l.index);
            if (trans.isEmpty()) continue;
            TextView t = new TextView(ctx);
            t.setText(trans);
            t.setTextSize(14);
            t.setTextColor(LyricsRenderer.GREEN);
            t.setTypeface(Typeface.DEFAULT_BOLD);
            t.setMaxLines(2);
            t.setEllipsize(TextUtils.TruncateAt.END);
            GradientDrawable d = new GradientDrawable();
            d.setColor(bg);
            d.setCornerRadius(dp(6));
            t.setBackground(d);
            t.setPadding(dp(6), dp(1), dp(6), dp(2));
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
            lp.leftMargin = l.bounds.left - origin[0] - dp(6);
            lp.topMargin = l.bounds.bottom - origin[1] - dp(4);
            lp.rightMargin = 0;
            t.setMaxWidth(Math.max(dp(120), l.bounds.width() + dp(12)));
            inlineLayer.addView(t, lp);
        }
    }

    // ---------- Fenster bauen ----------

    private int dp(float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, ctx.getResources().getDisplayMetrics()));
    }

    static int blend(int a, int b, float t) {
        int r = Math.round(((a >> 16) & 0xFF) * (1 - t) + ((b >> 16) & 0xFF) * t);
        int g = Math.round(((a >> 8) & 0xFF) * (1 - t) + ((b >> 8) & 0xFF) * t);
        int bl = Math.round((a & 0xFF) * (1 - t) + (b & 0xFF) * t);
        return 0xFF000000 | (r << 16) | (g << 8) | bl;
    }

    private static WindowManager.LayoutParams params(int width, int height, int gravity, int extraFlags) {
        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
                width, height,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN | extraFlags,
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
        card = new LinearLayout(ctx);
        card.setOrientation(LinearLayout.VERTICAL);
        cardBg = new GradientDrawable();
        cardBg.setColor(0xF2121212);
        cardBg.setCornerRadius(dp(16));
        card.setBackground(cardBg);
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
        TextView full = button("⛶");
        TextView settings = button("⚙");
        TextView minimize = button("—");
        TextView close = button("✕");
        header.addView(full);
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

        cardParams = params(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.WRAP_CONTENT, Gravity.TOP, 0);
        cardParams.y = y;

        bubble = new TextView(ctx);
        bubble.setText("文A");
        bubble.setTextColor(0xFF000000);
        bubble.setTypeface(Typeface.DEFAULT_BOLD);
        bubble.setTextSize(15);
        bubble.setGravity(Gravity.CENTER);
        GradientDrawable circle = new GradientDrawable();
        circle.setShape(GradientDrawable.OVAL);
        circle.setColor(LyricsRenderer.GREEN);
        bubble.setBackground(circle);
        bubble.setElevation(dp(6));
        bubbleParams = params(dp(52), dp(52), Gravity.TOP | Gravity.END, 0);
        bubbleParams.x = dp(12);
        bubbleParams.y = y;

        // Volle Bildschirmebene, die keine Berührungen abfängt – Spotify bleibt normal bedienbar.
        inlineLayer = new FrameLayout(ctx);
        inlineParams = params(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT, Gravity.TOP | Gravity.START,
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);

        full.setOnClickListener(v -> ctx.startActivity(
                new Intent(ctx, PlayerActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)));
        settings.setOnClickListener(v -> ctx.startActivity(
                new Intent(ctx, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)));
        minimize.setOnClickListener(v -> {
            minimized = true;
            update();
        });
        close.setOnClickListener(v -> {
            // Bis zum nächsten Song ausblenden.
            dismissedKey = now.trackKey;
            update();
        });
        draggable(header, cardRoot, cardParams, null);
        draggable(titleView, cardRoot, cardParams, null);
        draggable(bubble, bubble, bubbleParams, () -> {
            minimized = false;
            renderedKey = null;
            update();
        });
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
}
