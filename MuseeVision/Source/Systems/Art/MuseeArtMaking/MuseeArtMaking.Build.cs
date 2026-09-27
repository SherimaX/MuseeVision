using UnrealBuildTool;

/// <summary>Art's making twin (editor only): builds each work as a real object (a stretched canvas, a panel, a cast from its scan) from the hanging plan at making time.</summary>
public class MuseeArtMaking : MuseeModuleRules
{
	public MuseeArtMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeArt", "MuseeFormMaking", "MuseeForm", "MuseeMatter" });
		// FFormMadePiece holds its mesh as GeometryCore's FDynamicMesh3, which each module that makes pieces links. A cast
		// reads the scan Studio imported from the static mesh's description.
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "MeshDescription", "StaticMeshDescription" });
	}
}
