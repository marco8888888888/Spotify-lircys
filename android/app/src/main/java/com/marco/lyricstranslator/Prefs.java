package com.marco.lyricstranslator;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

final class Prefs {
    // Google-Translate-Sprachcodes.
    static final String[] LANGUAGES = {
        "af", "sq", "am", "ar", "hy", "az", "eu", "be", "bn", "bs", "bg", "ca", "ceb", "zh-CN", "zh-TW", "co",
        "hr", "cs", "da", "nl", "en", "eo", "et", "fi", "fr", "fy", "gl", "ka", "de", "el", "gu", "ht", "ha",
        "haw", "iw", "hi", "hmn", "hu", "is", "ig", "id", "ga", "it", "ja", "jw", "kn", "kk", "km", "rw", "ko",
        "ku", "ky", "lo", "la", "lv", "lt", "lb", "mk", "mg", "ms", "ml", "mt", "mi", "mr", "mn", "my", "ne",
        "no", "ny", "or", "ps", "fa", "pl", "pt", "pa", "ro", "ru", "sm", "gd", "sr", "st", "sn", "sd", "si",
        "sk", "sl", "so", "es", "su", "sw", "sv", "tl", "tg", "ta", "tt", "te", "th", "tr", "tk", "uk", "ur",
        "ug", "uz", "vi", "cy", "xh", "yi", "yo", "zu",
    };

    static final String MODE_BOTH = "both";
    static final String MODE_TRANSLATION = "translation";
    static final String MODE_ORIGINAL = "original";

    private Prefs() {}

    private static SharedPreferences sp(Context c) {
        return c.getSharedPreferences("settings", Context.MODE_PRIVATE);
    }

    static String lang(Context c) {
        String def = Locale.getDefault().getLanguage();
        if (!Arrays.asList(LANGUAGES).contains(def)) def = "de";
        return sp(c).getString("lang", def);
    }

    static void setLang(Context c, String v) {
        sp(c).edit().putString("lang", v).apply();
    }

    static String mode(Context c) {
        return sp(c).getString("mode", MODE_BOTH);
    }

    static void setMode(Context c, String v) {
        sp(c).edit().putString("mode", v).apply();
    }

    static boolean enabled(Context c) {
        return sp(c).getBoolean("enabled", true);
    }

    static void setEnabled(Context c, boolean v) {
        sp(c).edit().putBoolean("enabled", v).apply();
    }

    /** Übersetzung direkt unter Spotifys eigenen Lyrics-Zeilen (braucht die Bedienungshilfe). */
    static boolean inline(Context c) {
        return sp(c).getBoolean("inline", true);
    }

    static void setInline(Context c, boolean v) {
        sp(c).edit().putBoolean("inline", v).apply();
    }

    /** Abstand des Lyrics-Fensters vom unteren Bildschirmrand in Pixeln. */
    static int overlayBottom(Context c, int def) {
        return sp(c).getInt("overlayBottom", def);
    }

    static void setOverlayBottom(Context c, int y) {
        sp(c).edit().putInt("overlayBottom", y).apply();
    }

    static String langName(String code) {
        String tag = code.equals("iw") ? "he" : code.equals("jw") ? "jv" : code;
        Locale l = Locale.forLanguageTag(tag);
        String name = code.contains("-") ? l.getDisplayName() : l.getDisplayLanguage();
        if (name == null || name.isEmpty() || name.equalsIgnoreCase(tag)) return code;
        return Character.toUpperCase(name.charAt(0)) + name.substring(1);
    }

    /** Sprachcodes alphabetisch nach Namen in der Gerätesprache. */
    static List<String> sortedLanguages() {
        List<String> list = new ArrayList<>(Arrays.asList(LANGUAGES));
        Collections.sort(list, (a, b) -> langName(a).compareToIgnoreCase(langName(b)));
        return list;
    }
}
