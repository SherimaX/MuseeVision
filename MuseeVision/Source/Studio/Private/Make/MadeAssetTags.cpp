#include "Make/MadeAssetTags.h"

#include "Engine/World.h"
#include "Make/InputHash.h"
#include "UObject/AssetRegistryTagsContext.h"

namespace Musee::Studio
{
	namespace
	{
		void AddInputHashTag(FAssetRegistryTagsContext Context)
		{
			const UObject* Object = Context.GetObject();
			if (!Object || !Object->IsAsset())
			{
				return;
			}
			if (const FString* Hash = FindInputHash(*Object))
			{
				Context.AddTag(UObject::FAssetRegistryTag(InputHashKey(), *Hash, UObject::FAssetRegistryTag::TT_Hidden));
			}
		}

		/** The name UWorld gives its modification date (World.cpp, UWorld::GetAssetRegistryTags). */
		const TCHAR* const DateModifiedTag = TEXT("DateModified");

		void StampLevelDate(const UWorld* World, FAssetRegistryTagsContext Context)
		{
			if (!World || !Context.IsSaving() || !FindInputHash(*World))
			{
				return;
			}
			Context.AddTag(UObject::FAssetRegistryTag(DateModifiedTag, FDateTime::MinValue().ToString(),
				UObject::FAssetRegistryTag::TT_Chronological, UObject::FAssetRegistryTag::TD_Date));
		}
	}

	void FMadeAssetTags::Register()
	{
		InputHashTag = UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.AddStatic(&AddInputHashTag);
		// Broadcast after UWorld has written its own tags, so this value replaces the clock's.
		LevelDateTag = FWorldDelegates::GetAssetTagsWithContext.AddStatic(&StampLevelDate);
	}

	void FMadeAssetTags::Unregister()
	{
		UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.Remove(InputHashTag);
		FWorldDelegates::GetAssetTagsWithContext.Remove(LevelDateTag);
	}
}
