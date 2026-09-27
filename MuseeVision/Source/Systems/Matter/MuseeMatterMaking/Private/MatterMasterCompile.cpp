#include "MatterMasterCompile.h"

#include "Materials/Material.h"
#include "MaterialShared.h"
#include "Shader.h"
#include "VertexFactory.h"

namespace Musee::Matter
{
namespace
{
	/** The material quality the process renders at (r.MaterialQualityLevel): Num asks the engine for its active level. */
	constexpr EMaterialQualityLevel::Type ActiveQuality = EMaterialQualityLevel::Num;

	/** Every shader Material needs on Platform, as the parallel lists the engine compiles from: one entry per vertex
	 *  factory and shader type, or per vertex factory and shader pipeline. */
	struct FShaderList
	{
		TArray<const FVertexFactoryType*> VertexFactories;
		TArray<const FShaderPipelineType*> Pipelines;
		TArray<const FShaderType*> Shaders;
	};

	FShaderList ShadersOf(UMaterial& Material, EShaderPlatform Platform)
	{
		TArray<FDebugShaderTypeInfo> Types;
		Material.GetShaderTypes(Platform, nullptr, Types);
		FShaderList List;
		for (const FDebugShaderTypeInfo& Type : Types)
		{
			for (const FShaderType* Shader : Type.ShaderTypes)
			{
				List.VertexFactories.Add(Type.VFType);
				List.Pipelines.Add(nullptr);
				List.Shaders.Add(Shader);
			}
			for (const FDebugShaderPipelineInfo& Pipeline : Type.Pipelines)
			{
				List.VertexFactories.Add(Type.VFType);
				List.Pipelines.Add(Pipeline.Pipeline);
				List.Shaders.Add(nullptr);
			}
		}
		return List;
	}
}

FMasterCompile CompileMasterShaders(UMaterial& Material, EShaderPlatform Platform, ERHIFeatureLevel::Type FeatureLevel)
{
	FMasterCompile Compile;
	const FShaderList List = ShadersOf(Material, Platform);
	for (int32 Index = 0; Index < List.Shaders.Num(); ++Index)
	{
		const TCHAR* Factory = List.VertexFactories[Index] ? List.VertexFactories[Index]->GetName() : TEXT("no vertex factory");
		const TCHAR* Shader = List.Shaders[Index] ? List.Shaders[Index]->GetName() : List.Pipelines[Index]->GetName();
		Compile.ShaderTypes.Add(FString::Printf(TEXT("%s: %s"), Factory, Shader));
	}
	Material.CacheGivenTypesForCooking(Platform, FeatureLevel, ActiveQuality, List.VertexFactories, List.Pipelines, List.Shaders);

	FMaterialResource* Resource = Material.GetMaterialResource(Platform, ActiveQuality);
	if (!Resource)
	{
		Compile.Errors.Add(FString::Printf(TEXT("%s made no material resource for the shader platform."), *Material.GetPathName()));
		return Compile;
	}
	Resource->FinishCompilation();
	Compile.Errors = Resource->GetCompileErrors();
	const FMaterialShaderMap* ShaderMap = Resource->GetGameThreadShaderMap();
	Compile.ShaderCount = ShaderMap ? ShaderMap->GetShaderNum() : 0;
	// A failed compile leaves errors and no shader map. FMaterialShaderMap::CompiledSuccessfully is no witness: the
	// engine sets it only when material maps are cached in the DDC (IsMaterialMapDDCEnabled), which the editor's shader
	// job cache turns off. Completeness is the material's own flag, set when the compiled map is handed to it.
	Compile.bCompiled = Compile.Errors.IsEmpty() && ShaderMap && Resource->IsGameThreadShaderMapComplete() && Compile.ShaderCount > 0;
	if (!Compile.bCompiled && Compile.Errors.IsEmpty())
	{
		Compile.Errors.Add(FString::Printf(TEXT("%s has an incomplete shader map for the shader platform (%u shaders from %d listed shader types and pipelines), and the compiler reported no error."),
			*Material.GetPathName(), Compile.ShaderCount, List.Shaders.Num()));
	}
	return Compile;
}
}
