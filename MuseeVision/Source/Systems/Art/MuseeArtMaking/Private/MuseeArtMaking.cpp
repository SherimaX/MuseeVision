#include "ArtPieces.h"
#include "FormPieceSource.h"
#include "Modules/ModuleManager.h"
#include "MuseeStageDresser.h"

/** Art's making twin: it joins Form's piece sources and the stage dressers at startup, so every stage gets its works. */
class FMuseeArtMakingModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::FormMaking::RegisterPieceSource(System, Musee::ArtMaking::PieceSourceOrder, &Musee::ArtMaking::GatherArt);
		Musee::RegisterStageDresser(System, Musee::ArtMaking::DresserOrder, &Musee::ArtMaking::DressWorks);
	}

	virtual void ShutdownModule() override
	{
		Musee::FormMaking::UnregisterPieceSource(System);
		Musee::UnregisterStageDresser(System);
	}

private:
	const FName System = TEXT("Art");
};

IMPLEMENT_MODULE(FMuseeArtMakingModule, MuseeArtMaking);
