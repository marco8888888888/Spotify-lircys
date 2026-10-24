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
 * die Media-Session von Spotify (Titel, Künstler, Cover, Position) mitzulesen und zu steuern.
 */
public class SpotifyListener extends NotificationListenerService {
    static final String SPOTIFY = "com.spotify.music";

    private MediaSessionManager sessions;
    private MediaController controller;
    private NowPlaying now;

    private final MediaSessionManager.OnActiveSessionsChangedListener sessionsChanged = this::attach;

    private final MediaController.Callback callback = new MediaController.Callback() {
        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            now.onMetadata(metadata);
        }

        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            now.onPlayback(state);
        }

        @Override
        public void onSessionDestroyed() {
            detach();
            now.setController(null);
        }
    };

    @Override
    public void onListenerConnected() {
        now = NowPlaying.get(this);
        Overlay.get(this).start();
        sessions = getSystemService(MediaSessionManager.class);
        ComponentName me = new ComponentName(this, SpotifyListener.class);
        sessions.addOnActiveSessionsChangedListener(sessionsChanged, me);
        attach(sessions.getActiveSessions(me));
    }

    @Override
    public void onListenerDisconnected() {
        if (sessions != null) sessions.removeOnActiveSessionsChangedListener(sessionsChanged);
        detach();
        if (now != null) now.setController(null);
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
        now.setController(controller);
    }

    private void detach() {
        if (controller != null) controller.unregisterCallback(callback);
        controller = null;
    }
}
