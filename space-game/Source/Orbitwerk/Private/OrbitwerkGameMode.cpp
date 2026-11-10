// Orbitwerk - Haupt-Spielmodul

#include "OrbitwerkGameMode.h"

#include "OrbitwerkPlayerController.h"

#include "GameFramework/DefaultPawn.h"

AOrbitwerkGameMode::AOrbitwerkGameMode()
{
	PlayerControllerClass = AOrbitwerkPlayerController::StaticClass();

	// Platzhalter-Pawn (frei fliegende Debug-Kamera), bis T20/T21 eigene Pawns liefern.
	DefaultPawnClass = ADefaultPawn::StaticClass();
}
