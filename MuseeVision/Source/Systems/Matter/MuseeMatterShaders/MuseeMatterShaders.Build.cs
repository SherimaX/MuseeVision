using System.IO;
using UnrealBuildTool;

/// <summary>Maps Matter's shading library (Shaders/*.ush) at /Project/Matter before any material compiles.</summary>
public class MuseeMatterShaders : ModuleRules
{
	public MuseeMatterShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		if (Target.Configuration == UnrealTargetConfiguration.DebugGame) { OptimizeCode = CodeOptimization.Always; }
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "RenderCore" });
	}
}
