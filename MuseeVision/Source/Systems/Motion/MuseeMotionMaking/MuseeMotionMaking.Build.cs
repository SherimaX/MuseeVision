using UnrealBuildTool;

/// <summary>Motion's making twin (editor only): draws the parts of its machines whose length changes (telescopes, columns of turns) as Form drawings at making time.</summary>
public class MuseeMotionMaking : MuseeModuleRules
{
	public MuseeMotionMaking(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "MuseeMotion", "MuseeFormMaking", "MuseeForm" });
		// FFormMadePiece holds its mesh as GeometryCore's FDynamicMesh3, which each module that makes pieces links.
		PrivateDependencyModuleNames.Add("GeometryCore");
	}
}
