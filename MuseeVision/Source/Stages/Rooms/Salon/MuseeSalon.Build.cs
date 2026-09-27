using UnrealBuildTool;

/// <summary>The Salon Impression: a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeSalon : MuseeModuleRules
{
	public MuseeSalon(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the schedules the systems own (the hanging and the planting) to check its anchors.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeNature", "MuseeArt" });
	}
}
