package com.marco.lyricstranslator;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
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
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.Collections;
import java.util.List;

/**
 * Lyrics direkt über Spotify. Zwei Darstellungen:
 * - Karte im Stil von Spotifys Lyrics-Karte (Farbe vom Cover), oder kleiner Knopf, wenn minimiert
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
    private GradientDrawable cardBg, bubbleBg;
    private ImageView cover;
    private TextView titleView, artistView, subView;
    private View bubble;
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
        setAttached(inlineLayer, inlineParams, inline, false);
        setAttached(cardRoot, cardParams, show && !inline && !minimized, true);
        setAttached(bubble, bubbleParams, show && !inline && minimized, true);
        if (inline) renderInline();
        if (show && !inline && !minimized) renderCard();
        if (bubble.isAttachedToWindow()) bubbleBg.setColors(gradient(now.color));
    }

    private void setAttached(View v, WindowManager.LayoutParams p, boolean attach, boolean animate) {
        try {
            if (attach && !v.isAttachedToWindow() && v.getParent() == null) {
                wm.addView(v, p);
                if (animate) {
                    v.setAlpha(0f);
                    v.setTranslationY(dp(16));
                    v.animate().alpha(1f).translationY(0).setDuration(260).setInterpolator(new DecelerateInterpolator()).start();
                }
            } else if (!attach && v.isAttachedToWindow()) {
                v.animate().cancel();
                wm.removeView(v);
            }
        } catch (RuntimeException e) {
            // z. B. wenn die Overlay-Berechtigung gerade entzogen wurde
        }
    }

    // ---------- Karte ----------

    /** Verlauf wie bei Spotifys Lyrics-Karte: Cover-Farbe oben, dunkler nach unten. */
    private static int[] gradient(int color) {
        return new int[] {blend(color, 0xFFFFFFFF, 0.06f), blend(color, 0xFF000000, 0.38f)};
    }

    private void renderCard() {
        cardBg.setColors(gradient(now.color));
        if (now.art != null) cover.setImageBitmap(now.art);
        else cover.setImageResource(R.drawable.ic_translate);
        titleView.setText(now.title == null ? "Lyrics Translator" : now.title);
        artistView.setText(now.artist == null ? "" : now.artist);
        subView.setText(now.subline());
        subView.setVisibility(subView.getText().length() == 0 ? View.GONE : View.VISIBLE);
        String key = now.trackKey + "|" + (now.lyrics != null) + "|" + (now.translations != null) + "|" + Prefs.mode(ctx);
        if (!key.equals(renderedKey)) {
            renderedKey = key;
            LyricsRenderer.render(ctx, linesBox, now, 19, 15, null);
            if (now.lyrics == null && now.status != null) {
                TextView msg = new TextView(ctx);
                msg.setText(now.status);
                msg.setTextSize(15);
                msg.setTextColor(0xB3FFFFFF);
                msg.setPadding(0, dp(18), 0, 0);
                linesBox.addView(msg);
            }
            activeIndex = -2;
            scroll.scrollTo(0, 0);
        }
        main.removeCallbacks(tick);
        main.post(tick);
    }

    // ---------- Inline unter Spotifys Zeilen ----------

    private void renderInline() {
        inlineLayer.removeAllViews();
        int[] origin = new int[2];
        inlineLayer.getLocationOnScreen(origin);
        for (SpotifyLine l : spotifyLines) {
            String trans = now.translationAt(l.index);
            if (trans.isEmpty()) continue;
            TextView t = new TextView(ctx);
            t.setText(trans);
            t.setTextSize(15);
            t.setTextColor(LyricsRenderer.TRANSLATION);
            t.setTypeface(Typeface.create("sans-serif", Typeface.BOLD));
            t.setMaxLines(2);
            t.setEllipsize(TextUtils.TruncateAt.END);
            GradientDrawable d = new GradientDrawable();
            d.setColor(blend(now.color, 0xFF000000, 0.55f) & 0x00FFFFFF | 0xE6000000);
            d.setCornerRadius(dp(8));
            t.setBackground(d);
            t.setPadding(dp(8), dp(2), dp(8), dp(3));
            t.setElevation(dp(2));
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
            lp.leftMargin = l.bounds.left - origin[0] - dp(8);
            lp.topMargin = l.bounds.bottom - origin[1] - dp(4);
            t.setMaxWidth(Math.max(dp(120), l.bounds.width() + dp(16)));
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

    private ImageView iconButton(int icon, String description) {
        ImageView b = new ImageView(ctx);
        b.setImageResource(icon);
        b.setImageTintList(ColorStateList.valueOf(0xD9FFFFFF));
        b.setContentDescription(description);
        b.setPadding(dp(7), dp(7), dp(7), dp(7));
        GradientDrawable mask = new GradientDrawable();
        mask.setShape(GradientDrawable.OVAL);
        mask.setColor(0xFFFFFFFF);
        b.setBackground(new RippleDrawable(ColorStateList.valueOf(0x40FFFFFF), null, mask));
        return b;
    }

    private void ensureViews() {
        if (cardRoot != null) return;
        int y = Prefs.overlayBottom(ctx, dp(140));

        // Außen etwas Rand, damit der Schatten der Karte sichtbar bleibt.
        cardRoot = new FrameLayout(ctx);
        cardRoot.setPadding(dp(10), dp(6), dp(10), dp(10));
        cardRoot.setClipToPadding(false);
        LinearLayout card = new LinearLayout(ctx);
        card.setOrientation(LinearLayout.VERTICAL);
        cardBg = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, gradient(NowPlaying.DEFAULT_COLOR));
        cardBg.setCornerRadius(dp(14));
        card.setBackground(cardBg);
        card.setElevation(dp(8));
        card.setPadding(dp(14), dp(12), dp(8), dp(10));
        cardRoot.addView(card);

        // Kopf: Cover, Titel/Künstler, Knöpfe
        LinearLayout header = new LinearLayout(ctx);
        header.setGravity(Gravity.CENTER_VERTICAL);
        cover = new ImageView(ctx);
        cover.setScaleType(ImageView.ScaleType.CENTER_CROP);
        GradientDrawable coverShape = new GradientDrawable();
        coverShape.setColor(0x33FFFFFF);
        coverShape.setCornerRadius(dp(6));
        cover.setBackground(coverShape);
        cover.setClipToOutline(true);
        header.addView(cover, new LinearLayout.LayoutParams(dp(38), dp(38)));

        LinearLayout names = new LinearLayout(ctx);
        names.setOrientation(LinearLayout.VERTICAL);
        names.setPadding(dp(10), 0, dp(4), 0);
        titleView = new TextView(ctx);
        titleView.setTextColor(0xFFFFFFFF);
        titleView.setTypeface(Typeface.create("sans-serif", Typeface.BOLD));
        titleView.setTextSize(14);
        titleView.setSingleLine(true);
        titleView.setEllipsize(TextUtils.TruncateAt.END);
        artistView = new TextView(ctx);
        artistView.setTextColor(0xB3FFFFFF);
        artistView.setTextSize(12);
        artistView.setSingleLine(true);
        artistView.setEllipsize(TextUtils.TruncateAt.END);
        names.addView(titleView);
        names.addView(artistView);
        header.addView(names, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));

        int size = dp(34);
        ImageView full = iconButton(R.drawable.ic_fullscreen, "Vollbild");
        ImageView settings = iconButton(R.drawable.ic_tune, "Einstellungen");
        ImageView minimize = iconButton(R.drawable.ic_minimize, "Minimieren");
        ImageView close = iconButton(R.drawable.ic_close, "Schließen");
        for (View v : new View[] {full, settings, minimize, close}) header.addView(v, new LinearLayout.LayoutParams(size, size));
        card.addView(header);

        // Lyrics mit weichen Kanten oben/unten
        scroll = new ScrollView(ctx);
        scroll.setVerticalScrollBarEnabled(false);
        scroll.setVerticalFadingEdgeEnabled(true);
        scroll.setFadingEdgeLength(dp(28));
        scroll.setOverScrollMode(View.OVER_SCROLL_NEVER);
        linesBox = new LinearLayout(ctx);
        linesBox.setOrientation(LinearLayout.VERTICAL);
        linesBox.setPadding(0, dp(14), dp(6), dp(90));
        scroll.addView(linesBox);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(196));
        sp.topMargin = dp(2);
        card.addView(scroll, sp);

        subView = new TextView(ctx);
        subView.setTextColor(0x8CFFFFFF);
        subView.setTextSize(10.5f);
        subView.setSingleLine(true);
        subView.setEllipsize(TextUtils.TruncateAt.END);
        subView.setPadding(0, dp(4), dp(6), 0);
        card.addView(subView);

        cardParams = params(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM, 0);
        cardParams.y = y;

        // Minimiert: runder Knopf in der Cover-Farbe
        ImageView b = new ImageView(ctx);
        b.setImageResource(R.drawable.ic_translate);
        b.setImageTintList(ColorStateList.valueOf(0xFFFFFFFF));
        b.setPadding(dp(14), dp(14), dp(14), dp(14));
        bubbleBg = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, gradient(NowPlaying.DEFAULT_COLOR));
        bubbleBg.setShape(GradientDrawable.OVAL);
        bubbleBg.setStroke(dp(2), LyricsRenderer.GREEN);
        b.setBackground(bubbleBg);
        b.setElevation(dp(6));
        FrameLayout bubbleRoot = new FrameLayout(ctx);
        bubbleRoot.setPadding(dp(8), dp(8), dp(8), dp(8));
        bubbleRoot.setClipToPadding(false);
        bubbleRoot.addView(b, new FrameLayout.LayoutParams(dp(54), dp(54)));
        bubble = bubbleRoot;
        bubbleParams = params(WindowManager.LayoutParams.WRAP_CONTENT, WindowManager.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.END, 0);
        bubbleParams.x = dp(6);
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
        draggable(names, cardRoot, cardParams, null);
        draggable(b, bubble, bubbleParams, () -> {
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
                        // Fenster hängt unten: nach unten ziehen = Abstand zum unteren Rand kleiner.
                        p.y = Math.max(0, startY - Math.round(dy));
                        wm.updateViewLayout(window, p);
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    if (dragging) {
                        cardParams.y = bubbleParams.y = p.y;
                        Prefs.setOverlayBottom(ctx, p.y);
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
