#include "MatterResolve.h"

#include "MatterLog.h"
#include "MatterNames.h"
#include "MatterRecords.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

namespace Musee::Matter
{
UMaterialInterface* Resolve(FName PaletteKey, FName Working, FName History)
{
	if (PaletteKey.IsNone() || Working.IsNone() || History.IsNone())
	{
		UE_LOG(LogMuseeMatter, Warning, TEXT("A material is named by a palette key, a working and a history; this one was named '%s', '%s' and '%s'."),
			*PaletteKey.ToString(), *Working.ToString(), *History.ToString());
		return nullptr;
	}
	const FString Package = InstancePackage(PaletteKey, Working, History);
	const FString ObjectPath = ObjectPathOf(Package);
	UMaterialInterface* Material = FindObject<UMaterialInterface>(nullptr, *ObjectPath);
	if (!Material && FPackageName::DoesPackageExist(Package))
	{
		Material = LoadObject<UMaterialInterface>(nullptr, *ObjectPath);
	}
	if (!Material)
	{
		UE_LOG(LogMuseeMatter, Warning, TEXT("No material was made for %s, %s, %s (%s): the palette offers no such finish, or MuseeMatterMake has not run."),
			*PaletteKey.ToString(), *Working.ToString(), *History.ToString(), *Package);
	}
	return Material;
}

UMaterialInterface* ResolveFinish(const FString& ScheduleFolder, FName Surface)
{
	const TValueOrError<FMatterFinishSchedule, FString> Schedule = ReadFinishSchedule(ScheduleFolder);
	if (Schedule.HasError())
	{
		UE_LOG(LogMuseeMatter, Warning, TEXT("The finish schedule is unreadable: %s"), *Schedule.GetError());
		return nullptr;
	}
	const FMatterFinish* Finish = Schedule.GetValue().Find(Surface);
	if (!Finish)
	{
		UE_LOG(LogMuseeMatter, Warning, TEXT("The finish schedule of %s names no surface '%s'."),
			*Schedule.GetValue().Stage.ToString(), *Surface.ToString());
		return nullptr;
	}
	return Resolve(Finish->Material.ToName(), Finish->Working.ToName(), Finish->History.ToName());
}
}
