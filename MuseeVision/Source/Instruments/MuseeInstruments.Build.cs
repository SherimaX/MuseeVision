using UnrealBuildTool;

public class MuseeInstruments : MuseeModuleRules
{
	public MuseeInstruments(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeVisitor" });
	}
}
