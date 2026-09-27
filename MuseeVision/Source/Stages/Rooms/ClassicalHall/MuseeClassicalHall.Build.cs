using UnrealBuildTool;

/// <summary>The Classical Hall (level −1 under the Rotunda): a stage. It describes itself through Site and the kit and never sees another stage.</summary>
public class MuseeClassicalHall : MuseeModuleRules
{
	public MuseeClassicalHall(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeForm" });
	}
}
