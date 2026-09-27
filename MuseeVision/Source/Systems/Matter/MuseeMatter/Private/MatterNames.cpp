#include "MatterNames.h"

#include "Misc/Char.h"

namespace Musee::Matter
{
namespace
{
	/** The prefixes Unreal's naming convention gives materials and material instances. */
	constexpr const TCHAR* MasterPrefix = TEXT("M_Matter_");
	constexpr const TCHAR* InstancePrefix = TEXT("MI_");
	constexpr const TCHAR* PartSeparator = TEXT("_");
}

FString PascalCase(FStringView Key)
{
	FString Name;
	Name.Reserve(Key.Len());
	bool bWordStart = true;
	for (const TCHAR Char : Key)
	{
		if (Char == TEXT('.') || Char == TEXT('-'))
		{
			bWordStart = true;
			continue;
		}
		Name.AppendChar(bWordStart ? TChar<TCHAR>::ToUpper(Char) : Char);
		bWordStart = false;
	}
	return Name;
}

FString MasterName(FName Master, FName Layer)
{
	FString Name = MasterPrefix + PascalCase(Master.ToString());
	if (!Layer.IsNone())
	{
		Name += PascalCase(Layer.ToString());
	}
	return Name;
}

FString MasterPackage(FName Master, FName Layer)
{
	return FString(GeneratedFolder) / MasterName(Master, Layer);
}

FString InstanceName(FName Palette, FName Working, FName History)
{
	return InstancePrefix + PascalCase(Palette.ToString()) + PartSeparator + PascalCase(Working.ToString())
		+ PartSeparator + PascalCase(History.ToString());
}

FString InstancePackage(FName Palette, FName Working, FName History)
{
	return FString(GeneratedFolder) / InstanceName(Palette, Working, History);
}

FString ObjectPathOf(const FString& PackageName)
{
	int32 Slash = INDEX_NONE;
	PackageName.FindLastChar(TEXT('/'), Slash);
	return PackageName + TEXT(".") + PackageName.RightChop(Slash + 1);
}
}
