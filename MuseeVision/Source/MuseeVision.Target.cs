using UnrealBuildTool;

/// <summary>The desktop museum (Development, Shipping). Instruments are left out of Shipping by the .uproject.</summary>
public class MuseeVisionTarget : TargetRules
{
	public MuseeVisionTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "MuseeSite" });
	}
}
