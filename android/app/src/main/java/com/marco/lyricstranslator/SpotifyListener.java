package com.marco.lyricstranslator;

import android.content.ComponentName;
import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.service.notification.NotificationListenerService;

import java.util.List;

/**
 * Wird vom System gestartet, sobald die App „Benachrichtigungszugriff“ hat. Nur damit erlaubt Android,
 * die Media-Session von Spotify (Titel, Künstler, Position) mitzulesen.
 */
public class SpotifyListener extends NotificationListenerService {
    private static final String SPOTIFY = "com.spotify.music";

    private MediaSessionManager sessions;
    private MediaController controller;
    private Overlay overlay;

    private final MediaSessionManager.OnActiveSessionsChangedListener sessionsChanged = this::attach;

    private final MediaController.Callback callback = new MediaController.Callback() {
        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            pushMetadata(metadata);
        }

        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            overlay.onPlayback(state);
        }

        @Override
        public void onSessionDestroyed() {
            detach();
            overlay.onTrack(null, null, null, 0);
        }
    };

    @Override
    public void onListenerConnected() {
        overlay = Overlay.get(this);
        sessions = getSystemService(MediaSessionManager.class);
        ComponentName me = new ComponentName(this, SpotifyListener.class);
        sessions.addOnActiveSessionsChangedListener(sessionsChanged, me);
        attach(sessions.getActiveSessions(me));
    }

    @Override
    public void onListenerDisconnected() {
        if (sessions != null) sessions.removeOnActiveSessionsChangedListener(sessionsChanged);
        detach();
        if (overlay != null) overlay.onTrack(null, null, null, 0);
    }

    private void attach(List<MediaController> active) {
        MediaController spotify = null;
        if (active != null) {
            for (MediaController c : active) {
                if (SPOTIFY.equals(c.getPackageName())) {
                    spotify = c;
                    break;
                }
            }
        }
        if (spotify == null) return; // alte Session behalten, bis sie wirklich endet
        if (controller != null && controller.getSessionToken().equals(spotify.getSessionToken())) return;
        detach();
        controller = spotify;
        controller.registerCallback(callback);
        pushMetadata(controller.getMetadata());
        overlay.onPlayback(controller.getPlaybackState());
    }

    private void detach() {
        if (controller != null) controller.unregisterCallback(callback);
        controller = null;
    }

    private void pushMetadata(MediaMetadata m) {
        if (m == null) return;
        String title = m.getString(MediaMetadata.METADATA_KEY_TITLE);
        String artist = m.getString(MediaMetadata.METADATA_KEY_ARTIST);
        String album = m.getString(MediaMetadata.METADATA_KEY_ALBUM);
        long duration = m.getLong(MediaMetadata.METADATA_KEY_DURATION);
        if (title == null || artist == null) return;
        overlay.onTrack(title, artist.split(",")[0].trim(), album == null ? "" : album, duration);
    }
}
