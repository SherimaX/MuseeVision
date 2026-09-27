#include "FormPieceSource.h"
#include "Modules/ModuleManager.h"
#include "NaturePieces.h"

/** Nature's making twin: it joins Form's piece sources at startup, so every stage Studio makes gets its water and plants. */
class FMuseeNatureMakingModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::FormMaking::RegisterPieceSource(System, Musee::NatureMaking::PieceSourceOrder, &Musee::NatureMaking::GatherNature);
	}

	virtual void ShutdownModule() override
	{
		Musee::FormMaking::UnregisterPieceSource(System);
	}

private:
	const FName System = TEXT("Nature");
};

IMPLEMENT_MODULE(FMuseeNatureMakingModule, MuseeNatureMaking);
