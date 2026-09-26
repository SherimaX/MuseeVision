using UnrealBuildTool;

// The VR build (MuseeVisionVR.exe): the same museum with OpenXR, for a headset through SteamVR (the
// Vision Pro through ALVR). Build it as DebugGame (Scripts/package_vr.ps1): the .uproject enables
// OpenXR only for DebugGame game builds and the MuseeVisionVR module only for this target, so the
// desktop game (Development, Shipping) and the editor are exactly as before. An installed engine
// can't enable plugins per target, hence the configuration; the game code is optimised all the same
// (the Build.cs files).
public class MuseeVisionVRTarget : TargetRules
{
	public MuseeVisionVRTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "MuseeVision", "MuseeVisionVR" });
	}
}
