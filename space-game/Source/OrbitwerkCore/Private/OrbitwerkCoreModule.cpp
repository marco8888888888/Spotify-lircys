// Orbitwerk - OrbitwerkCore

#include "OrbitwerkCore.h"

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogOrbitwerkCore);

bool OrbitwerkCore::IsOrbitCoreLinked()
{
	return WITH_ORBITCORE != 0;
}

class FOrbitwerkCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogOrbitwerkCore, Log, TEXT("OrbitwerkCore gestartet. OrbitCore-Bibliothek: %s"),
			OrbitwerkCore::IsOrbitCoreLinked() ? TEXT("eingebunden") : TEXT("nicht gefunden (ThirdParty/OrbitCore fehlt)"));
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FOrbitwerkCoreModule, OrbitwerkCore)
