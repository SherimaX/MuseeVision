using UnrealBuildTool;

/// <summary>The Cube (Élan, level −1): a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeCube : MuseeModuleRules
{
	public MuseeCube(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the mechanisms schedule Motion owns to check the bodies and the control it names.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeMotion" });
	}
}
