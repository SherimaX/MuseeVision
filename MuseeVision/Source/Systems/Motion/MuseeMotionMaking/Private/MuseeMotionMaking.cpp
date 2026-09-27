#include "FormPieceSource.h"
#include "Modules/ModuleManager.h"
#include "MotionPieces.h"

/** Motion's making twin: it joins Form's piece sources at startup, so every stage's machines get their moving parts. */
class FMuseeMotionMakingModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::FormMaking::RegisterPieceSource(System, Musee::MotionMaking::PieceSourceOrder, &Musee::MotionMaking::GatherParts);
	}

	virtual void ShutdownModule() override
	{
		Musee::FormMaking::UnregisterPieceSource(System);
	}

private:
	const FName System = TEXT("Motion");
};

IMPLEMENT_MODULE(FMuseeMotionMakingModule, MuseeMotionMaking);
