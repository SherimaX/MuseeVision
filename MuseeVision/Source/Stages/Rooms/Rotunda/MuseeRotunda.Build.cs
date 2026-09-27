using UnrealBuildTool;

/// <summary>The Rotunda (D-012): the anchor stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeRotunda : MuseeModuleRules
{
	public MuseeRotunda(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the mechanisms schedule Motion owns, to check its anchors.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeMotion" });
		// Its dial is laid from Time's: where the museum's sun casts the node's shadow.
		PrivateDependencyModuleNames.Add("MuseeTime");
	}
}
