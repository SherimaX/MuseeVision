using UnrealBuildTool;

// Editor-only tools (never in a game build): the bake of the native procedural architecture into
// Nanite static meshes (MuseeBakeLibrary; Scripts/bake.py).
public class MuseeVisionEditor : ModuleRules
{
	public MuseeVisionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetRegistry",
			"MeshDescription",
			"StaticMeshDescription",
			"PhysicsCore",
			"ProceduralMeshComponent",
			"Json",   // Élan Cube: the journeys' terrain (MuseeJourneyLibrary)
			"MuseeVision"
		});
	}
}
