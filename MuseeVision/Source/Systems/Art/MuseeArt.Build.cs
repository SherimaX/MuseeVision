using UnrealBuildTool;

public class MuseeArt : MuseeModuleRules
{
	public MuseeArt(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeLight", "MuseeForm", "MuseeMatter", "MuseeTime" });
	}
}
