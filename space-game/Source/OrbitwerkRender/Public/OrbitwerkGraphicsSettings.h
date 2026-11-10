// Orbitwerk - OrbitwerkRender
//
// Grafik-Einstellungen (PROMPT.md Abschnitt 4).
// Aktiv durch Config/DefaultEngine.ini:
//   [/Script/Engine.Engine] GameUserSettingsClassName=/Script/OrbitwerkRender.OrbitwerkGraphicsSettings
//
// Presets: Laptop / Niedrig / Mittel / Hoch / Episch / Kino (+ Benutzerdefiniert).
// Ein Preset setzt:
//   1. die Engine-Scalability-Stufe (0..4) -> Config/DefaultScalability.ini [<Gruppe>@<Stufe>]
//      (Lumen an/aus haengt an GlobalIllumination-/Reflection-Stufe: 0-1 = aus (SSGI/SSR), 2-4 = Lumen)
//   2. Aufloesungsskala (TSR rechnet hoch) und FPS-Limit
//   3. die CVar-Sektion [OrbitwerkPreset.<Name>] aus DefaultScalability.ini (ow.*-Effekte, Nanite-Detail)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "OrbitwerkGraphicsSettings.generated.h"

UENUM(BlueprintType)
enum class EOrbitwerkGraphicsPreset : uint8
{
	Laptop      UMETA(DisplayName = "Laptop"),
	Low         UMETA(DisplayName = "Niedrig"),
	Medium      UMETA(DisplayName = "Mittel"),
	High        UMETA(DisplayName = "Hoch"),
	Epic        UMETA(DisplayName = "Episch"),
	Cinematic   UMETA(DisplayName = "Kino"),
	Custom      UMETA(DisplayName = "Benutzerdefiniert")
};

UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class ORBITWERKRENDER_API UOrbitwerkGraphicsSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UOrbitwerkGraphicsSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Aktive Instanz (nullptr, falls GameUserSettingsClassName nicht auf diese Klasse zeigt). */
	static UOrbitwerkGraphicsSettings* Get();

	// ---- Presets -------------------------------------------------------------------------

	/** Wendet ein Preset an, uebernimmt es sofort und speichert (GameUserSettings.ini). */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Graphics")
	void ApplyPreset(EOrbitwerkGraphicsPreset Preset);

	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	EOrbitwerkGraphicsPreset GetCurrentPreset() const { return CurrentPreset; }

	/** Naechstes Preset in der Reihenfolge Laptop -> ... -> Kino -> Laptop (Custom -> Laptop). */
	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	static EOrbitwerkGraphicsPreset GetNextPreset(EOrbitwerkGraphicsPreset Preset);

	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	static FText GetPresetDisplayName(EOrbitwerkGraphicsPreset Preset);

	// ---- Benchmark -----------------------------------------------------------------------

	/**
	 * Fuehrt den Engine-Hardware-Benchmark aus und leitet daraus ein Preset ab.
	 * Wendet NICHTS an (die aktuellen Einstellungen bleiben unveraendert).
	 */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Graphics")
	EOrbitwerkGraphicsPreset RunBenchmarkAndSuggestPreset(int32 WorkScale = 10);

	/**
	 * Erststart: Wenn noch nie gelaufen, Benchmark ausfuehren, vorgeschlagenes Preset anwenden
	 * und merken. Im Editor (PIE) wird nichts gemacht, damit der Editor nicht umgestellt wird.
	 * @return true, wenn der Benchmark jetzt gelaufen ist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Graphics")
	bool RunFirstStartBenchmarkIfNeeded();

	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	bool HasInitialBenchmarkRun() const { return bInitialBenchmarkDone; }

	// ---- Einzel-Einstellungen ------------------------------------------------------------

	/** Lumen an (GI/Reflexionen >= Stufe 2) oder aus (Fallback SSGI/SSR). Setzt Preset auf Custom. */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Graphics")
	void SetLumenEnabled(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	bool IsLumenEnabled() const;

	// ---- UGameUserSettings ---------------------------------------------------------------

	virtual void SetToDefaults() override;
	virtual void ApplyNonResolutionSettings() override;

private:
	/** Wendet die CVar-Sektion [OrbitwerkPreset.<Name>] aus DefaultScalability.ini an. */
	void ApplyPresetCVarSection(EOrbitwerkGraphicsPreset Preset) const;

	/** Aktuelles Preset (Custom, sobald Einzelwerte geaendert wurden). */
	UPROPERTY(Config)
	EOrbitwerkGraphicsPreset CurrentPreset = EOrbitwerkGraphicsPreset::Epic;

	/** Letztes echtes Preset; dessen CVar-Sektion gilt auch im Custom-Modus weiter. */
	UPROPERTY(Config)
	EOrbitwerkGraphicsPreset BasePreset = EOrbitwerkGraphicsPreset::Epic;

	UPROPERTY(Config)
	bool bInitialBenchmarkDone = false;
};
