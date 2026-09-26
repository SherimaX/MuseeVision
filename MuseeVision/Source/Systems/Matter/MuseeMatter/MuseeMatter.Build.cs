using UnrealBuildTool;

public class MuseeMatter : MuseeModuleRules
{
	public MuseeMatter(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeTime" });
	}
}
