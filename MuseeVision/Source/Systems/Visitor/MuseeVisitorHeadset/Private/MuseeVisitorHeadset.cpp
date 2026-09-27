#include "Engine/Engine.h"
#include "IHeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "Modules/ModuleManager.h"
#include "VisitorAdapter.h"
#include "VisitorBody.h"
#include "VisitorHeadset.h"

namespace Musee::Visitor::Headset
{
	/** The headset adapter: active while an OpenXR session tracks a connected headset. */
	class FHeadsetAdapter final : public IVisitorAdapter
	{
	public:
		virtual FName Name() const override { return TEXT("headset"); }

		virtual bool IsActive() const override
		{
			IXRTrackingSystem* Tracking = GEngine ? GEngine->XRSystem.Get() : nullptr;
			return Tracking && Tracking->GetHMDDevice() && Tracking->GetHMDDevice()->IsHMDConnected() && Tracking->IsHeadTrackingAllowed();
		}

		virtual void Adopt(AVisitorBody& Body) override
		{
			UVisitorHeadset* Headset = NewObject<UVisitorHeadset>(&Body, TEXT("Headset"));
			Headset->RegisterComponent();
		}
	};
}

/** The headset adapter's module, built only into the VR target (Decision D-010). */
class FMuseeVisitorHeadsetModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Musee::Visitor::RegisterAdapter(MakeShared<Musee::Visitor::Headset::FHeadsetAdapter>());
	}

	virtual void ShutdownModule() override
	{
		Musee::Visitor::UnregisterAdapter(TEXT("headset"));
	}
};

IMPLEMENT_MODULE(FMuseeVisitorHeadsetModule, MuseeVisitorHeadset);
