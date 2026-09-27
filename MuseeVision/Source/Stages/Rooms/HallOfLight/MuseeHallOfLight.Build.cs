using UnrealBuildTool;

/// <summary>The Hall of Light: a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeHallOfLight : MuseeModuleRules
{
	public MuseeHallOfLight(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
	}
}
