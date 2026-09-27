#include "Make/MadeAssetTags.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "StudioLog.h"

DEFINE_LOG_CATEGORY(LogMuseeStudio);

namespace Musee::Studio
{
	/**
	 * The workshop beside the museum. While it is loaded, every asset Studio made shows the asset registry what it
	 * was made from, and a made level saves without a clock in it.
	 */
	class FStudioModule final : public IModuleInterface
	{
	public:
		virtual void StartupModule() override { Tags.Register(); }
		virtual void ShutdownModule() override { Tags.Unregister(); }

	private:
		FMadeAssetTags Tags;
	};
}

IMPLEMENT_MODULE(Musee::Studio::FStudioModule, MuseeStudio);
