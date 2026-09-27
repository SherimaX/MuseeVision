#include "Modules/ModuleManager.h"
#include "MotionDresser.h"
#include "MuseeStageDresser.h"

/** Motion's module: it joins the stage dressers at startup, so every stage Studio makes gets its machines. */
class FMuseeMotionModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::RegisterStageDresser(System, Musee::Motion::DresserOrder, &Musee::Motion::DressMechanisms);
	}

	virtual void ShutdownModule() override
	{
		Musee::UnregisterStageDresser(System);
	}

private:
	const FName System = TEXT("Motion");
};

IMPLEMENT_MODULE(FMuseeMotionModule, MuseeMotion);
