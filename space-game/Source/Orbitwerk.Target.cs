// Orbitwerk - Game-Target (Standalone / Packaging)

using UnrealBuildTool;
using System.Collections.Generic;

public class OrbitwerkTarget : TargetRules
{
	public OrbitwerkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[]
		{
			"Orbitwerk",
			"OrbitwerkCore",
			"OrbitwerkRender",
			"OrbitwerkUI"
		});
	}
}
