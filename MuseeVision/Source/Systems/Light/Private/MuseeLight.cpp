#include "LightLog.h"
#include "Modules/ModuleManager.h"
#include "Schedule/LightDresser.h"

MUSEE_DEFINE_LOG_CATEGORY(Light);

/** Light's module: it registers Light's stage dresser, which places a stage's luminaires as Studio makes it. */
class FMuseeLightModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::RegisterStageDresser(TEXT("Light"), Musee::Light::DresserOrder, &Musee::Light::DressStage);
	}

	virtual void ShutdownModule() override
	{
		Musee::UnregisterStageDresser(TEXT("Light"));
	}
};

IMPLEMENT_MODULE(FMuseeLightModule, MuseeLight);
