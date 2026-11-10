// Orbitwerk - Haupt-Spielmodul
//
// Szenenwechsel per C++: Hauptmenue / Hangar / Flug.
// Jede Szene ist eine eigene Map unter /Game/Maps (angelegt von Scripts/create_maps.py).
// Die aktive Szene wird aus dem Namen der geladenen Map abgeleitet; unbekannte Maps
// (z. B. eine leere Editor-Map) verhalten sich wie das Hauptmenue.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OrbitwerkSceneSubsystem.generated.h"

UENUM(BlueprintType)
enum class EOrbitwerkScene : uint8
{
	Unknown   UMETA(DisplayName = "Unbekannt"),
	MainMenu  UMETA(DisplayName = "Hauptmenue"),
	Hangar    UMETA(DisplayName = "Hangar"),
	Flight    UMETA(DisplayName = "Flug")
};

UCLASS()
class ORBITWERK_API UOrbitwerkSceneSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Bequemer Zugriff aus jedem Objekt mit Welt (Actor, Widget, ...). */
	static UOrbitwerkSceneSubsystem* Get(const UObject* WorldContextObject);

	/** Laedt die Map der Szene. false, wenn die Map fehlt (dann Fehler im Log + auf dem Bildschirm). */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Scenes")
	bool OpenScene(EOrbitwerkScene Scene);

	/** Szene der aktuell geladenen Map. */
	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Scenes")
	EOrbitwerkScene GetCurrentScene() const;

	/** Langer Paketname der Map, z. B. "/Game/Maps/Hangar". Leer fuer Unknown. */
	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Scenes")
	static FString GetMapPathForScene(EOrbitwerkScene Scene);

	/** Kurzer Map-Name ("Hangar") oder langer Pfad -> Szene. */
	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Scenes")
	static EOrbitwerkScene GetSceneForMapName(const FString& MapName);

	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Scenes")
	static bool DoesSceneMapExist(EOrbitwerkScene Scene);
};
