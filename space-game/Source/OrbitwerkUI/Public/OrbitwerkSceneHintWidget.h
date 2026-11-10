// Orbitwerk - OrbitwerkUI
//
// Einfaches Overlay (oben links) mit Szenenname und Bedienhinweis.
// Platzhalter fuer Hangar/Flug, bis T14/T20 das echte HUD bzw. den Editor liefern.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OrbitwerkSceneHintWidget.generated.h"

class UTextBlock;

UCLASS()
class ORBITWERKUI_API UOrbitwerkSceneHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|HUD")
	void SetHint(const FText& InTitle, const FText& InHint);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintTextBlock;

	FText PendingTitle;
	FText PendingHint;
};
