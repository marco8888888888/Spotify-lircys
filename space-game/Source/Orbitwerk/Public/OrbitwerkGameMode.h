// Orbitwerk - Haupt-Spielmodul
//
// Globaler GameMode fuer alle Szenen (DefaultEngine.ini: GlobalDefaultGameMode).
// Die szenenspezifische Logik (UI, Input-Modus) steckt im PlayerController.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OrbitwerkGameMode.generated.h"

UCLASS()
class ORBITWERK_API AOrbitwerkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOrbitwerkGameMode();
};
