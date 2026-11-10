// Orbitwerk - Haupt-Spielmodul (GameMode, Szenen, Input)

using UnrealBuildTool;

public class Orbitwerk : ModuleRules
{
	public Orbitwerk(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"OrbitwerkUI"      // OrbitwerkMenuTypes.h wird im PlayerController-Header benutzt
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UMG",
			"OrbitwerkCore",
			"OrbitwerkRender"
		});
	}
}
