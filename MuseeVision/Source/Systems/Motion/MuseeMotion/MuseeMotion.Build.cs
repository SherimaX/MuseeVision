using UnrealBuildTool;

public class MuseeMotion : MuseeModuleRules
{
	public MuseeMotion(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter" });
	}
}
