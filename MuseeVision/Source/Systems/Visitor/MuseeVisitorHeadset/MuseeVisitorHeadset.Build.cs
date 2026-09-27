using UnrealBuildTool;

public class MuseeVisitorHeadset : MuseeModuleRules
{
	public MuseeVisitorHeadset(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeVisitor" });
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "EnhancedInput", "HeadMountedDisplay", "UMG", "Slate", "SlateCore" });
	}
}
