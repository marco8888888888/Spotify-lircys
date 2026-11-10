// Orbitwerk - OrbitwerkRender

#include "OrbitwerkQuality.h"

#include "HAL/IConsoleManager.h"

namespace OrbitwerkQualityCVars
{
	static TAutoConsoleVariable<int32> CVarClouds(
		TEXT("ow.Clouds.Quality"), 2,
		TEXT("Orbitwerk: Qualitaet volumetrischer Wolken (0 = aus/billig .. 4 = Kino)."),
		ECVF_Scalability);

	static TAutoConsoleVariable<int32> CVarParticles(
		TEXT("ow.Particles.Quality"), 2,
		TEXT("Orbitwerk: Partikeldichte (0 = minimal .. 4 = Kino)."),
		ECVF_Scalability);

	static TAutoConsoleVariable<int32> CVarAtmosphere(
		TEXT("ow.Atmosphere.Quality"), 2,
		TEXT("Orbitwerk: Atmosphaere/Himmel (0 = billig .. 4 = Kino)."),
		ECVF_Scalability);

	static TAutoConsoleVariable<int32> CVarPlanet(
		TEXT("ow.Planet.Quality"), 2,
		TEXT("Orbitwerk: Planeten-LOD/Terrain-Detail (0 = grob .. 4 = Kino)."),
		ECVF_Scalability);

	static TAutoConsoleVariable<int32> CVarPlume(
		TEXT("ow.Plume.Quality"), 2,
		TEXT("Orbitwerk: Triebwerksflammen/Rauch (0 = billig .. 4 = Kino)."),
		ECVF_Scalability);

	static TAutoConsoleVariable<int32> CVarReentry(
		TEXT("ow.Reentry.Quality"), 2,
		TEXT("Orbitwerk: Wiedereintritts-Plasma/Hitzeflimmern (0 = aus .. 4 = Kino)."),
		ECVF_Scalability);
}

const TCHAR* UOrbitwerkQualityLibrary::GetCVarName(EOrbitwerkEffect Effect)
{
	switch (Effect)
	{
	case EOrbitwerkEffect::Clouds:     return TEXT("ow.Clouds.Quality");
	case EOrbitwerkEffect::Particles:  return TEXT("ow.Particles.Quality");
	case EOrbitwerkEffect::Atmosphere: return TEXT("ow.Atmosphere.Quality");
	case EOrbitwerkEffect::Planet:     return TEXT("ow.Planet.Quality");
	case EOrbitwerkEffect::Plume:      return TEXT("ow.Plume.Quality");
	case EOrbitwerkEffect::Reentry:    return TEXT("ow.Reentry.Quality");
	default:                           break;
	}
	return TEXT("");
}

int32 UOrbitwerkQualityLibrary::GetEffectQuality(EOrbitwerkEffect Effect)
{
	int32 Value = 2;
	switch (Effect)
	{
	case EOrbitwerkEffect::Clouds:     Value = OrbitwerkQualityCVars::CVarClouds.GetValueOnAnyThread(); break;
	case EOrbitwerkEffect::Particles:  Value = OrbitwerkQualityCVars::CVarParticles.GetValueOnAnyThread(); break;
	case EOrbitwerkEffect::Atmosphere: Value = OrbitwerkQualityCVars::CVarAtmosphere.GetValueOnAnyThread(); break;
	case EOrbitwerkEffect::Planet:     Value = OrbitwerkQualityCVars::CVarPlanet.GetValueOnAnyThread(); break;
	case EOrbitwerkEffect::Plume:      Value = OrbitwerkQualityCVars::CVarPlume.GetValueOnAnyThread(); break;
	case EOrbitwerkEffect::Reentry:    Value = OrbitwerkQualityCVars::CVarReentry.GetValueOnAnyThread(); break;
	default:                           break;
	}
	return FMath::Clamp(Value, OrbitwerkQuality::MinQuality, OrbitwerkQuality::MaxQuality);
}

void UOrbitwerkQualityLibrary::SetEffectQuality(EOrbitwerkEffect Effect, int32 Quality)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(GetCVarName(Effect)))
	{
		// Gleiche Prioritaet wie die Presets, damit ein spaeterer Preset-Wechsel wieder greift.
		CVar->Set(FMath::Clamp(Quality, OrbitwerkQuality::MinQuality, OrbitwerkQuality::MaxQuality), ECVF_SetByScalability);
	}
}
