using UnrealBuildTool;

/// <summary>The Reserve (D-020): a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeReserve : MuseeModuleRules
{
	public MuseeReserve(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the schedules the systems own (the hanging and the mechanisms) to check its anchors.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeMotion", "MuseeArt" });
	}
}
