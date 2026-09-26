using UnrealBuildTool;

/// <summary>
/// The one base every Musée module's rules derive from: the shared build settings, written once.
/// DebugGame is the headset build (MuseeVisionVR.Target.cs), so Musée code is optimised there as in Development.
/// Every module except Foundation itself depends on Foundation, the standards everything is built to.
/// </summary>
public abstract class MuseeModuleRules : ModuleRules
{
	protected MuseeModuleRules(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		if (Target.Configuration == UnrealTargetConfiguration.DebugGame) { OptimizeCode = CodeOptimization.Always; }
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		if (GetType().Name != "MuseeFoundation") { PublicDependencyModuleNames.Add("MuseeFoundation"); }
	}
}

public class MuseeFoundation : MuseeModuleRules
{
	public MuseeFoundation(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
	}
}
