package com.marco.lyricstranslator;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

final class Http {
    private static final String USER_AGENT = "LyricsTranslator-Android/1.0 (https://github.com/marco8888888888/Spotify-lircys)";

    private Http() {}

    static String enc(String s) {
        try {
            return URLEncoder.encode(s, "UTF-8");
        } catch (java.io.UnsupportedEncodingException e) {
            throw new AssertionError(e);
        }
    }

    /** Liefert den Antworttext oder null, wenn der Server keinen 200er-Status meldet. */
    static String get(String url) throws IOException {
        return request(url, null);
    }

    static String postForm(String url, String body) throws IOException {
        return request(url, body);
    }

    private static String request(String url, String formBody) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        try {
            c.setConnectTimeout(10000);
            c.setReadTimeout(15000);
            c.setRequestProperty("User-Agent", USER_AGENT);
            if (formBody != null) {
                c.setRequestMethod("POST");
                c.setDoOutput(true);
                c.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
                try (OutputStream out = c.getOutputStream()) {
                    out.write(formBody.getBytes(StandardCharsets.UTF_8));
                }
            }
            if (c.getResponseCode() != 200) return null;
            try (InputStream in = c.getInputStream()) {
                ByteArrayOutputStream buf = new ByteArrayOutputStream();
                byte[] chunk = new byte[8192];
                int n;
                while ((n = in.read(chunk)) > 0) buf.write(chunk, 0, n);
                return buf.toString("UTF-8");
            }
        } finally {
            c.disconnect();
        }
    }
}
