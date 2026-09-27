using UnrealBuildTool;

/// <summary>The Nymphéas oval (D-019): a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeNympheas : MuseeModuleRules
{
	public MuseeNympheas(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the schedules the systems own (the hanging, planting and mechanisms) to check its anchors.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeMotion", "MuseeNature", "MuseeArt" });
	}
}
