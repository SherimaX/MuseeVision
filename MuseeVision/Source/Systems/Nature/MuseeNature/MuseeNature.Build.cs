using UnrealBuildTool;

public class MuseeNature : MuseeModuleRules
{
	public MuseeNature(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter", "MuseeTime" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara" });
	}
}
