// Lyrics Translator – wird vom Installer direkt in die Spotify-Desktop-App (Apps/xpui.spa) eingebaut.
// Zeigt für jeden Song synchronisierte Lyrics an und übersetzt sie in jede Sprache.

(function () {
  "use strict";

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

  // ---------- Wiedergabe-Status abgreifen ----------
  // Dieses Script läuft vor Spotifys eigenem Code. So können wir mitlesen, welche Position Spotify
  // an das Betriebssystem meldet (Media Session), und brauchen keine internen Spotify-APIs.

  const playback = { posMs: 0, at: 0, rate: 0, fromSession: false };
  const ms = navigator.mediaSession;
  if (ms && typeof ms.setPositionState === "function") {
    const original = ms.setPositionState.bind(ms);
    ms.setPositionState = function (s) {
      if (s && typeof s.position === "number") {
        playback.posMs = s.position * 1000;
        playback.rate = typeof s.playbackRate === "number" ? s.playbackRate : 1;
        playback.at = performance.now();
        playback.fromSession = true;
      }
      return original(s);
    };
  }

  const q = (sel) => document.querySelector(sel);
  const parseClock = (t) => {
    const p = (t || "").trim().split(":").map(Number);
    return p.some(isNaN) || !p.length ? null : p.reduce((a, b) => a * 60 + b, 0) * 1000;
  };

  function isPlaying() {
    if (ms && ms.playbackState && ms.playbackState !== "none") return ms.playbackState === "playing";
    const btn = q('[data-testid="control-button-playpause"]');
    const label = (btn && btn.getAttribute("aria-label")) || "";
    // Der Knopf zeigt „Pause“, solange Musik läuft (in jeder Sprache ähnlich geschrieben).
    return /pau/i.test(label);
  }

  // Fallback ohne Media Session: Zeitanzeige im Player (Sekundengenau, dazwischen hochgerechnet).
  let lastClock = null;
  function readDomPosition() {
    const t = parseClock(q('[data-testid="playback-position"]')?.textContent);
    if (t == null) return;
    if (t !== lastClock) {
      lastClock = t;
      playback.posMs = t;
      playback.at = performance.now();
    }
    playback.rate = isPlaying() ? 1 : 0;
  }

  function currentPositionMs() {
    if (!playback.fromSession || performance.now() - playback.at > 5000) readDomPosition();
    const elapsed = (performance.now() - playback.at) * (isPlaying() ? playback.rate || 1 : 0);
    // DOM-Wert ist nur sekundengenau – nicht weiter als eine Sekunde vorrechnen.
    return playback.posMs + (playback.fromSession ? elapsed : Math.min(elapsed, 999));
  }

  function currentTrack() {
    const md = ms && ms.metadata;
    let title = md && md.title;
    let artist = md && md.artist;
    let album = (md && md.album) || "";
    if (!title || !artist) {
      title = q('[data-testid="now-playing-widget"] [data-testid="context-item-info-title"]')?.textContent;
      artist = q('[data-testid="now-playing-widget"] [data-testid="context-item-info-artist"]')?.textContent;
    }
    if (!title || !artist) return null;
    return {
      title: title.trim(),
      artist: artist.split(",")[0].trim(),
      album,
      durationMs: parseClock(q('[data-testid="playback-duration"]')?.textContent) || 0,
    };
  }

  // ---------- Lyrics-Quellen ----------

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

  const plainToLines = (text) => text.split(/\r?\n/).map((t) => ({ time: null, text: t.trim() }));

  function lrclibResult(r) {
    if (!r) return null;
    if (r.syncedLyrics) return { synced: true, source: "LRCLIB", lines: parseLrc(r.syncedLyrics) };
    if (r.plainLyrics) return { synced: false, source: "LRCLIB", lines: plainToLines(r.plainLyrics) };
    return null;
  }

  async function fromLrclib(track) {
    const base = "https://lrclib.net/api";
    if (track.album && track.durationMs) {
      const params = new URLSearchParams({
        artist_name: track.artist,
        track_name: track.title,
        album_name: track.album,
        duration: String(Math.round(track.durationMs / 1000)),
      });
      const res = await fetch(`${base}/get?${params}`);
      if (res.ok) {
        const hit = lrclibResult(await res.json());
        if (hit) return hit;
      }
    }
    // Unscharfe Suche: Titel ohne Zusätze wie "(feat. …)" oder "- Remastered".
    const cleanTitle = track.title.replace(/\s*[([].*?[)\]]\s*/g, " ").replace(/\s+-\s+.*$/, "").trim();
    let res = await fetch(`${base}/search?${new URLSearchParams({ track_name: cleanTitle, artist_name: track.artist })}`);
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
    for (const source of [fromLrclib, fromLyricsOvh]) {
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
    const chunks = [[]];
    let size = 0;
    lines.forEach((l, i) => {
      if (!l.text) return;
      if (size + l.text.length > 1800 && chunks[chunks.length - 1].length) {
        chunks.push([]);
        size = 0;
      }
      chunks[chunks.length - 1].push(i);
      size += l.text.length + 1;
    });
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

  const store = {
    get(k, d) {
      try {
        const v = localStorage.getItem("lyrics-translator:" + k);
        return v == null ? d : v;
      } catch (_) {
        return d;
      }
    },
    set(k, v) {
      try {
        localStorage.setItem("lyrics-translator:" + k, v);
      } catch (_) {}
    },
  };

  const state = {
    targetLang: store.get("lang", (navigator.language || "de").split("-")[0]),
    mode: store.get("mode", "both"), // both | translation | original
    trackKey: null,
    lyrics: null,
    translations: null,
    activeIndex: -1,
    requestId: 0,
  };
  const translationCache = new Map();

  // ---------- Übersetzung direkt in Spotifys Lyrics-Ansicht ----------
  // Spotify zeigt seine Lyrics als einzelne Zeilen-Elemente. Wir übersetzen genau den Text, den Spotify anzeigt,
  // und hängen die Übersetzung in jede Zeile ein – so übernimmt sie Spotifys Schrift, Farbe und Hervorhebung.

  const LINE_SELECTOR = '[data-testid="fullscreen-lyric"], [data-testid="lyrics-line"], .lyrics-lyricsContent-lyric';
  const inline = {
    on: store.get("inline", "true") === "true",
    cache: new Map(), // "lang\u0000text" -> Übersetzung ("" = schon in Zielsprache)
    busy: false,
    queued: false,
    lineCount: 0,
  };

  function ownText(el) {
    let t = "";
    for (const n of el.childNodes) {
      if (n.nodeType === 1 && n.classList.contains("lt-inline")) continue;
      t += n.textContent;
    }
    return t.replace(/\s+/g, " ").trim();
  }

  function clearInline() {
    document.querySelectorAll(".lt-inline").forEach((n) => n.remove());
    document.querySelectorAll(".lt-only").forEach((n) => n.classList.remove("lt-only"));
  }

  function scanInline() {
    const els = document.querySelectorAll(LINE_SELECTOR);
    inline.lineCount = els.length;
    if (!inline.on || state.mode === "original") {
      clearInline();
      return;
    }
    const lang = state.targetLang;
    const missing = new Set();
    for (const el of els) {
      const text = ownText(el);
      let node = el.querySelector(":scope > .lt-inline");
      const trans = text ? inline.cache.get(lang + "\u0000" + text) : "";
      if (trans === undefined) missing.add(text);
      if (!trans) {
        if (node) node.remove();
        el.classList.remove("lt-only");
        continue;
      }
      if (!node) {
        node = document.createElement("div");
        node.className = "lt-inline";
        el.appendChild(node);
      }
      if (node.textContent !== trans) node.textContent = trans;
      const only = state.mode === "translation";
      if (only && !el.classList.contains("lt-only")) el.style.setProperty("--lt-fs", getComputedStyle(el).fontSize);
      el.classList.toggle("lt-only", only);
    }
    if (missing.size) translateMissing([...missing], lang);
  }

  async function translateMissing(texts, lang) {
    if (inline.busy) {
      inline.queued = true;
      return;
    }
    inline.busy = true;
    try {
      const r = await translateLines(texts.map((text) => ({ text })), lang);
      const already = r.detected && r.detected.split("-")[0] === lang.split("-")[0];
      texts.forEach((t, i) => {
        const out = r.lines[i] || "";
        inline.cache.set(lang + "\u0000" + t, already || out === t ? "" : out);
      });
    } catch (e) {
      console.warn("[Lyrics Translator]", e);
      // Später nochmal versuchen, aber nicht in einer Endlosschleife.
      await new Promise((res) => setTimeout(res, 5000));
    } finally {
      inline.busy = false;
    }
    if (inline.queued) inline.queued = false;
    scanInline();
  }

  let scanTimer = null;
  function queueScan() {
    if (scanTimer) return;
    scanTimer = setTimeout(() => {
      scanTimer = null;
      if (toggleBtn) placeToggle();
      scanInline();
    }, 200);
  }

  const ICON = `<svg height="16" width="16" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M12.87 15.07l-2.54-2.51.03-.03A17.52 17.52 0 0014.07 6H17V4h-7V2H8v2H1v2h11.17C11.5 7.92 10.44 9.75 9 11.35 8.07 10.32 7.3 9.19 6.69 8h-2c.73 1.63 1.73 3.17 2.98 4.56l-5.09 5.02L4 19l5-5 3.11 3.11.76-2.04zM18.5 10h-2L12 22h2l1.12-3h4.75L21 22h2l-4.5-12zm-2.62 7l1.62-4.33L19.12 17h-3.24z"/></svg>`;

  const CSS = `
  #lt-panel{position:fixed;top:64px;right:16px;bottom:100px;width:min(420px,40vw);z-index:9999;display:flex;flex-direction:column;
    background:rgba(18,18,18,.97);color:#fff;border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,.6);font-family:inherit;overflow:hidden}
  #lt-panel[hidden]{display:none}
  #lt-head{padding:12px 14px;border-bottom:1px solid #2a2a2a;display:flex;flex-direction:column;gap:8px}
  #lt-titlerow{display:flex;gap:8px;align-items:center}
  #lt-title{flex:1;font-weight:700;font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  #lt-close{background:none;border:0;color:#b3b3b3;font-size:18px;cursor:pointer;line-height:1}
  #lt-sub{font-size:12px;color:#b3b3b3}
  #lt-controls{display:flex;gap:8px}
  #lt-controls select{flex:1;background:#2a2a2a;color:#fff;border:0;border-radius:6px;padding:6px;font-size:12px}
  #lt-lines{flex:1;overflow-y:auto;padding:16px 14px 40vh;scroll-behavior:smooth}
  .lt-line{padding:6px 4px;opacity:.45;transition:opacity .2s}
  .lt-line.past{opacity:.7}.lt-line.active{opacity:1}
  #lt-panel.unsynced .lt-line{opacity:1}
  .lt-orig{font-size:16px;font-weight:700;line-height:1.35}
  .lt-trans{font-size:14px;color:#1ed760;line-height:1.35;margin-top:2px}
  .lt-gap{height:14px}
  #lt-msg{padding:24px 14px;color:#b3b3b3;font-size:14px}
  #lt-msg:empty{display:none}
  #lt-toggle{position:relative;background:none;border:0;color:#b3b3b3;cursor:pointer;padding:8px;display:inline-flex;align-items:center}
  #lt-toggle:hover{color:#fff;transform:scale(1.04)}
  #lt-toggle.on{color:#1ed760}
  #lt-toggle.on::after{content:"";position:absolute;bottom:2px;left:50%;width:4px;height:4px;margin-left:-2px;border-radius:50%;background:#1ed760}
  #lt-toggle.floating{position:fixed;right:20px;bottom:100px;z-index:9999;background:#1ed760;color:#000;border-radius:50%;padding:12px;box-shadow:0 4px 16px rgba(0,0,0,.5)}

  /* Übersetzung in Spotifys eigenen Lyrics-Zeilen: erbt Farbe und Hervorhebung der Zeile */
  .lt-inline{display:block;font-size:.62em;font-weight:600;line-height:1.3;margin-top:.2em;opacity:.85;color:inherit;letter-spacing:0;pointer-events:none}
  .lt-only{font-size:0!important}
  .lt-only>:not(.lt-inline){display:none!important}
  .lt-only>.lt-inline{font-size:var(--lt-fs,1em)!important;font-weight:inherit;opacity:1;margin:0;line-height:inherit}

  /* Kleines Menü über dem Knopf, im Stil von Spotifys Kontextmenüs */
  #lt-pop{position:fixed;right:16px;bottom:96px;z-index:10000;width:300px;background:#282828;color:#fff;border-radius:6px;
    box-shadow:0 16px 24px rgba(0,0,0,.3),0 6px 8px rgba(0,0,0,.2);padding:14px;font-size:14px;display:flex;flex-direction:column;gap:12px}
  #lt-pop[hidden]{display:none}
  #lt-pop h3{margin:0;font-size:14px;font-weight:700;display:flex;align-items:center;gap:8px}
  #lt-pop h3 svg{color:#1ed760}
  #lt-pop label{display:flex;flex-direction:column;gap:6px;font-size:12px;color:#b3b3b3}
  #lt-pop select{background:#3e3e3e;color:#fff;border:0;border-radius:4px;padding:8px;font-size:13px}
  #lt-pop .lt-row{display:flex;align-items:center;justify-content:space-between;gap:12px;font-size:14px;color:#fff}
  #lt-pop .lt-hint{font-size:12px;color:#b3b3b3;line-height:1.4}
  #lt-pop .lt-hint:empty{display:none}
  #lt-pop .lt-btn{background:transparent;color:#fff;border:1px solid #878787;border-radius:999px;padding:7px 14px;font-size:13px;font-weight:700;cursor:pointer}
  #lt-pop .lt-btn:hover{border-color:#fff;transform:scale(1.02)}
  .lt-switch{position:relative;width:40px;height:22px;flex:none;appearance:none;-webkit-appearance:none;background:#727272;border-radius:11px;cursor:pointer;transition:background .2s;margin:0}
  .lt-switch::before{content:"";position:absolute;top:3px;left:3px;width:16px;height:16px;border-radius:50%;background:#fff;transition:transform .2s}
  .lt-switch:checked{background:#1ed760}
  .lt-switch:checked::before{transform:translateX(18px)}`;

  let panel, linesEl, toggleBtn, pop;
  const $ = (id) => panel.querySelector("#" + id);

  let displayNames = null;
  try {
    displayNames = new Intl.DisplayNames([navigator.language], { type: "language" });
  } catch (_) {}
  const langName = (c) => {
    try {
      return (displayNames && displayNames.of(NAME_ALIASES[c] || c)) || c;
    } catch (_) {
      return c;
    }
  };

  function buildUi() {
    const style = document.createElement("style");
    style.textContent = CSS;
    document.head.appendChild(style);

    panel = document.createElement("div");
    panel.id = "lt-panel";
    panel.hidden = store.get("open", "false") !== "true";
    panel.innerHTML = `
      <div id="lt-head">
        <div id="lt-titlerow"><div id="lt-title">Lyrics Translator</div><button id="lt-close" title="Schließen">✕</button></div>
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
    linesEl = $("lt-lines");

    // Menü am Übersetzen-Knopf
    pop = document.createElement("div");
    pop.id = "lt-pop";
    pop.hidden = true;
    pop.innerHTML = `
      <h3>${ICON}Lyrics übersetzen</h3>
      <div class="lt-row">In Spotifys Lyrics anzeigen<input type="checkbox" class="lt-switch" id="lt-inline-on"></div>
      <label>Übersetzen in<select id="lt-pop-lang"></select></label>
      <label>Anzeige
        <select id="lt-pop-mode">
          <option value="both">Original + Übersetzung</option>
          <option value="translation">Nur Übersetzung</option>
          <option value="original">Nur Original</option>
        </select>
      </label>
      <div class="lt-hint" id="lt-pop-hint"></div>
      <button class="lt-btn" id="lt-pop-panel">Eigene Lyrics-Ansicht öffnen</button>`;
    document.body.appendChild(pop);

    const langSelects = [$("lt-lang"), pop.querySelector("#lt-pop-lang")];
    const modeSelects = [$("lt-mode"), pop.querySelector("#lt-pop-mode")];
    const sortedLangs = LANGUAGES.map((c) => [c, langName(c)]).sort((a, b) => a[1].localeCompare(b[1]));
    if (!LANGUAGES.includes(state.targetLang)) state.targetLang = "de";
    for (const s of langSelects) {
      sortedLangs.forEach(([c, n]) => s.add(new Option(n, c)));
      s.value = state.targetLang;
      s.addEventListener("change", () => {
        state.targetLang = s.value;
        store.set("lang", state.targetLang);
        langSelects.forEach((o) => (o.value = s.value));
        runTranslation();
        scanInline();
      });
    }
    for (const s of modeSelects) {
      s.value = state.mode;
      s.addEventListener("change", () => {
        state.mode = s.value;
        store.set("mode", state.mode);
        modeSelects.forEach((o) => (o.value = s.value));
        clearInline();
        scanInline();
        if (state.mode !== "original" && !state.translations) runTranslation();
        else render();
      });
    }
    const inlineSwitch = pop.querySelector("#lt-inline-on");
    inlineSwitch.checked = inline.on;
    inlineSwitch.addEventListener("change", () => setInline(inlineSwitch.checked));
    pop.querySelector("#lt-pop-panel").addEventListener("click", () => {
      pop.hidden = true;
      setOpen(true);
    });
    $("lt-close").addEventListener("click", () => setOpen(false));

    document.addEventListener("keydown", (e) => {
      if (e.ctrlKey && e.shiftKey && e.key.toLowerCase() === "l") setInline(!inline.on);
      if (e.key === "Escape") pop.hidden = true;
    });
    document.addEventListener("mousedown", (e) => {
      if (!pop.hidden && !pop.contains(e.target) && !toggleBtn.contains(e.target)) pop.hidden = true;
    });

    toggleBtn = document.createElement("button");
    toggleBtn.id = "lt-toggle";
    toggleBtn.title = "Lyrics übersetzen (Strg+Umschalt+L)";
    toggleBtn.innerHTML = ICON;
    toggleBtn.addEventListener("click", () => {
      pop.hidden = !pop.hidden;
      if (!pop.hidden) updateHint();
    });
    placeToggle();
    // Spotify baut seine Leiste beim Start (und manchmal danach) neu – Knopf bei Bedarf wieder einsetzen.
    setInterval(placeToggle, 1500);

    new MutationObserver(queueScan).observe(document.body, { childList: true, subtree: true, characterData: true });
  }

  function setInline(on) {
    inline.on = on;
    store.set("inline", String(on));
    pop.querySelector("#lt-inline-on").checked = on;
    toggleBtn.classList.toggle("on", on);
    scanInline();
  }

  function updateHint() {
    const lyricsBtn = q('[data-testid="lyrics-button"]');
    const noSpotifyLyrics = lyricsBtn && (lyricsBtn.disabled || lyricsBtn.getAttribute("aria-disabled") === "true");
    let hint = "";
    if (noSpotifyLyrics) hint = "Spotify hat für diesen Song keine Lyrics. Nutze die eigene Lyrics-Ansicht – sie sucht in weiteren Quellen.";
    else if (!inline.lineCount) hint = "Öffne in Spotify die Lyrics (Mikrofon-Knopf). Die Übersetzung erscheint dann unter jeder Zeile.";
    pop.querySelector("#lt-pop-hint").textContent = hint;
  }

  // Knopf neben Spotifys eigenen Lyrics-Knopf setzen; falls es den nicht gibt, schwebend unten rechts.
  function placeToggle() {
    if (!panel.isConnected) document.body.appendChild(panel);
    if (!pop.isConnected) document.body.appendChild(pop);
    const anchor = q('[data-testid="lyrics-button"]');
    if (anchor && anchor.parentElement) {
      if (toggleBtn.previousElementSibling !== anchor) {
        toggleBtn.classList.remove("floating");
        anchor.insertAdjacentElement("afterend", toggleBtn);
      }
    } else if (!toggleBtn.isConnected || !toggleBtn.classList.contains("floating")) {
      toggleBtn.classList.add("floating");
      document.body.appendChild(toggleBtn);
    }
    toggleBtn.classList.toggle("on", inline.on);
  }

  function setOpen(open) {
    panel.hidden = !open;
    store.set("open", String(open));
    state.activeIndex = -1;
    if (open) loadTrack();
  }

  function setMessage(text) {
    $("lt-msg").textContent = text || "";
  }

  function subline(extra) {
    const l = state.lyrics;
    return l ? `${l.source}${l.synced ? " · synchron" : ""}${extra ? " · " + extra : ""}` : extra || "";
  }

  function render() {
    linesEl.innerHTML = "";
    state.activeIndex = -1;
    const lyrics = state.lyrics;
    if (!lyrics) return;
    panel.classList.toggle("unsynced", !lyrics.synced);
    lyrics.lines.forEach((line, i) => {
      const div = document.createElement("div");
      if (!line.text) {
        div.className = "lt-line lt-gap";
        linesEl.appendChild(div);
        return;
      }
      div.className = "lt-line";
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
      linesEl.appendChild(div);
    });
  }

  function tick() {
    const lyrics = state.lyrics;
    if (!lyrics || !lyrics.synced || panel.hidden) return;
    const pos = currentPositionMs() + 250;
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

  async function runTranslation() {
    const lyrics = state.lyrics;
    state.translations = null;
    render();
    if (!lyrics || state.mode === "original") return;
    const lang = state.targetLang;
    const key = state.trackKey + "|" + lang;
    const req = state.requestId;
    try {
      let result = translationCache.get(key);
      if (!result) {
        $("lt-sub").textContent = subline("übersetze…");
        result = await translateLines(lyrics.lines, lang);
        translationCache.set(key, result);
      }
      if (req !== state.requestId || lang !== state.targetLang) return;
      const already = result.detected && result.detected.split("-")[0] === lang.split("-")[0];
      $("lt-sub").textContent = subline(
        result.detected ? `${langName(result.detected)} → ${langName(lang)}${already ? " (schon in dieser Sprache)" : ""}` : ""
      );
      state.translations = already ? null : result.lines;
      render();
    } catch (e) {
      if (req === state.requestId) $("lt-sub").textContent = subline(e.message);
    }
  }

  async function loadTrack() {
    if (panel.hidden) return;
    const track = currentTrack();
    const key = track ? `${track.artist}|${track.title}` : null;
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
    // Kurz warten, bis Spotify Länge und Album des neuen Songs anzeigt.
    await new Promise((r) => setTimeout(r, 400));
    const lyrics = await findLyrics(currentTrack() || track);
    if (req !== state.requestId) return;
    if (!lyrics) {
      setMessage("Für diesen Song wurden keine Lyrics gefunden.");
      return;
    }
    setMessage("");
    state.lyrics = lyrics;
    $("lt-sub").textContent = subline();
    runTranslation();
  }

  function start() {
    buildUi();
    setInterval(loadTrack, 1000);
    setInterval(tick, 200);
    loadTrack();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", start);
  else start();
})();
