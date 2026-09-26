using UnrealBuildTool;

public class MuseeForm : MuseeModuleRules
{
	public MuseeForm(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeMatter" });
	}
}
