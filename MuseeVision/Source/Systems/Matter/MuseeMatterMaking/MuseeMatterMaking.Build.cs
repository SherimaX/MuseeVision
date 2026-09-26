using UnrealBuildTool;

public class MuseeMatterMaking : MuseeModuleRules
{
	public MuseeMatterMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeMatter" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialEditor", "AssetTools" });
	}
}
