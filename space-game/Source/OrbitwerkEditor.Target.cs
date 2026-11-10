// Orbitwerk - Editor-Target

using UnrealBuildTool;
using System.Collections.Generic;

public class OrbitwerkEditorTarget : TargetRules
{
	public OrbitwerkEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
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
