"use strict";

// Google-Translate-Sprachcodes. Namen werden über Intl.DisplayNames in der Gerätesprache angezeigt.
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
const SCOPES = "user-read-currently-playing user-read-playback-state";
const REDIRECT_URI = location.origin + location.pathname;

const store = {
  get: (k, d) => {
    try {
      const v = localStorage.getItem("lt:" + k);
      return v == null ? d : v;
    } catch (_) {
      return d;
    }
  },
  set: (k, v) => {
    try {
      v == null ? localStorage.removeItem("lt:" + k) : localStorage.setItem("lt:" + k, v);
    } catch (_) {}
  },
};

const state = {
  targetLang: store.get("lang", navigator.language.split("-")[0]),
  mode: store.get("mode", "both"),
  trackKey: null,
  lyrics: null,
  translations: null,
  activeIndex: -1,
  requestId: 0,
  playback: null, // { progressMs, isPlaying, fetchedAt }
};
const translationCache = new Map();

const $ = (id) => document.getElementById(id);

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
  const params = new URLSearchParams({ artist_name: track.artist, track_name: track.title });
  if (track.album) params.set("album_name", track.album);
  if (track.durationMs) params.set("duration", String(Math.round(track.durationMs / 1000)));
  if (track.album && track.durationMs) {
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
      console.warn(source.name, e);
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

// ---------- Spotify (OAuth PKCE) ----------

const b64url = (buf) => btoa(String.fromCharCode(...new Uint8Array(buf))).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");

async function login() {
  const clientId = $("client-id").value.trim();
  if (!clientId) {
    alert("Bitte zuerst die Client ID eintragen.");
    return;
  }
  store.set("clientId", clientId);
  const verifier = b64url(crypto.getRandomValues(new Uint8Array(48)));
  store.set("verifier", verifier);
  const challenge = b64url(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(verifier)));
  location.href =
    "https://accounts.spotify.com/authorize?" +
    new URLSearchParams({
      client_id: clientId,
      response_type: "code",
      redirect_uri: REDIRECT_URI,
      code_challenge_method: "S256",
      code_challenge: challenge,
      scope: SCOPES,
    });
}

function logout() {
  ["accessToken", "refreshToken", "expiresAt"].forEach((k) => store.set(k, null));
  state.trackKey = null;
  state.playback = null;
  showTrack(null);
}

async function tokenRequest(body) {
  const res = await fetch("https://accounts.spotify.com/api/token", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams({ client_id: store.get("clientId", ""), ...body }),
  });
  if (!res.ok) throw new Error("Spotify-Anmeldung fehlgeschlagen (" + res.status + ")");
  const t = await res.json();
  store.set("accessToken", t.access_token);
  if (t.refresh_token) store.set("refreshToken", t.refresh_token);
  store.set("expiresAt", String(Date.now() + (t.expires_in - 60) * 1000));
}

async function handleRedirect() {
  const params = new URLSearchParams(location.search);
  if (!params.has("code") && !params.has("error")) return;
  history.replaceState(null, "", REDIRECT_URI);
  if (params.has("error")) {
    setMessage("Spotify-Anmeldung abgebrochen: " + params.get("error"));
    return;
  }
  await tokenRequest({
    grant_type: "authorization_code",
    code: params.get("code"),
    redirect_uri: REDIRECT_URI,
    code_verifier: store.get("verifier", ""),
  });
  store.set("verifier", null);
}

async function accessToken() {
  if (!store.get("refreshToken")) return null;
  if (Date.now() > Number(store.get("expiresAt", 0))) {
    await tokenRequest({ grant_type: "refresh_token", refresh_token: store.get("refreshToken") });
  }
  return store.get("accessToken");
}

async function pollSpotify() {
  let token;
  try {
    token = await accessToken();
  } catch (e) {
    setMessage(e.message + " – bitte in den Einstellungen neu verbinden.");
    logout();
    return;
  }
  if (!token) return;
  try {
    const res = await fetch("https://api.spotify.com/v1/me/player/currently-playing", {
      headers: { Authorization: "Bearer " + token },
    });
    if (res.status === 401) {
      store.set("expiresAt", "0");
      return;
    }
    if (res.status === 204 || !res.ok) {
      if (!state.manual) showTrack(null, "In Spotify läuft gerade nichts.");
      return;
    }
    const data = await res.json();
    const item = data.item;
    if (!item || data.currently_playing_type !== "track") return;
    // Ein manuell gesuchter Song bleibt stehen, bis in Spotify der nächste Song startet.
    if (state.manual && item.id === state.lastSpotifyId) return;
    state.lastSpotifyId = item.id;
    state.playback = { progressMs: data.progress_ms || 0, isPlaying: data.is_playing, fetchedAt: Date.now() };
    state.manual = false;
    showTrack({
      key: item.id,
      title: item.name,
      artist: item.artists.map((a) => a.name)[0],
      album: item.album && item.album.name,
      durationMs: item.duration_ms,
    });
  } catch (e) {
    console.warn(e);
  }
}

// ---------- UI ----------

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

function setMessage(text) {
  $("msg").textContent = text || "";
}

function subline(extra) {
  const l = state.lyrics;
  return l ? `${l.source}${l.synced ? " · synchron" : ""}${extra ? " · " + extra : ""}` : extra || "";
}

function render() {
  const linesEl = $("lines");
  linesEl.innerHTML = "";
  state.activeIndex = -1;
  const lyrics = state.lyrics;
  if (!lyrics) return;
  const syncedNow = lyrics.synced && state.playback;
  document.body.classList.toggle("unsynced", !syncedNow);
  lyrics.lines.forEach((line, i) => {
    const div = document.createElement("div");
    if (!line.text) {
      div.className = "line gap";
      linesEl.appendChild(div);
      return;
    }
    div.className = "line";
    const trans = state.translations && state.translations[i];
    if (state.mode !== "translation" || !trans) {
      const o = document.createElement("div");
      o.className = "orig";
      o.textContent = line.text;
      div.appendChild(o);
    }
    if (state.mode !== "original" && trans && trans !== line.text) {
      const t = document.createElement("div");
      t.className = state.mode === "translation" ? "orig" : "trans";
      t.textContent = trans;
      div.appendChild(t);
    }
    linesEl.appendChild(div);
  });
}

function tick() {
  const lyrics = state.lyrics;
  const pb = state.playback;
  if (!lyrics || !lyrics.synced || !pb) return;
  const pos = pb.progressMs + (pb.isPlaying ? Date.now() - pb.fetchedAt : 0) + 300;
  let idx = -1;
  for (let i = 0; i < lyrics.lines.length; i++) {
    if (lyrics.lines[i].time <= pos) idx = i;
    else break;
  }
  if (idx === state.activeIndex) return;
  state.activeIndex = idx;
  const els = $("lines").children;
  for (let i = 0; i < els.length; i++) {
    els[i].classList.toggle("active", i === idx);
    els[i].classList.toggle("past", i < idx);
  }
  const main = document.querySelector("main");
  if (els[idx]) main.scrollTop = els[idx].offsetTop - main.offsetTop - main.clientHeight / 3;
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
      $("sub").textContent = subline("übersetze…");
      result = await translateLines(lyrics.lines, lang);
      translationCache.set(key, result);
    }
    if (req !== state.requestId || lang !== state.targetLang) return;
    const already = result.detected && result.detected.split("-")[0] === lang.split("-")[0];
    $("sub").textContent = subline(
      result.detected ? `${langName(result.detected)} → ${langName(lang)}${already ? " (schon in dieser Sprache)" : ""}` : ""
    );
    state.translations = already ? null : result.lines;
    render();
  } catch (e) {
    if (req === state.requestId) $("sub").textContent = subline(e.message);
  }
}

async function showTrack(track, emptyMessage) {
  const key = track ? track.key || `${track.artist}|${track.title}` : null;
  if (key === state.trackKey) return;
  state.trackKey = key;
  const req = ++state.requestId;
  state.lyrics = null;
  state.translations = null;
  render();
  if (!track) {
    $("title").textContent = "Lyrics Translator";
    $("sub").textContent = "";
    setMessage(emptyMessage || (store.get("refreshToken") ? "Spiele einen Song in Spotify ab." : "Tippe auf ⚙︎, um Spotify zu verbinden oder einen Song zu suchen."));
    return;
  }
  $("title").textContent = `${track.title} – ${track.artist}`;
  $("sub").textContent = "";
  setMessage("Suche Lyrics…");
  const lyrics = await findLyrics(track);
  if (req !== state.requestId) return;
  if (!lyrics) {
    setMessage("Für diesen Song wurden keine Lyrics gefunden.");
    return;
  }
  setMessage("");
  state.lyrics = lyrics;
  $("sub").textContent = subline();
  runTranslation();
}

function setupUi() {
  const langSelect = $("lang");
  LANGUAGES.map((c) => [c, langName(c)])
    .sort((a, b) => a[1].localeCompare(b[1]))
    .forEach(([c, n]) => langSelect.add(new Option(n, c)));
  if (!LANGUAGES.includes(state.targetLang)) state.targetLang = "de";
  langSelect.value = state.targetLang;
  $("mode").value = state.mode;

  langSelect.addEventListener("change", () => {
    state.targetLang = langSelect.value;
    store.set("lang", state.targetLang);
    runTranslation();
  });
  $("mode").addEventListener("change", () => {
    state.mode = $("mode").value;
    store.set("mode", state.mode);
    if (state.mode !== "original" && !state.translations) runTranslation();
    else render();
  });

  $("redirect-uri").textContent = REDIRECT_URI;
  $("client-id").value = store.get("clientId", "");
  $("settings-btn").addEventListener("click", () => $("settings").showModal());
  $("login").addEventListener("click", login);
  $("logout").addEventListener("click", () => {
    logout();
    $("settings").close();
  });
  $("search").addEventListener("click", () => {
    const artist = $("q-artist").value.trim();
    const title = $("q-title").value.trim();
    if (!artist || !title) return;
    state.manual = true;
    state.playback = null;
    $("settings").close();
    showTrack({ title, artist });
  });
}

async function main() {
  setupUi();
  if ("serviceWorker" in navigator) navigator.serviceWorker.register("sw.js").catch(() => {});
  try {
    await handleRedirect();
  } catch (e) {
    setMessage(e.message);
  }
  showTrack(null);
  pollSpotify();
  setInterval(() => {
    if (!document.hidden) pollSpotify();
  }, 4000);
  document.addEventListener("visibilitychange", () => !document.hidden && pollSpotify());
  setInterval(tick, 200);
}

main();
