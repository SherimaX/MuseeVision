using UnrealBuildTool;

/// <summary>Nature's making twin (editor only): grows plants and shapes water and ground as Form drawings at making time.</summary>
public class MuseeNatureMaking : MuseeModuleRules
{
	public MuseeNatureMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeNature", "MuseeFormMaking", "MuseeForm", "MuseeMatter" });
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "DynamicMesh" });
	}
}
