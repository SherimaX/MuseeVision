#include "Nature/MuseeNatureActor.h"

#include "MuseeVision.h"
#include "Nature/NatureMesh.h"
#include "Sky/Ephemeris.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"
#include "UObject/UnrealType.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"

AMuseeNatureActor::AMuseeNatureActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Origin = CreateDefaultSubobject<USceneComponent>(TEXT("Origin"));
	Origin->SetMobility(EComponentMobility::Static);
	RootComponent = Origin;
}

int32 AMuseeNatureActor::EffectiveTerm() const
{
	if (SolarTerm >= 0) { return SolarTerm % 24; }
	// The middle of each season: Chunfen, Xiazhi, Qiufen, Dongzhi.
	switch (Season)
	{
	case EMuseeNatureSeason::Spring: return 3;
	case EMuseeNatureSeason::Summer: return 9;
	case EMuseeNatureSeason::Autumn: return 15;
	default: return 21;
	}
}

EMuseeNatureSeason AMuseeNatureActor::EffectiveSeason() const
{
	if (SolarTerm < 0) { return Season; }
	return static_cast<EMuseeNatureSeason>(FMath::Clamp((SolarTerm % 24) / 6, 0, 3));
}

void AMuseeNatureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RequestGrow();
}

void AMuseeNatureActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	RequestGrow();
}

void AMuseeNatureActor::RequestGrow()
{
	UWorld* World = GetWorld();
	if (!World || HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) { return; }
	if (World->IsGameWorld())
	{
		EnsureGrown(false);
		return;
	}
	if (bGrowPending) { return; }
	bGrowPending = true;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		bGrowPending = false;
		EnsureGrown(false);
		return false;
	}));
}

void AMuseeNatureActor::BeginPlay()
{
	Super::BeginPlay();
	if (bFollowToday)
	{
		int32 Term = MuseeEphemeris::SolarTerm(FDateTime::UtcNow());
		if (FMuseeObserver::FromTimeZone().Latitude < 0) { Term = (Term + 12) % 24; }
		SolarTerm = Term;
		Season = static_cast<EMuseeNatureSeason>(FMath::Clamp(Term / 6, 0, 3));
	}
	EnsureGrown(false);
}

void AMuseeNatureActor::Regrow()
{
	EnsureGrown(true);
}

uint32 AMuseeNatureActor::PropertyKey() const
{
	// Everything a plant is grown from: the editable properties declared on the plant classes.
	FString Text;
	for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_Transient | CPF_EditConst)) { continue; }
		if (CastField<FObjectPropertyBase>(Prop)) { continue; }   // components (collision), not growth
		const UClass* Declared = Prop->GetOwnerClass();
		if (!Declared || !Declared->IsChildOf(AMuseeNatureActor::StaticClass())) { continue; }
		FString Value;
		Prop->ExportTextItem_Direct(Value, Prop->ContainerPtrToValuePtr<void>(this), nullptr, nullptr, PPF_None);
		Text += Prop->GetName();
		Text += TEXT("=");
		Text += Value;
		Text += TEXT(";");
	}
	return GetTypeHash(Text);
}

void AMuseeNatureActor::EnsureGrown(bool bForce)
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	UWorld* World = GetWorld();
	if (!World || HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || IsPendingKillPending()) { return; }
	const bool bMovable = IsMovablePlant();
	if (!Plant)
	{
		Plant = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
		Plant->SetMobility(bMovable ? EComponentMobility::Movable : EComponentMobility::Static);
		Plant->SetupAttachment(GetRootComponent());
		Plant->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plant->SetCanEverAffectNavigation(false);
		Plant->bUseAsyncCooking = true;
		Plant->SetCastShadow(true);
		// Wind moves the leaves a few centimetres past the mesh's own bounds.
		Plant->SetBoundsScale(1.1f);
		Plant->RegisterComponent();
	}
	const uint32 Key = PropertyKey();
	if (!bForce && Key == GrownKey && Plant->GetNumSections() > 0) { return; }
	GrownKey = Key;
	Plant->ClearAllMeshSections();
	TriangleCount = 0;
	const double Start = FPlatformTime::Seconds();
	BuildPlant();
	UE_LOG(LogMusee, Verbose, TEXT("Nature: %s grown, %d triangles in %.0f ms."), *GetActorNameOrLabel(), TriangleCount,
		   (FPlatformTime::Seconds() - Start) * 1000.0);
}

void AMuseeNatureActor::WriteSection(int32 Index, const FNatureMesh& Mesh, const TCHAR* MaterialName)
{
	if (!Plant) { return; }
	if (Mesh.IsEmpty())
	{
		Plant->ClearMeshSection(Index);
		return;
	}
	Plant->CreateMeshSection(Index, Mesh.Vertices, Mesh.Triangles, Mesh.Normals, Mesh.UV0, Mesh.UV1, Mesh.UV2, Mesh.UV3, Mesh.Colors,
							 Mesh.Tangents, false);
	if (UMaterialInterface* Material = MuseeNature::LoadMaterial(MaterialName)) { Plant->SetMaterial(Index, Material); }
	TriangleCount += Mesh.NumTriangles();
}
