#include "Modules/ModuleManager.h"
#include "Settings/TimeSettings.h"
#include "TimeLog.h"

MUSEE_DEFINE_LOG_CATEGORY(Time);

/** Time's module: it registers Time's settings and commands, which its world subsystem follows. */
class FMuseeTimeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::Time::Settings::Register();
	}

	virtual void ShutdownModule() override
	{
		Musee::Time::Settings::Unregister();
	}
};

IMPLEMENT_MODULE(FMuseeTimeModule, MuseeTime);
