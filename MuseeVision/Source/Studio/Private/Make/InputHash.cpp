#include "Make/InputHash.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	FInputHasher::FInputHasher()
	{
		// Persistent serialisation: the same bytes on every machine and in every process.
		Archive.SetIsPersistent(true);
	}

	FInputHasher& FInputHasher::Add(FStringView Text)
	{
		const FTCHARToUTF8 Utf8(Text.GetData(), Text.Len());
		int64 Length = Utf8.Length();
		Archive << Length;
		// The hashing archive only reads the bytes it is given.
		Archive.Serialize(const_cast<void*>(static_cast<const void*>(Utf8.Get())), Length);
		return *this;
	}

	FInputHasher& FInputHasher::Add(int64 Value)
	{
		Archive << Value;
		return *this;
	}

	FInputHasher& FInputHasher::Add(double Value)
	{
		Archive << Value;
		return *this;
	}

	FInputHasher& FInputHasher::Add(const FVector& Value)
	{
		return Add(Value.X).Add(Value.Y).Add(Value.Z);
	}

	FInputHasher& FInputHasher::Add(const FQuat& Value)
	{
		return Add(Value.X).Add(Value.Y).Add(Value.Z).Add(Value.W);
	}

	FInputHasher& FInputHasher::Add(FMeshDescription& Description)
	{
		Archive << Description;
		return *this;
	}

	FString FInputHasher::Finish()
	{
		return LexToString(Archive.Finalize());
	}

	FName InputHashKey()
	{
		static const FName Key(TEXT("studio.input-hash"));
		return Key;
	}

	FString StoredInputHash(const FString& PackageName, const FString& AssetName)
	{
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		if (UObject* Loaded = FindObject<UObject>(nullptr, *ObjectPath); IsValid(Loaded))
		{
			return Loaded->GetPackage()->GetMetaData().GetValue(Loaded, InputHashKey());
		}
		FString Filename;
		if (!FPackageName::DoesPackageExist(PackageName, &Filename))
		{
			return FString();
		}
		// Read the package's registry section afresh: a make in another process may have rewritten it.
		IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		Registry.ScanFilesSynchronous({Filename}, /*bForceRescan*/ true);
		const FAssetData Asset = Registry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath), /*bIncludeOnlyOnDiskAssets*/ true);
		FString Hash;
		Asset.GetTagValue(InputHashKey(), Hash);
		return Hash;
	}

	void StoreInputHash(UObject& Asset, const FString& Hash)
	{
		Asset.GetPackage()->GetMetaData().SetValue(&Asset, InputHashKey(), *Hash);
	}

	const FString* FindInputHash(const UObject& Asset)
	{
		UPackage* Package = Asset.GetPackage();
		if (!Package || Package->HasAnyPackageFlags(PKG_CompiledIn))
		{
			return nullptr;
		}
		return Package->GetMetaData().FindValue(&Asset, InputHashKey());
	}
}
