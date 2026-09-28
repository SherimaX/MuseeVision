using UnrealBuildTool;

public class MuseeLight : MuseeModuleRules
{
	public MuseeLight(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeForm", "MuseeMatter", "MuseeTime" });
		// The evidence test reads the catalogue's records as JSON, to check what each credits to its photometric file.
		PrivateDependencyModuleNames.Add("Json");
		// An image that encloses its luminaire is shown on a surface Light makes at run time (the Sphere's image).
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "GeometryFramework" });
	}
}
