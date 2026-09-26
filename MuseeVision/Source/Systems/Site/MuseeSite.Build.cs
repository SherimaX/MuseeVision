using UnrealBuildTool;

public class MuseeSite : MuseeModuleRules
{
	public MuseeSite(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeArt", "MuseeLight", "MuseeSound", "MuseeMotion", "MuseeNature", "MuseeForm", "MuseeMatter", "MuseeTime", "MuseeVisitor" });
		PrivateDependencyModuleNames.AddRange(new string[] { "NavigationSystem" });
	}
}
