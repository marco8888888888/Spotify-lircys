// Orbitwerk - OrbitwerkRender

#include "OrbitwerkGraphicsSettings.h"

#include "OrbitwerkRender.h"

#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Scalability.h"

#define LOCTEXT_NAMESPACE "OrbitwerkGraphicsSettings"

namespace OrbitwerkGraphics
{
	/** Feste Eckdaten je Preset. Feintuning der CVars steht in DefaultScalability.ini. */
	struct FPresetDefinition
	{
		int32 ScalabilityLevel;   // 0 = Low .. 4 = Cinematic
		float ResolutionScale;    // Prozent der Ausgabeaufloesung (TSR rechnet hoch)
		float FrameRateLimit;     // 0 = unbegrenzt
		const TCHAR* SectionKey;  // [OrbitwerkPreset.<SectionKey>]
	};

	static FPresetDefinition GetPresetDefinition(EOrbitwerkGraphicsPreset Preset)
	{
		switch (Preset)
		{
		case EOrbitwerkGraphicsPreset::Laptop:    return { 0,  58.f, 60.f, TEXT("Laptop") };
		case EOrbitwerkGraphicsPreset::Low:       return { 0,  70.f,  0.f, TEXT("Low") };
		case EOrbitwerkGraphicsPreset::Medium:    return { 1,  80.f,  0.f, TEXT("Medium") };
		case EOrbitwerkGraphicsPreset::High:      return { 2,  90.f,  0.f, TEXT("High") };
		case EOrbitwerkGraphicsPreset::Epic:      return { 3, 100.f,  0.f, TEXT("Epic") };
		case EOrbitwerkGraphicsPreset::Cinematic: return { 4, 100.f,  0.f, TEXT("Cinematic") };
		default:                                  break;
		}
		return { 3, 100.f, 0.f, TEXT("Epic") };
	}

	/** Ab dieser GI-/Reflexions-Stufe ist Lumen aktiv (siehe DefaultScalability.ini). */
	static constexpr int32 LumenMinQualityLevel = 2;
}

UOrbitwerkGraphicsSettings::UOrbitwerkGraphicsSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UOrbitwerkGraphicsSettings* UOrbitwerkGraphicsSettings::Get()
{
	return GEngine ? Cast<UOrbitwerkGraphicsSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UOrbitwerkGraphicsSettings::ApplyPreset(EOrbitwerkGraphicsPreset Preset)
{
	if (Preset == EOrbitwerkGraphicsPreset::Custom)
	{
		CurrentPreset = EOrbitwerkGraphicsPreset::Custom;
		SaveSettings();
		return;
	}

	const OrbitwerkGraphics::FPresetDefinition Definition = OrbitwerkGraphics::GetPresetDefinition(Preset);

	SetOverallScalabilityLevel(Definition.ScalabilityLevel);
	SetResolutionScaleValueEx(Definition.ResolutionScale);
	SetFrameRateLimit(Definition.FrameRateLimit);

	CurrentPreset = Preset;
	BasePreset = Preset;

	if (GIsEditor)
	{
		// Im Editor (PIE) keine Fenster-/Aufloesungsaenderung anfassen.
		ApplyNonResolutionSettings();
		SaveSettings();
	}
	else
	{
		// ApplySettings -> ApplyResolutionSettings + ApplyNonResolutionSettings (unsere CVars) + SaveSettings.
		ApplySettings(false);
	}

	UE_LOG(LogOrbitwerkRender, Log, TEXT("Grafik-Preset angewendet: %s (Stufe %d, Aufloesung %.0f %%, FPS-Limit %.0f)"),
		Definition.SectionKey, Definition.ScalabilityLevel, Definition.ResolutionScale, Definition.FrameRateLimit);
}

EOrbitwerkGraphicsPreset UOrbitwerkGraphicsSettings::GetNextPreset(EOrbitwerkGraphicsPreset Preset)
{
	switch (Preset)
	{
	case EOrbitwerkGraphicsPreset::Laptop:    return EOrbitwerkGraphicsPreset::Low;
	case EOrbitwerkGraphicsPreset::Low:       return EOrbitwerkGraphicsPreset::Medium;
	case EOrbitwerkGraphicsPreset::Medium:    return EOrbitwerkGraphicsPreset::High;
	case EOrbitwerkGraphicsPreset::High:      return EOrbitwerkGraphicsPreset::Epic;
	case EOrbitwerkGraphicsPreset::Epic:      return EOrbitwerkGraphicsPreset::Cinematic;
	case EOrbitwerkGraphicsPreset::Cinematic: return EOrbitwerkGraphicsPreset::Laptop;
	default:                                  break;
	}
	return EOrbitwerkGraphicsPreset::Laptop;
}

FText UOrbitwerkGraphicsSettings::GetPresetDisplayName(EOrbitwerkGraphicsPreset Preset)
{
	switch (Preset)
	{
	case EOrbitwerkGraphicsPreset::Laptop:    return LOCTEXT("PresetLaptop", "Laptop");
	case EOrbitwerkGraphicsPreset::Low:       return LOCTEXT("PresetLow", "Niedrig");
	case EOrbitwerkGraphicsPreset::Medium:    return LOCTEXT("PresetMedium", "Mittel");
	case EOrbitwerkGraphicsPreset::High:      return LOCTEXT("PresetHigh", "Hoch");
	case EOrbitwerkGraphicsPreset::Epic:      return LOCTEXT("PresetEpic", "Episch");
	case EOrbitwerkGraphicsPreset::Cinematic: return LOCTEXT("PresetCinematic", "Kino");
	case EOrbitwerkGraphicsPreset::Custom:    return LOCTEXT("PresetCustom", "Benutzerdefiniert");
	default:                                  break;
	}
	return FText::GetEmpty();
}

EOrbitwerkGraphicsPreset UOrbitwerkGraphicsSettings::RunBenchmarkAndSuggestPreset(int32 WorkScale)
{
	// RunHardwareBenchmark ueberschreibt ScalabilityQuality mit den Benchmark-Stufen.
	// Wir wollen nur einen Vorschlag -> vorher sichern, danach zuruecksetzen.
	const Scalability::FQualityLevels SavedQuality = ScalabilityQuality;

	RunHardwareBenchmark(WorkScale);

	const int32 Levels[] =
	{
		GetViewDistanceQuality(),
		GetShadowQuality(),
		GetGlobalIlluminationQuality(),
		GetReflectionQuality(),
		GetPostProcessingQuality(),
		GetTextureQuality(),
		GetVisualEffectQuality(),
		GetShadingQuality()
	};

	float Sum = 0.f;
	for (const int32 LevelValue : Levels)
	{
		Sum += static_cast<float>(FMath::Clamp(LevelValue, 0, 4));
	}
	const float Average = Sum / static_cast<float>(UE_ARRAY_COUNT(Levels));

	const float CpuResult = GetLastCPUBenchmarkResult();
	const float GpuResult = GetLastGPUBenchmarkResult();

	ScalabilityQuality = SavedQuality;

	// Benchmark liefert max. Stufe 3 (Episch). Kino schlagen wir nie automatisch vor.
	EOrbitwerkGraphicsPreset Suggested = EOrbitwerkGraphicsPreset::Medium;
	if (GpuResult <= 0.f && CpuResult <= 0.f)
	{
		Suggested = EOrbitwerkGraphicsPreset::Medium; // Benchmark fehlgeschlagen -> sicherer Mittelweg
	}
	else if (Average < 0.5f)
	{
		Suggested = EOrbitwerkGraphicsPreset::Laptop;
	}
	else if (Average < 1.25f)
	{
		Suggested = EOrbitwerkGraphicsPreset::Low;
	}
	else if (Average < 2.0f)
	{
		Suggested = EOrbitwerkGraphicsPreset::Medium;
	}
	else if (Average < 2.75f)
	{
		Suggested = EOrbitwerkGraphicsPreset::High;
	}
	else
	{
		Suggested = EOrbitwerkGraphicsPreset::Epic;
	}

	UE_LOG(LogOrbitwerkRender, Log, TEXT("Benchmark: CPU %.1f, GPU %.1f, mittlere Stufe %.2f -> Vorschlag %s"),
		CpuResult, GpuResult, Average, *GetPresetDisplayName(Suggested).ToString());

	return Suggested;
}

bool UOrbitwerkGraphicsSettings::RunFirstStartBenchmarkIfNeeded()
{
	if (bInitialBenchmarkDone || GIsEditor)
	{
		return false;
	}

	const EOrbitwerkGraphicsPreset Suggested = RunBenchmarkAndSuggestPreset();
	bInitialBenchmarkDone = true;
	ApplyPreset(Suggested); // speichert auch bInitialBenchmarkDone
	return true;
}

void UOrbitwerkGraphicsSettings::SetLumenEnabled(bool bEnable)
{
	const int32 MinLevel = OrbitwerkGraphics::LumenMinQualityLevel;
	if (bEnable)
	{
		SetGlobalIlluminationQuality(FMath::Max(GetGlobalIlluminationQuality(), MinLevel));
		SetReflectionQuality(FMath::Max(GetReflectionQuality(), MinLevel));
	}
	else
	{
		SetGlobalIlluminationQuality(FMath::Min(GetGlobalIlluminationQuality(), MinLevel - 1));
		SetReflectionQuality(FMath::Min(GetReflectionQuality(), MinLevel - 1));
	}

	CurrentPreset = EOrbitwerkGraphicsPreset::Custom;
	ApplyNonResolutionSettings();
	SaveSettings();
}

bool UOrbitwerkGraphicsSettings::IsLumenEnabled() const
{
	return GetGlobalIlluminationQuality() >= OrbitwerkGraphics::LumenMinQualityLevel;
}

void UOrbitwerkGraphicsSettings::SetToDefaults()
{
	Super::SetToDefaults();

	// Engine-Default ist Stufe 3 (Episch) -> passt zum Preset Episch.
	CurrentPreset = EOrbitwerkGraphicsPreset::Epic;
	BasePreset = EOrbitwerkGraphicsPreset::Epic;
	bInitialBenchmarkDone = false;
}

void UOrbitwerkGraphicsSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	ApplyPresetCVarSection(CurrentPreset == EOrbitwerkGraphicsPreset::Custom ? BasePreset : CurrentPreset);
}

void UOrbitwerkGraphicsSettings::ApplyPresetCVarSection(EOrbitwerkGraphicsPreset Preset) const
{
	if (!GConfig || Preset == EOrbitwerkGraphicsPreset::Custom)
	{
		return;
	}

	const FString SectionName = FString::Printf(TEXT("OrbitwerkPreset.%s"),
		OrbitwerkGraphics::GetPresetDefinition(Preset).SectionKey);

	TArray<FString> Lines;
	if (!GConfig->GetSection(*SectionName, Lines, GScalabilityIni))
	{
		UE_LOG(LogOrbitwerkRender, Warning, TEXT("Sektion [%s] fehlt in DefaultScalability.ini."), *SectionName);
		return;
	}

	for (const FString& Line : Lines)
	{
		FString Key;
		FString Value;
		if (!Line.Split(TEXT("="), &Key, &Value))
		{
			continue;
		}
		Key.TrimStartAndEndInline();
		Value.TrimStartAndEndInline();

		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Key))
		{
			// Scalability-Prioritaet: ueberschreibt die Engine-Gruppen (gleiche Prioritaet, spaeter gesetzt),
			// laesst aber Projekt-, Ini- und Konsolen-Overrides in Ruhe.
			CVar->Set(*Value, ECVF_SetByScalability);
		}
		else
		{
			UE_LOG(LogOrbitwerkRender, Verbose, TEXT("[%s] CVar '%s' (noch) nicht registriert - uebersprungen."), *SectionName, *Key);
		}
	}
}

#undef LOCTEXT_NAMESPACE
