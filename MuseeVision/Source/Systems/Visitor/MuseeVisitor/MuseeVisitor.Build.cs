using UnrealBuildTool;

public class MuseeVisitor : MuseeModuleRules
{
	public MuseeVisitor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });
		// DLSS Frame Generation's frame count for the settings card: Streamline exists on Windows only.
		bool bStreamline = Target.Platform == UnrealTargetPlatform.Win64;
		if (bStreamline) { PrivateDependencyModuleNames.Add("StreamlineCore"); }
		PrivateDefinitions.Add("MUSEE_WITH_STREAMLINE=" + (bStreamline ? "1" : "0"));
	}
}
