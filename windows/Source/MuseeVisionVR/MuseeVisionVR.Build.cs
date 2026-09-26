using UnrealBuildTool;

// The visitor in a headset. Built only into the VR target (MuseeVisionVR.Target.cs).
public class MuseeVisionVR : ModuleRules
{
	public MuseeVisionVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// The VR build is DebugGame (MuseeVisionVR.Target.cs) but runs at full speed.
		OptimizeCode = CodeOptimization.Always;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"HeadMountedDisplay",
			"UMG",
			"Slate",
			"SlateCore",
			"MuseeVision"
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
