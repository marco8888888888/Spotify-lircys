"""
Orbitwerk - legt die leeren Szenen-Maps an (Unreal-Python, im Editor ausfuehren).

    /Game/Maps/MainMenu   Hauptmenue
    /Game/Maps/Hangar     Hangar (Raketen-Editor, T20)
    /Game/Maps/Flight     Flug (T21-T23)

Jede Map bekommt nur einen PlayerStart. Bereits vorhandene Maps werden NICHT
ueberschrieben - das Skript kann gefahrlos mehrfach laufen.

Ausfuehren:
  * Editor-Menue  Tools > Execute Python Script...  ->  Scripts/create_maps.py
  * oder Output Log, Eingabezeile auf "Python" stellen:
        py "<Pfad>/space-game/Scripts/create_maps.py"

Voraussetzung: Plugins "Python Editor Script Plugin" und "Editor Scripting Utilities"
(beide in Orbitwerk.uproject aktiviert).
"""

import unreal

MAP_DIRECTORY = "/Game/Maps"

# (Name, Origin-Rebasing aktivieren)
MAPS = [
    ("MainMenu", False),
    ("Hangar", False),
    ("Flight", True),
]


def _log(message):
    unreal.log("[Orbitwerk] " + message)


def _warn(message):
    unreal.log_warning("[Orbitwerk] " + message)


def _error(message):
    unreal.log_error("[Orbitwerk] " + message)


def _find_world_settings():
    """WorldSettings der aktuell offenen Editor-Map (verschiedene API-Wege, je nach UE-Version)."""
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if world is not None and hasattr(world, "get_world_settings"):
            settings = world.get_world_settings()
            if settings is not None:
                return settings
    except Exception:  # noqa: BLE001 - nur Fallback
        pass

    try:
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        for actor in actors:
            if isinstance(actor, unreal.WorldSettings):
                return actor
    except Exception:  # noqa: BLE001 - nur Fallback
        pass

    return None


def _configure_world(enable_origin_rebasing):
    settings = _find_world_settings()
    if settings is None:
        _warn("WorldSettings nicht gefunden - Origin-Rebasing bitte ggf. manuell setzen.")
        return
    try:
        settings.set_editor_property("enable_world_origin_rebasing", enable_origin_rebasing)
    except Exception as exc:  # noqa: BLE001
        _warn("Origin-Rebasing konnte nicht gesetzt werden: {}".format(exc))


def _create_map(name, enable_origin_rebasing):
    asset_path = "{}/{}".format(MAP_DIRECTORY, name)

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        _log("{} existiert bereits - uebersprungen.".format(asset_path))
        return True

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Leere Map anlegen, speichern und oeffnen.
    if not level_subsystem.new_level(asset_path):
        _error("Konnte {} nicht anlegen.".format(asset_path))
        return False

    actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(0.0, 0.0, 100.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

    _configure_world(enable_origin_rebasing)

    if not level_subsystem.save_current_level():
        _error("Konnte {} nicht speichern.".format(asset_path))
        return False

    _log("{} angelegt.".format(asset_path))
    return True


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(MAP_DIRECTORY):
        unreal.EditorAssetLibrary.make_directory(MAP_DIRECTORY)

    ok = True
    for name, rebasing in MAPS:
        ok = _create_map(name, rebasing) and ok

    # Zum Schluss das Hauptmenue oeffnen (Startmap von Spiel und Editor).
    main_menu = "{}/MainMenu".format(MAP_DIRECTORY)
    if unreal.EditorAssetLibrary.does_asset_exist(main_menu):
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(main_menu)

    if ok:
        _log("Fertig: MainMenu, Hangar, Flight sind vorhanden.")
    else:
        _error("Mindestens eine Map konnte nicht angelegt werden - siehe Output Log.")


main()
