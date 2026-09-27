using UnrealBuildTool;

public class MuseeStudio : MuseeModuleRules
{
	public MuseeStudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeArt", "MuseeArtMaking", "MuseeLight", "MuseeSound", "MuseeMotion", "MuseeNature", "MuseeFormMaking", "MuseeNatureMaking", "MuseeForm", "MuseeMatterMaking", "MuseeMatter", "MuseeTime", "MuseeVisitor", "MuseeInstruments" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "MeshConversion", "GeometryCore", "AssetTools", "LevelInstanceEditor", "Localization", "PythonScriptPlugin" });
	}
}
