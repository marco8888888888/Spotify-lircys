// Orbitwerk - Haupt-Spielmodul

#include "OrbitwerkSceneSubsystem.h"

#include "Orbitwerk.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

namespace OrbitwerkScenes
{
	static const TCHAR* MapDirectory = TEXT("/Game/Maps/");
	static const TCHAR* MainMenuMap = TEXT("MainMenu");
	static const TCHAR* HangarMap = TEXT("Hangar");
	static const TCHAR* FlightMap = TEXT("Flight");
}

UOrbitwerkSceneSubsystem* UOrbitwerkSceneSubsystem::Get(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UOrbitwerkSceneSubsystem>() : nullptr;
}

bool UOrbitwerkSceneSubsystem::OpenScene(EOrbitwerkScene Scene)
{
	const FString MapPath = GetMapPathForScene(Scene);
	if (MapPath.IsEmpty())
	{
		UE_LOG(LogOrbitwerk, Warning, TEXT("OpenScene: unbekannte Szene."));
		return false;
	}

	if (!DoesSceneMapExist(Scene))
	{
		const FString Message = FString::Printf(
			TEXT("Map %s fehlt. Bitte Scripts/create_maps.py im Editor ausfuehren (siehe docs/T00-test.md)."), *MapPath);
		UE_LOG(LogOrbitwerk, Error, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Message);
		}
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogOrbitwerk, Error, TEXT("OpenScene: keine Welt vorhanden."));
		return false;
	}

	UE_LOG(LogOrbitwerk, Log, TEXT("Szenenwechsel -> %s"), *MapPath);
	UGameplayStatics::OpenLevel(World, FName(*MapPath));
	return true;
}

EOrbitwerkScene UOrbitwerkSceneSubsystem::GetCurrentScene() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		return EOrbitwerkScene::Unknown;
	}

	// true = PIE-Praefix ("UEDPIE_0_") entfernen.
	return GetSceneForMapName(UGameplayStatics::GetCurrentLevelName(World, true));
}

FString UOrbitwerkSceneSubsystem::GetMapPathForScene(EOrbitwerkScene Scene)
{
	switch (Scene)
	{
	case EOrbitwerkScene::MainMenu: return FString(OrbitwerkScenes::MapDirectory) + OrbitwerkScenes::MainMenuMap;
	case EOrbitwerkScene::Hangar:   return FString(OrbitwerkScenes::MapDirectory) + OrbitwerkScenes::HangarMap;
	case EOrbitwerkScene::Flight:   return FString(OrbitwerkScenes::MapDirectory) + OrbitwerkScenes::FlightMap;
	default:                        break;
	}
	return FString();
}

EOrbitwerkScene UOrbitwerkSceneSubsystem::GetSceneForMapName(const FString& MapName)
{
	// Akzeptiert "Hangar", "/Game/Maps/Hangar" und "/Game/Maps/Hangar.Hangar".
	FString ShortName = FPackageName::GetShortName(MapName);
	int32 DotIndex = INDEX_NONE;
	if (ShortName.FindChar(TEXT('.'), DotIndex))
	{
		ShortName.LeftInline(DotIndex);
	}

	if (ShortName.Equals(OrbitwerkScenes::MainMenuMap, ESearchCase::IgnoreCase))
	{
		return EOrbitwerkScene::MainMenu;
	}
	if (ShortName.Equals(OrbitwerkScenes::HangarMap, ESearchCase::IgnoreCase))
	{
		return EOrbitwerkScene::Hangar;
	}
	if (ShortName.Equals(OrbitwerkScenes::FlightMap, ESearchCase::IgnoreCase))
	{
		return EOrbitwerkScene::Flight;
	}
	return EOrbitwerkScene::Unknown;
}

bool UOrbitwerkSceneSubsystem::DoesSceneMapExist(EOrbitwerkScene Scene)
{
	const FString MapPath = GetMapPathForScene(Scene);
	return !MapPath.IsEmpty() && FPackageName::DoesPackageExist(MapPath);
}
