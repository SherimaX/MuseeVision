using UnrealBuildTool;

/// <summary>The Sphere (D-012): a stage above the Atrium. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeSphere : MuseeModuleRules
{
	public MuseeSphere(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
		// Its tests read the mechanisms schedule Motion owns, to check its bodies against it.
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeMotion" });
	}
}
