package com.marco.lyricstranslator;

import org.json.JSONArray;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Übersetzt Liedzeilen gebündelt über Google Translate; die Ausgangssprache wird automatisch erkannt. */
final class Translator {
    static final class Result {
        final String[] lines;
        final String detected;

        Result(String[] lines, String detected) {
            this.lines = lines;
            this.detected = detected;
        }
    }

    private Translator() {}

    /** Blockiert – nicht auf dem UI-Thread aufrufen. */
    static Result translate(List<Lyrics.Line> lines, String target) throws Exception {
        List<List<Integer>> chunks = new ArrayList<>();
        chunks.add(new ArrayList<>());
        int size = 0;
        for (int i = 0; i < lines.size(); i++) {
            String t = lines.get(i).text;
            if (t.isEmpty()) continue;
            List<Integer> last = chunks.get(chunks.size() - 1);
            if (size + t.length() > 1800 && !last.isEmpty()) {
                chunks.add(new ArrayList<>());
                size = 0;
            }
            chunks.get(chunks.size() - 1).add(i);
            size += t.length() + 1;
        }

        String[] out = new String[lines.size()];
        for (int i = 0; i < out.length; i++) out[i] = "";
        String detected = null;
        for (List<Integer> chunk : chunks) {
            if (chunk.isEmpty()) continue;
            StringBuilder text = new StringBuilder();
            for (int i : chunk) {
                if (text.length() > 0) text.append('\n');
                text.append(lines.get(i).text);
            }
            String body = Http.postForm(
                    "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&dt=t&tl=" + Http.enc(target),
                    "q=" + Http.enc(text.toString()));
            if (body == null) throw new IOException("Übersetzung fehlgeschlagen");
            JSONArray data = new JSONArray(body);
            StringBuilder translated = new StringBuilder();
            JSONArray sentences = data.getJSONArray(0);
            for (int s = 0; s < sentences.length(); s++) translated.append(sentences.getJSONArray(s).optString(0, ""));
            if (detected == null && data.length() > 2) detected = data.optString(2, null);
            String[] parts = translated.toString().split("\n", -1);
            for (int j = 0; j < chunk.size(); j++) out[chunk.get(j)] = j < parts.length ? parts[j].trim() : "";
        }
        return new Result(out, detected);
    }
}
