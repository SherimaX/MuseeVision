#pragma once

#include "CoreMinimal.h"
#include "RHIDefinitions.h"
#include "RHIShaderPlatform.h"

class UMaterial;

/**
 * Compiling a master's shaders for one shader platform, to prove the master and the shading library it includes
 * compile, and to report the compiler's errors when they do not.
 *
 * It asks the material for every shader type it needs on the platform (UMaterial::GetShaderTypes) and compiles exactly
 * those, blocking, through the engine's cook path (UMaterial::CacheGivenTypesForCooking). That path does not depend on
 * the material's rendering resources, which exist only where the process can render, so it works without an RHI
 * (-nullrhi), in a commandlet or an unattended editor.
 */
namespace Musee::Matter
{
	/** The museum's desktop shader platform: Direct3D 12 at Shader Model 6 (Config/DefaultEngine.ini). */
	inline constexpr EShaderPlatform MuseumShaderPlatform = SP_PCD3D_SM6;
	inline constexpr ERHIFeatureLevel::Type MuseumFeatureLevel = ERHIFeatureLevel::SM6;

	/** What compiling a master for one platform produced. */
	struct FMasterCompile
	{
		bool bCompiled = false;
		/** The material translator's and the shader compiler's errors. */
		TArray<FString> Errors;
		/** The shaders in the compiled shader map. */
		uint32 ShaderCount = 0;
		/** What was asked of the compiler: each vertex factory with its shader type or shader pipeline. */
		TArray<FString> ShaderTypes;
	};

	/** Compiles every shader Material needs on Platform, whose feature level is FeatureLevel, blocking until done. */
	FMasterCompile CompileMasterShaders(UMaterial& Material, EShaderPlatform Platform, ERHIFeatureLevel::Type FeatureLevel);
}
