#include "Cast/CastRules.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPath.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioCastRulesTest, "Musee.Studio.CastRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioCastRulesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	const UMaterialInterface* Opaque = UMaterial::GetDefaultMaterial(MD_Surface);
	const UMaterialInterface* Translucent = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent")).TryLoad());
	if (!TestNotNull(TEXT("the stock translucent material loads"), Translucent))
	{
		return false;
	}
	TestTrue(TEXT("the default surface material is opaque"), KindOf(*Opaque) == ECastKind::Opaque);
	TestTrue(TEXT("a translucent blend mode is translucent"), KindOf(*Translucent) == ECastKind::Translucent);

	const FMeshBuildSettings Stone = BuildSettingsFor(ECastKind::Opaque, 3.0);
	TestFalse(TEXT("normals are kept"), Stone.bRecomputeNormals);
	TestFalse(TEXT("tangents are kept"), Stone.bRecomputeTangents);
	TestFalse(TEXT("no lightmap UVs"), Stone.bGenerateLightmapUVs);
	TestTrue(TEXT("full-precision UVs"), Stone.bUseFullPrecisionUVs);
	TestEqual(TEXT("an opaque part has a distance field"), Stone.DistanceFieldResolutionScale, 1.0f);
	TestEqual(TEXT("a compact piece gets the default 12 cards"), Stone.MaxLumenMeshCards, 12);
	TestEqual(TEXT("a room-scale piece gets 32 cards"), BuildSettingsFor(ECastKind::Opaque, 28.0).MaxLumenMeshCards, 32);

	const FMeshBuildSettings Glass = BuildSettingsFor(ECastKind::Translucent, 3.0);
	TestEqual(TEXT("a translucent part has no distance field"), Glass.DistanceFieldResolutionScale, 0.0f);
	TestEqual(TEXT("a translucent part has no cards"), Glass.MaxLumenMeshCards, 0);

	const FMeshNaniteSettings StoneNanite = NaniteSettingsFor(ECastKind::Opaque);
	TestTrue(TEXT("Nanite is on for opaque parts"), StoneNanite.bEnabled);
	TestTrue(TEXT("Nanite keeps the drawn tangents"), StoneNanite.bExplicitTangents);
	TestTrue(TEXT("the fallback keeps every triangle"), StoneNanite.FallbackTarget == ENaniteFallbackTarget::PercentTriangles
		&& StoneNanite.FallbackPercentTriangles == 1.0f && StoneNanite.GenerateFallback == ENaniteGenerateFallback::Enabled);
	TestFalse(TEXT("Nanite is off for translucent parts"), NaniteSettingsFor(ECastKind::Translucent).bEnabled);

	TestEqual(TEXT("the opaque part keeps the cast's name"), FString(PartSuffix(ECastKind::Opaque)), FString());
	TestEqual(TEXT("the translucent part is named apart"), FString(PartSuffix(ECastKind::Translucent)), FString(TEXT("_Translucent")));
	return true;
}

#endif
