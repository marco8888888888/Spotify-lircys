// Orbitwerk - OrbitwerkCore
// Hinweis: Dieses Modul kompiliert ohne PCH (siehe OrbitwerkCore.Build.cs).
// Jede Datei muss daher alles, was sie braucht, selbst inkludieren.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

#ifndef WITH_ORBITCORE
#define WITH_ORBITCORE 0
#endif

ORBITWERKCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogOrbitwerkCore, Log, All);

namespace OrbitwerkCore
{
	/** true, wenn ThirdParty/OrbitCore beim Build gefunden und mitkompiliert wurde. */
	ORBITWERKCORE_API bool IsOrbitCoreLinked();
}
