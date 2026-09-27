using UnrealBuildTool;

/// <summary>The Nymphéas oval (D-019): a stage. It describes itself through the kit and never sees another stage.</summary>
public class MuseeNympheas : MuseeModuleRules
{
	public MuseeNympheas(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite" });
		PrivateDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter", "MuseeTime", "MuseeLight", "MuseeSound", "MuseeMotion", "MuseeNature", "MuseeArt" });
	}
}
