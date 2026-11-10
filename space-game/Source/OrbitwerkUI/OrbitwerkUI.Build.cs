// Orbitwerk - OrbitwerkUI
// UE-Modul fuer HUD, Navball, Editor-UI und Settings-Menue (UMG, Logik in C++).

using UnrealBuildTool;

public class OrbitwerkUI : ModuleRules
{
	public OrbitwerkUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"OrbitwerkCore",
			"OrbitwerkRender"
		});
	}
}
