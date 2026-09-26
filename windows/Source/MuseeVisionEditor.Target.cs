using UnrealBuildTool;

public class MuseeVisionEditorTarget : TargetRules
{
	public MuseeVisionEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MuseeVision");
	}
}
