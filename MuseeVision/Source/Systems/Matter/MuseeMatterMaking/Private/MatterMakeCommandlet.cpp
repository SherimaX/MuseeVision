#include "MatterMakeCommandlet.h"

#include "HAL/PlatformTime.h"
#include "Hash/Blake3.h"
#include "Materials/Material.h"
#include "MatterMasterAsset.h"
#include "MatterMasterCompile.h"
#include "MatterStoneProbe.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogMuseeMatter, Log, All);

namespace Musee::Matter
{
namespace
{
	/** Makes one master, compiles it and reports; true when it succeeded. */
	bool MakeAndReport(const FMasterRecipe& Recipe)
	{
		const double MakeStart = FPlatformTime::Seconds();
		const FMasterMade Made = MakeMasterAsset(Recipe);
		if (!Made.Succeeded())
		{
			for (const FString& Problem : Made.Problems)
			{
				UE_LOG(LogMuseeMatter, Error, TEXT("%s: %s"), *Recipe.AssetName.ToString(), *Problem);
			}
			return false;
		}
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Made.Filename);
		const FBlake3Hash PackageHash = FBlake3::HashBuffer(Bytes.GetData(), Bytes.Num());
		UE_LOG(LogMuseeMatter, Display, TEXT("Made %s in %.2f s: %d bytes, package BLAKE3 %s, StateId %s."),
			*Made.Material->GetPathName(), FPlatformTime::Seconds() - MakeStart, Bytes.Num(),
			*LexToString(PackageHash), *Made.Material->StateId.ToString());

		const double CompileStart = FPlatformTime::Seconds();
		const FMasterCompile Compile = CompileMasterShaders(*Made.Material, MuseumShaderPlatform, MuseumFeatureLevel);
		for (const FString& Error : Compile.Errors)
		{
			UE_LOG(LogMuseeMatter, Error, TEXT("%s: %s"), *Recipe.AssetName.ToString(), *Error);
		}
		for (const FString& Type : Compile.ShaderTypes)
		{
			UE_LOG(LogMuseeMatter, Display, TEXT("  compiled %s"), *Type);
		}
		UE_LOG(LogMuseeMatter, Display, TEXT("Compiled %s for PCD3D_SM6 in %.2f s: %s, %u shaders."),
			*Recipe.AssetName.ToString(), FPlatformTime::Seconds() - CompileStart,
			Compile.bCompiled ? TEXT("compiled") : TEXT("failed"), Compile.ShaderCount);
		return Compile.bCompiled;
	}
}
}

UMatterMakeCommandlet::UMatterMakeCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UMatterMakeCommandlet::Main(const FString& Params)
{
	const bool bMade = Musee::Matter::MakeAndReport(Musee::Matter::StoneProbeRecipe());
	return bMade ? 0 : 1;
}
