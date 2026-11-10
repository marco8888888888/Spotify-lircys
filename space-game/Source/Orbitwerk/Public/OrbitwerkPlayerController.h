// Orbitwerk - Haupt-Spielmodul
//
// PlayerController fuer alle Szenen:
//   * Hauptmenue (oder unbekannte Map): Menue-Widget, Mauszeiger, UI-Eingabe, Erststart-Benchmark
//   * Hangar / Flug: Platzhalter-Overlay, Spiel-Eingabe, "Zurueck" -> Hauptmenue
//
// Enhanced Input: Die Basis-Aktionen werden zur Laufzeit in C++ erzeugt (keine .uasset noetig).
// Spaetere Tasks (T14 Flugsteuerung, T20 Hangar) koennen eigene Mapping Contexts
// ueber AdditionalMappingContexts (Blueprint-Unterklasse) oder eigenen Code hinzufuegen.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OrbitwerkMenuTypes.h"
#include "OrbitwerkSceneSubsystem.h"
#include "OrbitwerkPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UOrbitwerkMainMenuWidget;
class UOrbitwerkSceneHintWidget;
struct FInputActionValue;

UCLASS()
class ORBITWERK_API AOrbitwerkPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOrbitwerkPlayerController();

	UFUNCTION(BlueprintPure, Category = "Orbitwerk")
	EOrbitwerkScene GetScene() const { return CurrentScene; }

	/** Zurueck ins Hauptmenue (Hangar/Flug). Auch per Taste Esc / Backspace. */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk")
	void ReturnToMainMenu();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	/** Widget-Klasse des Hauptmenues (Default: reine C++-Klasse). */
	UPROPERTY(EditDefaultsOnly, Category = "Orbitwerk|UI")
	TSubclassOf<UOrbitwerkMainMenuWidget> MainMenuWidgetClass;

	/** Overlay fuer Hangar/Flug (Default: reine C++-Klasse). */
	UPROPERTY(EditDefaultsOnly, Category = "Orbitwerk|UI")
	TSubclassOf<UOrbitwerkSceneHintWidget> SceneHintWidgetClass;

	/** Zusaetzliche Mapping Contexts (Assets), werden mit Prioritaet 1 hinzugefuegt. */
	UPROPERTY(EditDefaultsOnly, Category = "Orbitwerk|Input")
	TArray<TObjectPtr<UInputMappingContext>> AdditionalMappingContexts;

private:
	void EnsureInputObjects();
	void AddMappingContexts();
	void SetupSceneUI();
	void ShowMainMenu();
	void ShowSceneHint(const FText& InTitle, const FText& InHint);
	void ClearActiveWidget();

	void HandleBackAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleMenuAction(EOrbitwerkMenuAction Action);

	/** "Zurueck" (Esc / Backspace). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BackAction;

	/** Basis-Mapping-Context dieser Klasse (Prioritaet 0). */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> BaseMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveWidget;

	EOrbitwerkScene CurrentScene = EOrbitwerkScene::Unknown;
};
