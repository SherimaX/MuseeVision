#include "SiteRoom.h"

#include "UObject/UObjectHash.h"

TArray<const USiteRoom*> USiteRoom::AllRooms()
{
	TArray<UClass*> Classes;
	GetDerivedClasses(StaticClass(), Classes, true);
	TArray<const USiteRoom*> Rooms;
	for (const UClass* Class : Classes)
	{
		const bool bStale = Class->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated);
		if (!Class->HasAnyClassFlags(CLASS_Abstract) && !bStale)
		{
			Rooms.Add(Class->GetDefaultObject<USiteRoom>());
		}
	}
	Rooms.Sort([](const USiteRoom& A, const USiteRoom& B) { return A.StageKey().ToString() < B.StageKey().ToString(); });
	return Rooms;
}

const USiteRoom* USiteRoom::Find(const FMuseeKey& Stage)
{
	for (const USiteRoom* Room : AllRooms())
	{
		if (Room->StageKey() == Stage)
		{
			return Room;
		}
	}
	return nullptr;
}
