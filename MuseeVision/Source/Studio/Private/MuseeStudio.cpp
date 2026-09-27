#include "Make/MadeAssetTags.h"
#include "Make/MakeCommand.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "StudioLog.h"

DEFINE_LOG_CATEGORY(LogMuseeStudio);

namespace Musee::Studio
{
	/**
	 * The workshop beside the museum. While it is loaded, every asset Studio made shows the asset registry what it
	 * was made from, a made level saves without a clock in it, and musee.studio.make answers.
	 */
	class FStudioModule final : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			Tags.Register();
			RegisterMakeCommand();
		}

		virtual void ShutdownModule() override
		{
			UnregisterMakeCommand();
			Tags.Unregister();
		}

	private:
		FMadeAssetTags Tags;
	};
}

IMPLEMENT_MODULE(Musee::Studio::FStudioModule, MuseeStudio);
