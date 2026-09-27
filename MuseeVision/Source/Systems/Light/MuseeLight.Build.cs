using UnrealBuildTool;

public class MuseeLight : MuseeModuleRules
{
	public MuseeLight(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter", "MuseeTime" });
	}
}
