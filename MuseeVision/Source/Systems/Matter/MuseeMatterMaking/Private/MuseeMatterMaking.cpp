#include "MatterMasterAsset.h"
#include "MatterNames.h"
#include "Modules/ModuleManager.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

namespace Musee::Matter
{
namespace
{
	/** Shows what each of Matter's made assets was made from (its package metadata) as asset registry tags. */
	void AddMadeTags(FAssetRegistryTagsContext Context)
	{
		const UObject* Object = Context.GetObject();
		if (!Object || !Object->IsAsset())
		{
			return;
		}
		UPackage* Package = Object->GetPackage();
		const FString PackageName = Package->GetName();
		if (!PackageName.StartsWith(GeneratedFolder) && PackageName != MomentCollectionPackage)
		{
			return;
		}
		FMetaData& MetaData = Package->GetMetaData();
		for (const TCHAR* Tag : { MadeTag::Made, MadeTag::Inputs, MadeTag::Records, MadeTag::Generator })
		{
			if (const FString* Value = MetaData.FindValue(Object, Tag))
			{
				Context.AddTag(UObject::FAssetRegistryTag(FName(Tag), *Value, UObject::FAssetRegistryTag::TT_Hidden));
			}
		}
	}
}
}

/** Matter's making twin: its masters, its instance filler and its make step. */
class FMuseeMatterMakingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		MadeTags = UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.AddStatic(&Musee::Matter::AddMadeTags);
	}

	virtual void ShutdownModule() override
	{
		UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.Remove(MadeTags);
	}

private:
	FDelegateHandle MadeTags;
};

IMPLEMENT_MODULE(FMuseeMatterMakingModule, MuseeMatterMaking);
