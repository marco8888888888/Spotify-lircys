// Orbitwerk - OrbitwerkRender
//
// Zentrale Qualitaetsstufen fuer eigene Effekte (PROMPT.md Abschnitt 4):
// Jeder visuelle Effekt liest seine Stufe 0..4 aus einer CVar "ow.<Effekt>.Quality".
// Stufe 0 = billig oder aus. Die Presets in Config/DefaultScalability.ini
// ([OrbitwerkPreset.<Name>]) setzen diese CVars.
//
// Neue Effekte: Eintrag in EOrbitwerkEffect + CVar in OrbitwerkQuality.cpp ergaenzen.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OrbitwerkQuality.generated.h"

namespace OrbitwerkQuality
{
	inline constexpr int32 MinQuality = 0;
	inline constexpr int32 MaxQuality = 4;
}

UENUM(BlueprintType)
enum class EOrbitwerkEffect : uint8
{
	Clouds      UMETA(DisplayName = "Wolken"),         // ow.Clouds.Quality
	Particles   UMETA(DisplayName = "Partikel"),       // ow.Particles.Quality
	Atmosphere  UMETA(DisplayName = "Atmosphaere"),    // ow.Atmosphere.Quality
	Planet      UMETA(DisplayName = "Planeten"),       // ow.Planet.Quality
	Plume       UMETA(DisplayName = "Triebwerke"),     // ow.Plume.Quality
	Reentry     UMETA(DisplayName = "Wiedereintritt")  // ow.Reentry.Quality
};

UCLASS()
class ORBITWERKRENDER_API UOrbitwerkQualityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Aktuelle Qualitaetsstufe 0..4 des Effekts (thread-sicher lesbar). */
	UFUNCTION(BlueprintPure, Category = "Orbitwerk|Graphics")
	static int32 GetEffectQuality(EOrbitwerkEffect Effect);

	/** Setzt die Stufe 0..4 (wird beim naechsten Preset-Wechsel ueberschrieben). */
	UFUNCTION(BlueprintCallable, Category = "Orbitwerk|Graphics")
	static void SetEffectQuality(EOrbitwerkEffect Effect, int32 Quality);

	/** Name der zugehoerigen Konsolenvariable, z. B. TEXT("ow.Clouds.Quality"). */
	static const TCHAR* GetCVarName(EOrbitwerkEffect Effect);
};
