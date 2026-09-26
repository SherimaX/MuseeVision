using UnrealBuildTool;

public class MuseeVisionTarget : TargetRules
{
	public MuseeVisionTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MuseeVision");
	}
}
