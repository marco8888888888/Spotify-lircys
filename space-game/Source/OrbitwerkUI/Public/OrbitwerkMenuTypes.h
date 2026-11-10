// Orbitwerk - OrbitwerkUI

#pragma once

#include "CoreMinimal.h"
#include "OrbitwerkMenuTypes.generated.h"

/** Aktionen, die das Hauptmenue an das Spielmodul meldet (das UI-Modul kennt keine Szenen-Logik). */
UENUM(BlueprintType)
enum class EOrbitwerkMenuAction : uint8
{
	OpenHangar   UMETA(DisplayName = "Hangar"),
	StartFlight  UMETA(DisplayName = "Flug starten"),
	Quit         UMETA(DisplayName = "Beenden")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrbitwerkMenuActionSignature, EOrbitwerkMenuAction, Action);
