// NAME: Lyrics Translator
// AUTHOR: marco8888888888
// DESCRIPTION: Zeigt für jeden Song (synchronisierte) Lyrics an und übersetzt sie in jede Sprache.

(function LyricsTranslator() {
  const S = window.Spicetify;
  if (!S || !S.Player || !S.Topbar || !S.CosmosAsync || !S.LocalStorage) {
    setTimeout(LyricsTranslator, 300);
    return;
  }

  // Google-Translate-Sprachcodes. Namen werden über Intl.DisplayNames in der UI-Sprache angezeigt.
  const LANGUAGES = [
    "af", "sq", "am", "ar", "hy", "az", "eu", "be", "bn", "bs", "bg", "ca", "ceb", "zh-CN", "zh-TW", "co",
    "hr", "cs", "da", "nl", "en", "eo", "et", "fi", "fr", "fy", "gl", "ka", "de", "el", "gu", "ht", "ha",
    "haw", "iw", "hi", "hmn", "hu", "is", "ig", "id", "ga", "it", "ja", "jw", "kn", "kk", "km", "rw", "ko",
    "ku", "ky", "lo", "la", "lv", "lt", "lb", "mk", "mg", "ms", "ml", "mt", "mi", "mr", "mn", "my", "ne",
    "no", "ny", "or", "ps", "fa", "pl", "pt", "pa", "ro", "ru", "sm", "gd", "sr", "st", "sn", "sd", "si",
    "sk", "sl", "so", "es", "su", "sw", "sv", "tl", "tg", "ta", "tt", "te", "th", "tr", "tk", "uk", "ur",
    "ug", "uz", "vi", "cy", "xh", "yi", "yo", "zu",
  ];
  const NAME_ALIASES = { iw: "he", jw: "jv" };

  const store = {
    get(key, fallback) {
      const v = S.LocalStorage.get("lyrics-translator:" + key);
      return v == null ? fallback : v;
    },
    set(key, value) {
      S.LocalStorage.set("lyrics-translator:" + key, value);
    },
  };

  const state = {
    targetLang: store.get("lang", (navigator.language || "de").split("-")[0]),
    mode: store.get("mode", "both"), // both | translation | original
    open: store.get("open", "false") === "true",
    trackKey: null,
    lyrics: null, // { synced: bool, lines: [{ time, text }], source }
    translations: null, // string[] | null
    activeIndex: -1,
    requestId: 0,
  };
  const translationCache = new Map();

  // ---------- Lyrics-Quellen ----------

  function currentTrack() {
    const item = S.Player.data && (S.Player.data.item || S.Player.data.track);
    if (!item) return null;
    const m = item.metadata || {};
    const title = m.title || item.name;
    const artist = m.artist_name || (item.artists && item.artists[0] && item.artists[0].name);
    if (!title || !artist) return null;
    return {
      uri: item.uri,
      id: item.uri && item.uri.split(":")[2],
      title,
      artist,
      album: m.album_title || (item.album && item.album.name) || "",
      durationMs: Number(m.duration || (item.duration && item.duration.milliseconds) || 0),
    };
  }

  function parseLrc(lrc) {
    const lines = [];
    for (const raw of lrc.split(/\r?\n/)) {
      const stamps = [...raw.matchAll(/\[(\d+):(\d+(?:\.\d+)?)\]/g)];
      if (!stamps.length) continue;
      const text = raw.replace(/\[\d+:\d+(?:\.\d+)?\]/g, "").trim();
      for (const s of stamps) lines.push({ time: (Number(s[1]) * 60 + Number(s[2])) * 1000, text });
    }
    return lines.sort((a, b) => a.time - b.time);
  }

  function plainToLines(text) {
    return text.split(/\r?\n/).map((t) => ({ time: null, text: t.trim() }));
  }

  async function fromSpotify(track) {
    if (!track.id) return null;
    const res = await S.CosmosAsync.get(
      `https://spclient.wg.spotify.com/color-lyrics/v2/track/${track.id}?format=json&vocalRemoval=false&market=from_token`
    );
    const l = res && res.lyrics;
    if (!l || !l.lines || !l.lines.length) return null;
    const synced = l.syncType === "LINE_SYNCED";
    return {
      synced,
      source: "Spotify",
      lines: l.lines.map((x) => ({ time: synced ? Number(x.startTimeMs) : null, text: (x.words || "").replace("♪", "").trim() })),
    };
  }

  function lrclibResult(r) {
    if (!r) return null;
    if (r.syncedLyrics) return { synced: true, source: "LRCLIB", lines: parseLrc(r.syncedLyrics) };
    if (r.plainLyrics) return { synced: false, source: "LRCLIB", lines: plainToLines(r.plainLyrics) };
    return null;
  }

  async function fromLrclib(track) {
    const base = "https://lrclib.net/api";
    const params = new URLSearchParams({ artist_name: track.artist, track_name: track.title });
    if (track.album) params.set("album_name", track.album);
    if (track.durationMs) params.set("duration", String(Math.round(track.durationMs / 1000)));
    let res = await fetch(`${base}/get?${params}`);
    if (res.ok) {
      const hit = lrclibResult(await res.json());
      if (hit) return hit;
    }
    // Unscharfe Suche: Titel ohne Zusätze wie "(feat. …)" oder "- Remastered".
    const cleanTitle = track.title.replace(/\s*[([].*?[)\]]\s*/g, " ").replace(/\s+-\s+.*$/, "").trim();
    res = await fetch(`${base}/search?${new URLSearchParams({ track_name: cleanTitle, artist_name: track.artist })}`);
    let list = res.ok ? await res.json() : [];
    if (!list.length) {
      res = await fetch(`${base}/search?${new URLSearchParams({ q: `${track.artist} ${cleanTitle}` })}`);
      list = res.ok ? await res.json() : [];
    }
    if (track.durationMs) {
      const d = track.durationMs / 1000;
      list.sort((a, b) => Math.abs(a.duration - d) - Math.abs(b.duration - d));
    }
    list.sort((a, b) => Number(!!b.syncedLyrics) - Number(!!a.syncedLyrics));
    for (const r of list) {
      const hit = lrclibResult(r);
      if (hit) return hit;
    }
    return null;
  }

  async function fromLyricsOvh(track) {
    const res = await fetch(`https://api.lyrics.ovh/v1/${encodeURIComponent(track.artist)}/${encodeURIComponent(track.title)}`);
    if (!res.ok) return null;
    const data = await res.json();
    return data.lyrics ? { synced: false, source: "lyrics.ovh", lines: plainToLines(data.lyrics) } : null;
  }

  async function findLyrics(track) {
    let fallback = null;
    for (const source of [fromSpotify, fromLrclib, fromLyricsOvh]) {
      try {
        const hit = await source(track);
        if (hit && hit.lines.some((l) => l.text)) {
          if (hit.synced) return hit;
          fallback = fallback || hit;
        }
      } catch (e) {
        console.warn("[Lyrics Translator]", source.name, e);
      }
    }
    return fallback;
  }

  // ---------- Übersetzung ----------

  async function translateChunk(text, target) {
    const res = await fetch(`https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=${encodeURIComponent(target)}&dt=t`, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
      body: "q=" + encodeURIComponent(text),
    });
    if (!res.ok) throw new Error("Übersetzung fehlgeschlagen (" + res.status + ")");
    const data = await res.json();
    return { text: data[0].map((s) => s[0]).join(""), detected: data[2] };
  }

  // Übersetzt alle nicht-leeren Zeilen gebündelt und ordnet sie wieder den Originalzeilen zu.
  async function translateLines(lines, target) {
    const idx = [];
    lines.forEach((l, i) => l.text && idx.push(i));
    const chunks = [[]];
    let size = 0;
    for (const i of idx) {
      if (size + lines[i].text.length > 1800 && chunks[chunks.length - 1].length) {
        chunks.push([]);
        size = 0;
      }
      chunks[chunks.length - 1].push(i);
      size += lines[i].text.length + 1;
    }
    const out = lines.map(() => "");
    let detected = null;
    for (const chunk of chunks) {
      if (!chunk.length) continue;
      const r = await translateChunk(chunk.map((i) => lines[i].text).join("\n"), target);
      detected = detected || r.detected;
      const parts = r.text.split("\n");
      chunk.forEach((i, j) => (out[i] = (parts[j] || "").trim()));
    }
    return { lines: out, detected };
  }

  // ---------- UI ----------

  const style = document.createElement("style");
  style.textContent = `
  #lt-panel{position:fixed;top:64px;right:16px;bottom:96px;width:min(420px,40vw);z-index:9999;display:flex;flex-direction:column;
    background:rgba(18,18,18,.97);color:#fff;border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,.6);font-family:inherit;overflow:hidden}
  #lt-panel[hidden]{display:none}
  #lt-head{padding:12px 14px;border-bottom:1px solid #2a2a2a;display:flex;flex-direction:column;gap:8px}
  #lt-title{font-weight:700;font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  #lt-sub{font-size:12px;color:#b3b3b3}
  #lt-controls{display:flex;gap:8px}
  #lt-controls select{flex:1;background:#2a2a2a;color:#fff;border:0;border-radius:6px;padding:6px;font-size:12px}
  #lt-lines{flex:1;overflow-y:auto;padding:16px 14px 40vh;scroll-behavior:smooth}
  .lt-line{padding:6px 4px;border-radius:6px;opacity:.45;transition:opacity .2s;cursor:default}
  .lt-line.synced{cursor:pointer}.lt-line.synced:hover{background:#ffffff10}
  .lt-line.active{opacity:1}
  .lt-line.past{opacity:.7}
  #lt-panel.unsynced .lt-line{opacity:1}
  .lt-orig{font-size:16px;font-weight:700;line-height:1.35}
  .lt-trans{font-size:14px;color:#1ed760;line-height:1.35;margin-top:2px}
  .lt-gap{height:14px}
  #lt-msg{padding:24px 14px;color:#b3b3b3;font-size:14px}`;
  document.head.appendChild(style);

  const panel = document.createElement("div");
  panel.id = "lt-panel";
  panel.hidden = !state.open;
  panel.innerHTML = `
    <div id="lt-head">
      <div id="lt-title">Lyrics Translator</div>
      <div id="lt-sub"></div>
      <div id="lt-controls">
        <select id="lt-lang" title="Zielsprache"></select>
        <select id="lt-mode" title="Anzeige">
          <option value="both">Original + Übersetzung</option>
          <option value="translation">Nur Übersetzung</option>
          <option value="original">Nur Original</option>
        </select>
      </div>
    </div>
    <div id="lt-msg"></div>
    <div id="lt-lines"></div>`;
  document.body.appendChild(panel);

  const $ = (id) => panel.querySelector("#" + id);
  const langSelect = $("lt-lang");
  const modeSelect = $("lt-mode");
  const linesEl = $("lt-lines");
  const msgEl = $("lt-msg");

  let displayNames = null;
  try {
    displayNames = new Intl.DisplayNames([S.Locale ? S.Locale.getLocale() : navigator.language], { type: "language" });
  } catch (_) {}
  const langName = (c) => {
    try {
      return (displayNames && displayNames.of(NAME_ALIASES[c] || c)) || c;
    } catch (_) {
      return c;
    }
  };
  LANGUAGES.map((c) => [c, langName(c)])
    .sort((a, b) => a[1].localeCompare(b[1]))
    .forEach(([c, n]) => langSelect.add(new Option(n, c)));
  if (!LANGUAGES.includes(state.targetLang)) state.targetLang = "de";
  langSelect.value = state.targetLang;
  modeSelect.value = state.mode;

  langSelect.addEventListener("change", () => {
    state.targetLang = langSelect.value;
    store.set("lang", state.targetLang);
    runTranslation();
  });
  modeSelect.addEventListener("change", () => {
    state.mode = modeSelect.value;
    store.set("mode", state.mode);
    if (state.mode !== "original" && !state.translations) runTranslation();
    else render();
  });

  function setMessage(text) {
    msgEl.textContent = text || "";
    msgEl.hidden = !text;
  }

  function render() {
    linesEl.innerHTML = "";
    state.activeIndex = -1;
    const lyrics = state.lyrics;
    if (!lyrics) return;
    panel.classList.toggle("unsynced", !lyrics.synced);
    lyrics.lines.forEach((line, i) => {
      if (!line.text) {
        const gap = document.createElement("div");
        gap.className = "lt-gap lt-line";
        linesEl.appendChild(gap);
        return;
      }
      const div = document.createElement("div");
      div.className = "lt-line" + (lyrics.synced ? " synced" : "");
      const trans = state.translations && state.translations[i];
      if (state.mode !== "translation" || !trans) {
        const o = document.createElement("div");
        o.className = "lt-orig";
        o.textContent = line.text;
        div.appendChild(o);
      }
      if (state.mode !== "original" && trans && trans !== line.text) {
        const t = document.createElement("div");
        t.className = state.mode === "translation" ? "lt-orig" : "lt-trans";
        t.textContent = trans;
        div.appendChild(t);
      }
      if (lyrics.synced) div.addEventListener("click", () => S.Player.seek(line.time));
      linesEl.appendChild(div);
    });
  }

  function tick() {
    const lyrics = state.lyrics;
    if (!lyrics || !lyrics.synced || panel.hidden) return;
    const pos = S.Player.getProgress() + 250;
    let idx = -1;
    for (let i = 0; i < lyrics.lines.length; i++) {
      if (lyrics.lines[i].time <= pos) idx = i;
      else break;
    }
    if (idx === state.activeIndex) return;
    state.activeIndex = idx;
    const els = linesEl.children;
    for (let i = 0; i < els.length; i++) {
      els[i].classList.toggle("active", i === idx);
      els[i].classList.toggle("past", i < idx);
    }
    if (els[idx]) linesEl.scrollTop = els[idx].offsetTop - linesEl.clientHeight / 3;
  }
  setInterval(tick, 200);

  async function runTranslation() {
    const lyrics = state.lyrics;
    state.translations = null;
    render();
    if (!lyrics || state.mode === "original") return;
    const key = state.trackKey + "|" + state.targetLang;
    const req = state.requestId;
    try {
      let result = translationCache.get(key);
      if (!result) {
        $("lt-sub").textContent = `${lyrics.source}${lyrics.synced ? " · synchron" : ""} · übersetze…`;
        result = await translateLines(lyrics.lines, state.targetLang);
        translationCache.set(key, result);
      }
      if (req !== state.requestId || state.targetLang !== key.split("|").pop()) return;
      const already = result.detected && result.detected.split("-")[0] === state.targetLang.split("-")[0];
      $("lt-sub").textContent =
        `${lyrics.source}${lyrics.synced ? " · synchron" : ""}` +
        (result.detected ? ` · ${langName(result.detected)} → ${langName(state.targetLang)}` : "") +
        (already ? " (schon in dieser Sprache)" : "");
      state.translations = already ? null : result.lines;
      render();
    } catch (e) {
      if (req === state.requestId) $("lt-sub").textContent = `${lyrics.source} · ${e.message}`;
    }
  }

  async function loadTrack() {
    const track = currentTrack();
    const key = track ? track.uri || `${track.artist}|${track.title}` : null;
    if (key === state.trackKey) return;
    state.trackKey = key;
    const req = ++state.requestId;
    state.lyrics = null;
    state.translations = null;
    render();
    if (!track) {
      $("lt-title").textContent = "Lyrics Translator";
      $("lt-sub").textContent = "";
      setMessage("Spiele einen Song ab.");
      return;
    }
    $("lt-title").textContent = `${track.title} – ${track.artist}`;
    $("lt-sub").textContent = "";
    setMessage("Suche Lyrics…");
    const lyrics = await findLyrics(track);
    if (req !== state.requestId) return;
    if (!lyrics) {
      setMessage("Für diesen Song wurden keine Lyrics gefunden.");
      return;
    }
    setMessage("");
    state.lyrics = lyrics;
    $("lt-sub").textContent = `${lyrics.source}${lyrics.synced ? " · synchron" : ""}`;
    runTranslation();
  }

  const icon = `<svg role="img" height="16" width="16" viewBox="0 0 24 24" fill="currentColor"><path d="M12.87 15.07l-2.54-2.51.03-.03A17.52 17.52 0 0014.07 6H17V4h-7V2H8v2H1v2h11.17C11.5 7.92 10.44 9.75 9 11.35 8.07 10.32 7.3 9.19 6.69 8h-2c.73 1.63 1.73 3.17 2.98 4.56l-5.09 5.02L4 19l5-5 3.11 3.11.76-2.04zM18.5 10h-2L12 22h2l1.12-3h4.75L21 22h2l-4.5-12zm-2.62 7l1.62-4.33L19.12 17h-3.24z"/></svg>`;
  new S.Topbar.Button("Lyrics übersetzen", icon, () => {
    panel.hidden = !panel.hidden;
    store.set("open", String(!panel.hidden));
    if (!panel.hidden) {
      state.activeIndex = -1;
      tick();
    }
  });

  S.Player.addEventListener("songchange", loadTrack);
  loadTrack();
})();
