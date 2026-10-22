package com.marco.lyricstranslator;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
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
    private TextView notifStatus, overlayStatus, a11yStatus;
    private Button notifButton, overlayButton, a11yButton;

    private void settingsChanged() {
        NowPlaying.get(this).refreshSettings();
        Overlay.get(this).refreshSettings();
    }

    private int dp(float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, getResources().getDisplayMetrics()));
    }

    private TextView text(LinearLayout parent, String s, float size, boolean bold, int color) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(size);
        t.setTextColor(color);
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(bold ? 22 : 6);
        parent.addView(t, lp);
        return t;
    }

    private Button button(LinearLayout parent, String label, View.OnClickListener l) {
        Button b = new Button(this);
        b.setText(label);
        b.setAllCaps(false);
        b.setOnClickListener(l);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(6);
        parent.addView(b, lp);
        return b;
    }

    private Spinner spinner(LinearLayout parent, List<String> labels, int selected, AdapterView.OnItemSelectedListener l) {
        Spinner s = new Spinner(this);
        ArrayAdapter<String> a = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, labels);
        a.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        s.setAdapter(a);
        s.setSelection(selected, false);
        s.setOnItemSelectedListener(l);
        parent.addView(s);
        return s;
    }

    private abstract static class OnSelect implements AdapterView.OnItemSelectedListener {
        @Override
        public void onNothingSelected(AdapterView<?> parent) {}
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        int white = 0xFFFFFFFF, gray = 0xFFB3B3B3;

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(0xFF121212);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(20), dp(28), dp(20), dp(40));
        scroll.addView(root);

        text(root, "Lyrics Translator", 26, true, white);
        text(root, "Zeigt zu jedem Song, der in Spotify läuft, die Lyrics direkt in Spotify an – "
                + "mitlaufend und übersetzt in die Sprache deiner Wahl.", 14, false, gray);
        Button player = button(root, "▶  Player mit Lyrics öffnen", v -> startActivity(new Intent(this, PlayerActivity.class)));
        player.setTextSize(17);

        text(root, "1. Spotify mitlesen", 17, true, white);
        text(root, "Android erlaubt nur mit „Benachrichtigungszugriff“, zu sehen, welcher Song in Spotify läuft. "
                + "Die App liest dabei keine Nachrichten.\n\nWird der Schalter grau angezeigt („Eingeschränkte Einstellung“): "
                + "App-Info von Lyrics Translator öffnen → oben rechts ⋮ → „Eingeschränkte Einstellungen zulassen“, dann nochmal versuchen.",
                13, false, gray);
        notifStatus = text(root, "", 14, false, white);
        notifButton = button(root, "Benachrichtigungszugriff erlauben",
                v -> startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)));
        button(root, "App-Info öffnen (für eingeschränkte Einstellungen)",
                v -> startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:" + getPackageName()))));

        text(root, "2. Über Spotify anzeigen", 17, true, white);
        overlayStatus = text(root, "", 14, false, white);
        overlayButton = button(root, "„Über anderen Apps einblenden“ erlauben",
                v -> startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()))));

        text(root, "3. Nur in Spotify & unter Spotifys Lyrics (empfohlen)", 17, true, white);
        text(root, "Mit der Bedienungshilfe erscheint das Fenster nur, wenn Spotify offen ist. Und wenn du in Spotify "
                + "die Lyrics öffnest, steht die Übersetzung direkt unter jeder Zeile. Die App liest dafür nur Texte aus Spotify.\n\n"
                + "Einstellungen → Bedienungshilfen → Installierte Apps → „Lyrics Translator – in Spotify anzeigen“ → Ein. "
                + "Ist es grau: wie oben „Eingeschränkte Einstellungen zulassen“.", 13, false, gray);
        a11yStatus = text(root, "", 14, false, white);
        a11yButton = button(root, "Bedienungshilfe einschalten",
                v -> startActivity(new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)));
        Switch inline = new Switch(this);
        inline.setText("Übersetzung unter Spotifys eigenen Lyrics");
        inline.setTextColor(white);
        inline.setTextSize(15);
        inline.setChecked(Prefs.inline(this));
        inline.setPadding(0, dp(12), 0, dp(4));
        inline.setOnCheckedChangeListener((b, on) -> {
            Prefs.setInline(this, on);
            settingsChanged();
        });
        root.addView(inline);

        text(root, "4. Sprache", 17, true, white);
        List<String> codes = Prefs.sortedLanguages();
        List<String> names = new ArrayList<>();
        for (String c : codes) names.add(Prefs.langName(c));
        spinner(root, names, Math.max(0, codes.indexOf(Prefs.lang(this))), new OnSelect() {
            @Override
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                Prefs.setLang(MainActivity.this, codes.get(pos));
                settingsChanged();
            }
        });

        text(root, "Anzeige", 17, true, white);
        String[] modes = {Prefs.MODE_BOTH, Prefs.MODE_TRANSLATION, Prefs.MODE_ORIGINAL};
        List<String> modeNames = Arrays.asList("Original + Übersetzung", "Nur Übersetzung", "Nur Original");
        int modeIndex = 0;
        for (int i = 0; i < modes.length; i++) if (modes[i].equals(Prefs.mode(this))) modeIndex = i;
        spinner(root, modeNames, modeIndex, new OnSelect() {
            @Override
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                Prefs.setMode(MainActivity.this, modes[pos]);
                settingsChanged();
            }
        });

        Switch enabled = new Switch(this);
        enabled.setText("Lyrics-Fenster über Spotify anzeigen");
        enabled.setTextColor(white);
        enabled.setTextSize(15);
        enabled.setChecked(Prefs.enabled(this));
        enabled.setPadding(0, dp(20), 0, dp(4));
        enabled.setOnCheckedChangeListener((b, on) -> {
            Prefs.setEnabled(this, on);
            settingsChanged();
        });
        root.addView(enabled);

        button(root, "Spotify öffnen", v -> {
            Intent i = getPackageManager().getLaunchIntentForPackage("com.spotify.music");
            if (i != null) startActivity(i);
        });

        text(root, "Tipp: Fenster am Titel nach oben/unten ziehen. „—“ macht es klein (grüner Knopf), "
                + "„✕“ blendet es bis zum nächsten Song aus, „⛶“ öffnet den Player.", 13, false, gray);

        setContentView(scroll);
    }

    @Override
    protected void onResume() {
        super.onResume();
        String listeners = Settings.Secure.getString(getContentResolver(), "enabled_notification_listeners");
        boolean notif = listeners != null && listeners.contains(getPackageName());
        boolean overlay = Settings.canDrawOverlays(this);
        notifStatus.setText(notif ? "✅ Erlaubt" : "❌ Noch nicht erlaubt");
        overlayStatus.setText(overlay ? "✅ Erlaubt" : "❌ Noch nicht erlaubt");
        notifButton.setVisibility(notif ? View.GONE : View.VISIBLE);
        overlayButton.setVisibility(overlay ? View.GONE : View.VISIBLE);
        String services = Settings.Secure.getString(getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        boolean a11y = services != null && services.contains(getPackageName() + "/");
        a11yStatus.setText(a11y ? "✅ Eingeschaltet" : "➖ Aus (optional)");
        a11yButton.setVisibility(a11y ? View.GONE : View.VISIBLE);
        settingsChanged();
    }
}
