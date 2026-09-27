using UnrealBuildTool;

public class MuseeFormMaking : MuseeModuleRules
{
	public MuseeFormMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "GeometryCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "GeometryAlgorithms", "DynamicMesh" });
	}
}
