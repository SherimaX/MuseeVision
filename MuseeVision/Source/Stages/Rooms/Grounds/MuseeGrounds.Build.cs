using UnrealBuildTool;

/// <summary>The Grounds: the one Ground stage, the museum's site under its own sky. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeGrounds : MuseeModuleRules
{
	public MuseeGrounds(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
	}
}
