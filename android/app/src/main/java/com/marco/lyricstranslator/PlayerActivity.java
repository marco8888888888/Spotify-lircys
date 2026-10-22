package com.marco.lyricstranslator;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Vollbild-Player im Spotify-Stil: Cover, mitlaufende Lyrics mit Übersetzung, Fortschritt und Steuerung.
 * Die Musik selbst läuft weiter in Spotify – hier wird Spotify nur ferngesteuert.
 */
public class PlayerActivity extends Activity implements NowPlaying.Listener {
    private final Handler main = new Handler(Looper.getMainLooper());
    private NowPlaying now;

    private LinearLayout root, linesBox;
    private GradientDrawable background;
    private ImageView cover;
    private TextView title, artist, sub, message, elapsed, remaining, playPause;
    private ScrollView scroll;
    private SeekBar seek;
    private boolean userSeeking;
    private String renderedKey;
    private int activeIndex = -2;

    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            updateProgress();
            int idx = now.activeIndex();
            if (idx != activeIndex) {
                activeIndex = idx;
                LyricsRenderer.highlight(linesBox, scroll, idx);
            }
            main.postDelayed(this, 200);
        }
    };

    private int dp(float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, getResources().getDisplayMetrics()));
    }

    private TextView label(String s, float sp, int color, boolean bold) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(sp);
        t.setTextColor(color);
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        t.setSingleLine(true);
        t.setEllipsize(TextUtils.TruncateAt.END);
        return t;
    }

    private TextView control(String symbol, float sp, View.OnClickListener l) {
        TextView b = label(symbol, sp, 0xFFFFFFFF, true);
        b.setGravity(Gravity.CENTER);
        b.setPadding(dp(18), dp(6), dp(18), dp(6));
        b.setOnClickListener(l);
        return b;
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        now = NowPlaying.get(this);
        Window w = getWindow();
        w.setStatusBarColor(0x00000000);
        w.setNavigationBarColor(0xFF121212);

        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(20), dp(36), dp(20), dp(20));
        background = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, new int[] {NowPlaying.DEFAULT_COLOR, 0xFF121212});
        root.setBackground(background);

        // Kopfzeile: schließen, Sprache, Einstellungen
        LinearLayout top = new LinearLayout(this);
        top.setGravity(Gravity.CENTER_VERTICAL);
        TextView close = control("⌄", 24, v -> finish());
        close.setPadding(0, 0, dp(12), 0);
        top.addView(close);
        Spinner lang = new Spinner(this);
        List<String> codes = Prefs.sortedLanguages();
        List<String> names = new ArrayList<>();
        for (String c : codes) names.add("文A  " + Prefs.langName(c));
        ArrayAdapter<String> a = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, names);
        a.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        lang.setAdapter(a);
        lang.setSelection(Math.max(0, codes.indexOf(Prefs.lang(this))), false);
        lang.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                if (codes.get(pos).equals(Prefs.lang(PlayerActivity.this))) return;
                Prefs.setLang(PlayerActivity.this, codes.get(pos));
                now.refreshSettings();
            }

            @Override
            public void onNothingSelected(AdapterView<?> p) {}
        });
        top.addView(lang, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        top.addView(control("⚙", 20, v -> startActivity(new Intent(this, MainActivity.class))));
        root.addView(top);

        // Song: Cover + Titel
        LinearLayout song = new LinearLayout(this);
        song.setGravity(Gravity.CENTER_VERTICAL);
        song.setPadding(0, dp(12), 0, dp(8));
        cover = new ImageView(this);
        cover.setScaleType(ImageView.ScaleType.CENTER_CROP);
        cover.setClipToOutline(true);
        GradientDrawable coverBg = new GradientDrawable();
        coverBg.setColor(0xFF333333);
        coverBg.setCornerRadius(dp(8));
        cover.setBackground(coverBg);
        song.addView(cover, new LinearLayout.LayoutParams(dp(64), dp(64)));
        LinearLayout names2 = new LinearLayout(this);
        names2.setOrientation(LinearLayout.VERTICAL);
        names2.setPadding(dp(14), 0, 0, 0);
        title = label("", 20, 0xFFFFFFFF, true);
        artist = label("", 15, 0xFFDDDDDD, false);
        sub = label("", 11, 0xFFB3B3B3, false);
        names2.addView(title);
        names2.addView(artist);
        names2.addView(sub);
        song.addView(names2, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        root.addView(song);

        // Lyrics
        message = label("", 16, 0xFFB3B3B3, false);
        message.setSingleLine(false);
        message.setPadding(0, dp(20), 0, 0);
        root.addView(message);
        scroll = new ScrollView(this);
        scroll.setVerticalScrollBarEnabled(false);
        linesBox = new LinearLayout(this);
        linesBox.setOrientation(LinearLayout.VERTICAL);
        linesBox.setPadding(0, dp(8), 0, dp(240));
        scroll.addView(linesBox);
        root.addView(scroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1));

        // Fortschritt
        seek = new SeekBar(this);
        seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar s, int p, boolean fromUser) {
                if (fromUser) elapsed.setText(clock(p));
            }

            @Override
            public void onStartTrackingTouch(SeekBar s) {
                userSeeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar s) {
                userSeeking = false;
                now.seekTo(s.getProgress());
            }
        });
        root.addView(seek);
        LinearLayout times = new LinearLayout(this);
        elapsed = label("0:00", 12, 0xFFB3B3B3, false);
        remaining = label("", 12, 0xFFB3B3B3, false);
        remaining.setGravity(Gravity.END);
        times.addView(elapsed, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        times.addView(remaining, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        times.setPadding(dp(16), 0, dp(16), 0);
        root.addView(times);

        // Steuerung
        LinearLayout controls = new LinearLayout(this);
        controls.setGravity(Gravity.CENTER);
        controls.setPadding(0, dp(8), 0, 0);
        controls.addView(control("⏮", 30, v -> now.previous()));
        playPause = control("⏸", 30, v -> now.playPause());
        GradientDrawable playBg = new GradientDrawable();
        playBg.setShape(GradientDrawable.OVAL);
        playBg.setColor(0xFFFFFFFF);
        playPause.setBackground(playBg);
        playPause.setTextColor(0xFF000000);
        controls.addView(playPause, new LinearLayout.LayoutParams(dp(68), dp(68)));
        controls.addView(control("⏭", 30, v -> now.next()));
        root.addView(controls);

        setContentView(root);
    }

    @Override
    protected void onResume() {
        super.onResume();
        now.addListener(this);
        renderedKey = null;
        onNowPlayingChanged();
        main.post(tick);
    }

    @Override
    protected void onPause() {
        super.onPause();
        now.removeListener(this);
        main.removeCallbacks(tick);
    }

    @Override
    public void onNowPlayingChanged() {
        background.setColors(new int[] {now.color, 0xFF121212});
        if (now.art != null) cover.setImageBitmap(now.art);
        else cover.setImageDrawable(null);
        title.setText(now.title == null ? "Nichts läuft" : now.title);
        artist.setText(now.artist == null ? "" : now.artist);
        sub.setText(now.subline());
        playPause.setText(now.isPlaying() ? "⏸" : "▶");

        if (now.controller == null) {
            message.setText("Spiele einen Song in Spotify ab.\n\nFalls hier nichts passiert: In den Einstellungen (⚙) den Benachrichtigungszugriff erlauben.");
        } else if (now.lyrics == null) {
            message.setText(now.status == null ? "" : now.status);
        } else {
            message.setText("");
        }
        message.setVisibility(message.getText().length() == 0 ? View.GONE : View.VISIBLE);

        String key = now.trackKey + "|" + (now.lyrics != null) + "|" + (now.translations != null) + "|" + Prefs.mode(this);
        if (!key.equals(renderedKey)) {
            renderedKey = key;
            LyricsRenderer.render(this, linesBox, now, 24, 18, i -> now.seekTo(now.lyrics.lines.get(i).timeMs));
            activeIndex = -2;
            scroll.scrollTo(0, 0);
        }
        updateProgress();
    }

    private void updateProgress() {
        int dur = (int) Math.max(0, now.durationMs);
        seek.setMax(Math.max(1, dur));
        if (userSeeking) return;
        int pos = (int) now.positionMs();
        seek.setProgress(pos);
        elapsed.setText(clock(pos));
        remaining.setText(dur > 0 ? "-" + clock(Math.max(0, dur - pos)) : "");
    }

    private static String clock(long ms) {
        long s = ms / 1000;
        return String.format(Locale.ROOT, "%d:%02d", s / 60, s % 60);
    }
}
