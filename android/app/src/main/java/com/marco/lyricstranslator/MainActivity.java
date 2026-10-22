package com.marco.lyricstranslator;

import android.app.Activity;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Einrichtung: Berechtigungen, Zielsprache, Anzeigemodus. */
public class MainActivity extends Activity {
    private static final int BG = 0xFF121212, CARD = 0xFF1E1E1E, FIELD = 0xFF2A2A2A, WHITE = 0xFFFFFFFF, GRAY = 0xFFB3B3B3;

    private TextView notifStatus, overlayStatus, a11yStatus;
    private View notifButton, overlayButton, a11yButton;

    private int dp(float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, getResources().getDisplayMetrics()));
    }

    private void settingsChanged() {
        NowPlaying.get(this).refreshSettings();
        Overlay.get(this).refreshSettings();
    }

    // ---------- Bausteine ----------

    private TextView text(LinearLayout parent, String s, float size, boolean bold, int color, int topMargin) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(size);
        t.setTextColor(color);
        t.setLineSpacing(0, 1.15f);
        t.setTypeface(Typeface.create("sans-serif", bold ? Typeface.BOLD : Typeface.NORMAL));
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(topMargin);
        parent.addView(t, lp);
        return t;
    }

    private GradientDrawable rounded(int color, float radiusDp) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radiusDp));
        return d;
    }

    private LinearLayout card(LinearLayout parent) {
        LinearLayout c = new LinearLayout(this);
        c.setOrientation(LinearLayout.VERTICAL);
        c.setBackground(rounded(CARD, 14));
        c.setPadding(dp(16), dp(14), dp(16), dp(16));
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(14);
        parent.addView(c, lp);
        return c;
    }

    /** Kartenkopf mit Nummer, Titel und Status rechts. */
    private TextView cardHeader(LinearLayout card, String number, String title) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        TextView n = new TextView(this);
        n.setText(number);
        n.setTextSize(13);
        n.setTextColor(0xFF000000);
        n.setTypeface(Typeface.DEFAULT_BOLD);
        n.setGravity(Gravity.CENTER);
        GradientDrawable circle = new GradientDrawable();
        circle.setShape(GradientDrawable.OVAL);
        circle.setColor(LyricsRenderer.GREEN);
        n.setBackground(circle);
        row.addView(n, new LinearLayout.LayoutParams(dp(24), dp(24)));
        TextView t = new TextView(this);
        t.setText(title);
        t.setTextSize(16);
        t.setTextColor(WHITE);
        t.setTypeface(Typeface.create("sans-serif", Typeface.BOLD));
        t.setPadding(dp(10), 0, dp(8), 0);
        row.addView(t, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        TextView status = new TextView(this);
        status.setTextSize(13);
        status.setTypeface(Typeface.DEFAULT_BOLD);
        row.addView(status);
        card.addView(row);
        return status;
    }

    /** Runder Knopf wie bei Spotify: grün gefüllt oder mit grauem Rand. */
    private TextView pill(LinearLayout parent, String label, boolean primary, View.OnClickListener l) {
        TextView b = new TextView(this);
        b.setText(label);
        b.setTextSize(15);
        b.setGravity(Gravity.CENTER);
        b.setTypeface(Typeface.create("sans-serif", Typeface.BOLD));
        b.setTextColor(primary ? 0xFF000000 : WHITE);
        b.setPadding(dp(18), dp(12), dp(18), dp(12));
        GradientDrawable shape = new GradientDrawable();
        shape.setCornerRadius(dp(999));
        if (primary) shape.setColor(LyricsRenderer.GREEN);
        else shape.setStroke(dp(1), 0xFF7F7F7F);
        b.setBackground(new RippleDrawable(ColorStateList.valueOf(primary ? 0x33000000 : 0x33FFFFFF), shape, rounded(WHITE, 999)));
        b.setOnClickListener(l);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(12);
        parent.addView(b, lp);
        return b;
    }

    private void toggle(LinearLayout parent, String label, boolean checked, CompoundButton.OnCheckedChangeListener l) {
        Switch s = new Switch(this);
        s.setText(label);
        s.setTextColor(WHITE);
        s.setTextSize(15);
        s.setChecked(checked);
        int[][] states = {{android.R.attr.state_checked}, {}};
        s.setThumbTintList(new ColorStateList(states, new int[] {LyricsRenderer.GREEN, 0xFFB3B3B3}));
        s.setTrackTintList(new ColorStateList(states, new int[] {0x991ED760, 0xFF535353}));
        s.setPadding(0, dp(12), 0, dp(2));
        s.setOnCheckedChangeListener(l);
        parent.addView(s, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
    }

    private void spinner(LinearLayout parent, List<String> labels, int selected, AdapterView.OnItemSelectedListener l) {
        Spinner s = new Spinner(this);
        ArrayAdapter<String> a = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, labels);
        a.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        s.setAdapter(a);
        s.setSelection(selected, false);
        s.setOnItemSelectedListener(l);
        s.setBackground(rounded(FIELD, 10));
        s.setPopupBackgroundDrawable(rounded(FIELD, 10));
        s.setPadding(dp(8), 0, dp(8), 0);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(48));
        lp.topMargin = dp(8);
        parent.addView(s, lp);
    }

    private abstract static class OnSelect implements AdapterView.OnItemSelectedListener {
        @Override
        public void onNothingSelected(AdapterView<?> parent) {}
    }

    // ---------- Bildschirm ----------

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(BG);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(18), dp(28), dp(18), dp(40));
        scroll.addView(root);

        // Kopf mit Logo
        LinearLayout head = new LinearLayout(this);
        head.setGravity(Gravity.CENTER_VERTICAL);
        ImageView logo = new ImageView(this);
        logo.setImageResource(R.drawable.ic_launcher);
        head.addView(logo, new LinearLayout.LayoutParams(dp(52), dp(52)));
        LinearLayout headText = new LinearLayout(this);
        headText.setOrientation(LinearLayout.VERTICAL);
        headText.setPadding(dp(14), 0, 0, 0);
        text(headText, "Lyrics Translator", 24, true, WHITE, 0);
        text(headText, "Lyrics in Spotify – übersetzt in jede Sprache", 13, false, GRAY, 2);
        head.addView(headText);
        root.addView(head);

        // 1. Benachrichtigungszugriff
        LinearLayout c1 = card(root);
        notifStatus = cardHeader(c1, "1", "Spotify erkennen");
        text(c1, "Nur mit „Benachrichtigungszugriff“ verrät Android, welcher Song in Spotify läuft. "
                + "Nachrichten werden nicht gelesen.", 13, false, GRAY, 8);
        notifButton = pill(c1, "Erlauben", true, v -> startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)));

        // 2. Overlay
        LinearLayout c2 = card(root);
        overlayStatus = cardHeader(c2, "2", "In Spotify anzeigen");
        text(c2, "Damit die Lyrics über Spotify erscheinen dürfen.", 13, false, GRAY, 8);
        overlayButton = pill(c2, "Erlauben", true,
                v -> startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()))));

        // 3. Bedienungshilfe
        LinearLayout c3 = card(root);
        a11yStatus = cardHeader(c3, "3", "Direkt unter Spotifys Lyrics");
        text(c3, "Empfohlen: Das Fenster erscheint dann nur in Spotify. Und wenn du in Spotify die Lyrics öffnest, "
                + "steht die Übersetzung direkt unter jeder Zeile.\n\n"
                + "Bedienungshilfen → Installierte Apps → „Lyrics Translator – in Spotify anzeigen“ → Ein.", 13, false, GRAY, 8);
        a11yButton = pill(c3, "Bedienungshilfen öffnen", true, v -> startActivity(new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)));
        toggle(c3, "Übersetzung unter Spotifys Lyrics", Prefs.inline(this), (b, on) -> {
            Prefs.setInline(this, on);
            settingsChanged();
        });

        // Hinweis für graue Schalter (Android 13+)
        LinearLayout hint = card(root);
        text(hint, "Schalter grau? („Eingeschränkte Einstellung“)", 14, true, WHITE, 0);
        text(hint, "App-Info öffnen → oben rechts ⋮ → „Eingeschränkte Einstellungen zulassen“ → dann Schritt 1 oder 3 nochmal.",
                13, false, GRAY, 6);
        pill(hint, "App-Info öffnen", false,
                v -> startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:" + getPackageName()))));

        // Sprache & Anzeige
        LinearLayout c4 = card(root);
        text(c4, "Übersetzen in", 16, true, WHITE, 0);
        List<String> codes = Prefs.sortedLanguages();
        List<String> names = new ArrayList<>();
        for (String c : codes) names.add(Prefs.langName(c));
        spinner(c4, names, Math.max(0, codes.indexOf(Prefs.lang(this))), new OnSelect() {
            @Override
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                Prefs.setLang(MainActivity.this, codes.get(pos));
                settingsChanged();
            }
        });
        text(c4, "Anzeige", 16, true, WHITE, 16);
        String[] modes = {Prefs.MODE_BOTH, Prefs.MODE_TRANSLATION, Prefs.MODE_ORIGINAL};
        List<String> modeNames = Arrays.asList("Original + Übersetzung", "Nur Übersetzung", "Nur Original");
        int modeIndex = 0;
        for (int i = 0; i < modes.length; i++) if (modes[i].equals(Prefs.mode(this))) modeIndex = i;
        spinner(c4, modeNames, modeIndex, new OnSelect() {
            @Override
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                Prefs.setMode(MainActivity.this, modes[pos]);
                settingsChanged();
            }
        });
        toggle(c4, "Lyrics in Spotify anzeigen", Prefs.enabled(this), (b, on) -> {
            Prefs.setEnabled(this, on);
            settingsChanged();
        });

        pill(root, "Spotify öffnen", true, v -> {
            Intent i = getPackageManager().getLaunchIntentForPackage(SpotifyListener.SPOTIFY);
            if (i != null) startActivity(i);
        });

        text(root, "Im Lyrics-Fenster: am Titel nach oben/unten ziehen · ⌄ minimieren · ✕ bis zum nächsten Song ausblenden · "
                + "Vollbild-Knopf öffnet den großen Player", 12, false, 0xFF8C8C8C, 14).setGravity(Gravity.CENTER);

        setContentView(scroll);
    }

    private void setStatus(TextView status, View button, boolean ok, String offText, int offColor) {
        status.setText(ok ? "✓ Erledigt" : offText);
        status.setTextColor(ok ? LyricsRenderer.GREEN : offColor);
        button.setVisibility(ok ? View.GONE : View.VISIBLE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        String listeners = Settings.Secure.getString(getContentResolver(), "enabled_notification_listeners");
        setStatus(notifStatus, notifButton, listeners != null && listeners.contains(getPackageName()), "Fehlt", 0xFFFF8A80);
        setStatus(overlayStatus, overlayButton, Settings.canDrawOverlays(this), "Fehlt", 0xFFFF8A80);
        String services = Settings.Secure.getString(getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        setStatus(a11yStatus, a11yButton, services != null && services.contains(getPackageName() + "/"), "Optional", GRAY);
        settingsChanged();
    }
}
