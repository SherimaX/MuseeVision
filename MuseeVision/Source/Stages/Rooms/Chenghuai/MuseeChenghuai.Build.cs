using UnrealBuildTool;

/// <summary>澄懷 Chenghuai, the house and garden on the Rotunda's north door: a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeChenghuai : MuseeModuleRules
{
	public MuseeChenghuai(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
	}
}
