package com.marco.lyricstranslator;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/** Baut die Lyrics-Zeilen (Original + Übersetzung) für Overlay und Player. */
final class LyricsRenderer {
    interface OnLineClick {
        void onLine(int index);
    }

    static final int GREEN = 0xFF1ED760;
    /** Übersetzung: helles Grün, gut lesbar auf jeder dunklen Cover-Farbe. */
    static final int TRANSLATION = 0xFFB4F5C8;
    static final float DIM = 0.4f, PAST = 0.65f;

    private LyricsRenderer() {}

    private static int dp(Context c, float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, c.getResources().getDisplayMetrics()));
    }

    static void render(Context ctx, LinearLayout box, NowPlaying np, float origSp, float transSp, OnLineClick click) {
        box.removeAllViews();
        Lyrics lyrics = np.lyrics;
        if (lyrics == null) return;
        String mode = Prefs.mode(ctx);
        boolean dimmed = lyrics.synced && np.playback != null;
        for (int i = 0; i < lyrics.lines.size(); i++) {
            Lyrics.Line line = lyrics.lines.get(i);
            LinearLayout row = new LinearLayout(ctx);
            row.setOrientation(LinearLayout.VERTICAL);
            row.setPadding(0, dp(ctx, 6), 0, dp(ctx, 6));
            if (line.text.isEmpty()) {
                row.setMinimumHeight(dp(ctx, origSp * 0.7f));
                box.addView(row);
                continue;
            }
            String trans = np.translationAt(i);
            boolean hasTrans = !trans.isEmpty();
            if (!Prefs.MODE_TRANSLATION.equals(mode) || !hasTrans) row.addView(text(ctx, line.text, origSp, 0xFFFFFFFF, true));
            if (!Prefs.MODE_ORIGINAL.equals(mode) && hasTrans) {
                boolean under = !Prefs.MODE_TRANSLATION.equals(mode);
                TextView t = text(ctx, trans, under ? transSp : origSp, under ? TRANSLATION : 0xFFFFFFFF, !under);
                if (under) t.setPadding(0, dp(ctx, 2), 0, 0);
                row.addView(t);
            }
            row.setAlpha(dimmed ? DIM : 1f);
            if (click != null && lyrics.synced) {
                int index = i;
                row.setOnClickListener(v -> click.onLine(index));
            }
            box.addView(row);
        }
    }

    private static TextView text(Context ctx, String s, float sp, int color, boolean bold) {
        TextView t = new TextView(ctx);
        t.setText(s);
        t.setTextSize(sp);
        t.setTextColor(color);
        t.setTypeface(Typeface.create("sans-serif", bold ? Typeface.BOLD : Typeface.NORMAL));
        t.setLineSpacing(0, 1.08f);
        return t;
    }

    /** Hebt Zeile idx hervor und scrollt sie ins obere Drittel. */
    static void highlight(LinearLayout box, ScrollView scroll, int idx) {
        for (int i = 0; i < box.getChildCount(); i++) {
            float target = i == idx ? 1f : i < idx ? PAST : DIM;
            View row = box.getChildAt(i);
            if (row.getAlpha() != target) row.animate().alpha(target).setDuration(220).start();
        }
        if (idx >= 0 && idx < box.getChildCount()) {
            View row = box.getChildAt(idx);
            scroll.post(() -> scroll.smoothScrollTo(0, Math.max(0, row.getTop() - scroll.getHeight() / 3)));
        }
    }
}
