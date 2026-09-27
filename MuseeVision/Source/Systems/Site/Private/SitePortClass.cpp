#include "SitePortClass.h"

#include "MuseeRecords.h"

TValueOrError<FSitePortClass, FString> Musee::Site::FindPortClass(const FMuseeKey& Key, const FString& Path)
{
	TValueOrError<TArray<FSitePortClass>, FString> Read = ReadRecords<FSitePortClass>(Path);
	if (Read.HasError())
	{
		return MakeError(Read.StealError());
	}
	for (FSitePortClass& Class : Read.GetValue())
	{
		if (Class.Key == Key)
		{
			return MakeValue(MoveTemp(Class));
		}
	}
	return MakeError(FString::Printf(TEXT("%s holds no port class %s"), *Path, *Key.ToString()));
}
