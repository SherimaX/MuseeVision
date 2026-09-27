using UnrealBuildTool;

public class MuseeStudio : MuseeModuleRules
{
	public MuseeStudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeArt", "MuseeLight", "MuseeSound", "MuseeMotion", "MuseeNature", "MuseeFormMaking", "MuseeNatureMaking", "MuseeForm", "MuseeMatterMaking", "MuseeMatter", "MuseeTime", "MuseeVisitor", "MuseeInstruments" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "MeshConversion", "AssetTools", "LevelInstanceEditor", "Localization", "PythonScriptPlugin" });
	}
}
