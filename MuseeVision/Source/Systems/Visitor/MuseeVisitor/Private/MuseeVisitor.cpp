#include "Modules/ModuleManager.h"
#include "VisitorPace.h"

/** Visitor's module: it registers the visitor's settings, which the body follows. */
class FMuseeVisitorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FVisitorPace::Register();
	}

	virtual void ShutdownModule() override
	{
		FVisitorPace::Unregister();
	}
};

IMPLEMENT_MODULE(FMuseeVisitorModule, MuseeVisitor);
