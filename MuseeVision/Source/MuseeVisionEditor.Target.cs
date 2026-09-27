using UnrealBuildTool;

/// <summary>The editor: every module, including the making modules (MatterMaking, FormMaking, Studio) that no game build links.</summary>
public class MuseeVisionEditorTarget : TargetRules
{
	public MuseeVisionEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "MuseeSite", "MuseeStudio", "MuseeNympheas", "MuseeRotunda" });
	}
}
