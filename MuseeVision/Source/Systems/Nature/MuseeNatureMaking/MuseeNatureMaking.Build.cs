using UnrealBuildTool;

/// <summary>Nature's making twin (editor only): grows plants and shapes water and ground as Form drawings at making time.</summary>
public class MuseeNatureMaking : MuseeModuleRules
{
	public MuseeNatureMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeNature", "MuseeFormMaking", "MuseeForm", "MuseeMatter" });
		// FFormMadePiece holds its mesh as GeometryCore's FDynamicMesh3, which each module that makes pieces links.
		PrivateDependencyModuleNames.Add("GeometryCore");
	}
}
