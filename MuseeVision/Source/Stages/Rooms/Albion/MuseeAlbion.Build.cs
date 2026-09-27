using UnrealBuildTool;

/// <summary>Albion, the British court on the Rotunda's south door: a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeAlbion : MuseeModuleRules
{
	public MuseeAlbion(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the hanging schedule Art owns, to check its anchors.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeArt" });
	}
}
