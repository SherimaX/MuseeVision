using UnrealBuildTool;

public class MuseeVisitor : MuseeModuleRules
{
	public MuseeVisitor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });
		// DLSS Frame Generation's frame count for the settings card: Streamline exists on Windows, and only in game builds
		// (the NVIDIA plugins are enabled for game targets only, so the editor never loads a second copy of them).
		bool bStreamline = Target.Platform == UnrealTargetPlatform.Win64 && Target.Type == TargetType.Game;
		if (bStreamline) { PrivateDependencyModuleNames.Add("StreamlineCore"); }
		PrivateDefinitions.Add("MUSEE_WITH_STREAMLINE=" + (bStreamline ? "1" : "0"));
	}
}
