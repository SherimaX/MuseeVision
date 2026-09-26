using UnrealBuildTool;

public class MuseeVision : ModuleRules
{
	public MuseeVision(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// DebugGame is the VR build (MuseeVisionVR.Target.cs): optimised like Development.
		if (Target.Configuration == UnrealTargetConfiguration.DebugGame) { OptimizeCode = CodeOptimization.Always; }

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ProceduralMeshComponent",
			"GeometryCore",
			"GeometryAlgorithms",
			"Json",
			"Slate",
			"SlateCore",
			"RHI",
			"AssetRegistry"   // Élan Cube: the journeys find their land's tiles
		});

		// DLSS Frame Generation's frame count, for the graphics settings' frame rate (Settings/MuseeGraphics).
		bool bStreamline = Target.Platform == UnrealTargetPlatform.Win64;
		if (bStreamline) { PrivateDependencyModuleNames.Add("StreamlineCore"); }
		PrivateDefinitions.Add("MUSEE_WITH_STREAMLINE=" + (bStreamline ? "1" : "0"));

		// Salon interior: the lanterns' live weather from Giverny (Salon/SalonSky, Open-Meteo over HTTPS; no key).
		if (!PrivateDependencyModuleNames.Contains("HTTP")) { PrivateDependencyModuleNames.Add("HTTP"); }
		// Salon interior: the rain on the lanterns' glass, made as it plays (Salon/SalonRain: a procedural sound wave).
		if (!PrivateDependencyModuleNames.Contains("AudioExtensions")) { PrivateDependencyModuleNames.Add("AudioExtensions"); }

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
