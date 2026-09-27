using UnrealBuildTool;

public class MuseeStudio : MuseeModuleRules
{
	public MuseeStudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeSite", "MuseeArt", "MuseeArtMaking", "MuseeLight", "MuseeSound", "MuseeMotion", "MuseeNature", "MuseeFormMaking", "MuseeNatureMaking", "MuseeForm", "MuseeMatterMaking", "MuseeMatter", "MuseeTime", "MuseeVisitor", "MuseeInstruments" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "MeshConversion", "GeometryCore", "AssetTools", "LevelInstanceEditor", "Localization", "PythonScriptPlugin" });
		// The makers' photometric files come in through the engine's own IES import (its IES converter).
		PrivateDependencyModuleNames.Add("IESFile");
		// The collection's glTF scans come in through the engine's own glTF import (the Interchange plugin).
		PrivateDependencyModuleNames.AddRange(new string[] { "InterchangeCore", "InterchangeEngine", "InterchangePipelines" });
	}
}
