using UnrealBuildTool;

public class MuseeTime : MuseeModuleRules
{
	public MuseeTime(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP" });
	}
}
