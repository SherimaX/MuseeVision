using UnrealBuildTool;

public class MuseeSound : MuseeModuleRules
{
	public MuseeSound(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter", "MuseeTime" });
		PrivateDependencyModuleNames.AddRange(new string[] { "AudioMixer", "MetasoundEngine" });
	}
}
