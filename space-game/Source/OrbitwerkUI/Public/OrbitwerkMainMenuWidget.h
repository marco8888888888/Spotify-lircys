// Orbitwerk - OrbitwerkUI
//
// Hauptmenue. Das Layout wird komplett in C++ aufgebaut (kein Widget-Blueprint noetig).
// Eine Blueprint-Unterklasse mit eigenem Layout ist moeglich; dann wird das C++-Layout
// nicht erzeugt und die Buttons muessen dort an die Handle*-Funktionen gebunden werden.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OrbitwerkMenuTypes.h"
#include "OrbitwerkMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class ORBITWERKUI_API UOrbitwerkMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Wird ausgeloest, wenn der Spieler Hangar / Flug / Beenden waehlt. */
	UPROPERTY(BlueprintAssignable, Category = "Orbitwerk|Menu")
	FOrbitwerkMenuActionSignature OnMenuAction;

	/** Aktualisiert die Anzeige des aktuellen Grafik-Presets. */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void RefreshGraphicsLabel();

	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void HandleHangarClicked();

	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void HandleFlightClicked();

	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void HandleGraphicsClicked();

	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void HandleBenchmarkClicked();

	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Menu")
	void HandleQuitClicked();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();
	UButton* AddMenuButton(UVerticalBox* Box, const FText& InLabel, UTextBlock** OutLabel = nullptr);
	void SetStatus(const FText& InStatus);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GraphicsLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLabel;
};
