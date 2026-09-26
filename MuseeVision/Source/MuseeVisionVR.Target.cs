using UnrealBuildTool;

/// <summary>
/// The headset museum (DebugGame, OpenXR through SteamVR). An installed engine can't enable plugins per target, so the
/// .uproject enables OpenXR for DebugGame game builds only and the headset adapter for this target only; Musée code is
/// optimised in DebugGame by MuseeModuleRules.
/// </summary>
public class MuseeVisionVRTarget : TargetRules
{
	public MuseeVisionVRTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "MuseeSite", "MuseeVisitorHeadset" });
	}
}
