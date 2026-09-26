#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

/** Maps Matter's shading library at /Project/Matter, so every generated material includes the one surface law. */
class FMuseeMatterShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const FString Dir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/Systems/Matter/MuseeMatterShaders/Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Project/Matter"), Dir);
	}
};

IMPLEMENT_MODULE(FMuseeMatterShadersModule, MuseeMatterShaders);
