// Orbitwerk - OrbitwerkRender
// UE-Modul fuer Planeten-LOD, Atmosphaere, Effekte und das Grafik-Settings-System.

using UnrealBuildTool;

public class OrbitwerkRender : ModuleRules
{
	public OrbitwerkRender(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OrbitwerkCore"
		});
	}
}
