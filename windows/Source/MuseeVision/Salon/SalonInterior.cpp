#include "Salon/SalonInterior.h"

#include "Furniture/FurnitureKit.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/** Plan metres throughout (x east, plan y south, z up), written in centimetres by SalonKit::FMeshData. */
namespace SalonInteriorBuild
{
	using namespace FurnitureKit;
	using SalonKit::FProfile;

	const FVector kUp(0.0, 0.0, 1.0);

	UProceduralMeshComponent* MakeMesh(AActor* Owner, const TCHAR* Name, USceneComponent* Root, bool bCollide)
	{
		UProceduralMeshComponent* M = Owner->CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(Root);
		M->bUseAsyncCooking = true;
		M->bUseComplexAsSimpleCollision = true;
		if (bCollide)
		{
			M->SetCollisionProfileName(TEXT("BlockAll"));
			M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return M;
	}

	TSoftObjectPtr<UMaterialInterface> Soft(const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); }

	/** Sets each section's material, or its fallback (so nothing shows the default material). */
	void ApplyAll(UProceduralMeshComponent* Target, const TArray<TSoftObjectPtr<UMaterialInterface>>& Wanted, const TArray<const TCHAR*>& Fallbacks)
	{
		if (!Target) { return; }
		for (int32 s = 0; s < Wanted.Num(); ++s)
		{
			UMaterialInterface* M = Wanted[s].IsNull() ? nullptr : Wanted[s].LoadSynchronous();
			if (!M && Fallbacks.IsValidIndex(s) && Fallbacks[s]) { M = Cast<UMaterialInterface>(FSoftObjectPath(Fallbacks[s]).TryLoad()); }
			if (M) { Target->SetMaterial(s, M); }
		}
	}

	FSectionFn Level(double Half, double Z0, double Z1, double Rad, int32 Seg = 4, int32 Sub = 1)
	{
		return RectSection(0.0, 0.5 * (Z0 + Z1), Half, 0.5 * (Z1 - Z0), Rad, Seg, Sub);
	}

	FBarEnds Rounded(double E, int32 Steps = 4)
	{
		FBarEnds Ends;
		Ends.E0 = Ends.E1 = E;
		Ends.RoundSteps = Steps;
		return Ends;
	}

	// ================================================================ The benches

	constexpr double kBenchHalfL = 1.2, kBenchHalfW = 0.3, kBenchTop = 0.44, kPlinthTop = 0.075, kPlinthInset = 0.04;
	constexpr double kBenchY = 2.6;
	// Bays 2–5, on each bay's centre (the axial viewing stone's x).
	constexpr double kBenchX[4] = {-32.0, -44.0, -56.0, -68.3};

	void Bench(FMeshData* S, const FVector2D& C, int32 Velvet)
	{
		const FVector O(C.X, C.Y, 0.0);
		const FPath Along = FPath::Line(O + FVector(-kBenchHalfL, 0, 0), O + FVector(kBenchHalfL, 0, 0));
		// The plinth: fumed oak, 75 mm, set back 40 mm all round (a shadow line under the upholstery), its arrises eased.
		Bar(S[0], FPath::Line(O + FVector(-kBenchHalfL + kPlinthInset, 0, 0), O + FVector(kBenchHalfL - kPlinthInset, 0, 0)),
			Level(kBenchHalfW - kPlinthInset, -0.002, kPlinthTop + 0.002, 0.004, 3, 1), Rounded(0.004, 2));
		// The upholstered body: velvet over its sides, the top edge rolled (30 mm), the bottom edge 12 mm, the ends rounded
		// in plan; its flat top at 0.43, the quilting above it.
		constexpr double RTop = 0.040, RBottom = 0.014, EndR = 0.040, Flat = kBenchTop - 0.016;
		FMeshData& V = S[Velvet];
		{
			const double CB = 0.5 * (kPlinthTop + Flat), HB = 0.5 * (Flat - kPlinthTop);
			Bar(V, Along, [=](double, double Inset)
			{
				return RoundRect(0.0, CB, kBenchHalfW - Inset, HB - Inset, RBottom - Inset, RTop - Inset, 5, 8);
			}, Rounded(EndR, 6));
		}
		// The quilted top: four panels along the bench, each domed 16 mm, pulled down to the seams.
		const double QX = kBenchHalfL - EndR, QY = kBenchHalfW - RTop;
		constexpr int32 Panels = 4, NX = 128, NY = 24;
		V.Patch(NX, NY, [&](int32 I, int32 J)
			{
				const double X = -QX + 2.0 * QX * I / NX, Y = -QY + 2.0 * QY * J / NY;
				const double SX = FMath::Frac((X + QX) / (2.0 * QX) * Panels - 1e-9), SY = (Y + QY) / (2.0 * QY);
				const double Dome = 0.016 * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * SX), 0.45) * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * SY), 0.35);
				return O + FVector(X, Y, Flat + 0.0005 + Dome);
			}, [](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return FVector(0.0, 0.0, 1.0); });
		// Piping: a 4 mm cord round the top at the roll's 45° line, one round the bottom, and one across each seam.
		const double S45 = 1.0 - FMath::Sqrt(0.5), Out45 = 0.0022;
		for (const bool bTop : {true, false})
		{
			const double R = bTop ? RTop : RBottom;
			const double Z = bTop ? Flat - R * S45 + Out45 : kPlinthTop + R * S45 - Out45;
			const TArray<FVector2D> Loop = RoundRect(C.X, C.Y, kBenchHalfL - EndR * S45 + Out45, kBenchHalfW - R * S45 + Out45, EndR * 0.8, EndR * 0.8, 6, 16);
			Bar(V, FPath(AtHeight(Loop, Z), FVector::UpVector, true), [](double, double) { return Circle(0.0, 0.0, 0.004, 10); });
		}
		for (int32 K = 1; K < Panels; ++K)
		{
			const double X = -QX + 2.0 * QX * K / Panels;
			Bar(V, FPath::Line(O + FVector(X, -QY - 0.004, Flat + 0.003), O + FVector(X, QY + 0.004, Flat + 0.003)),
				[](double, double) { return Circle(0.0, 0.0, 0.0035, 8); }, Rounded(0.0035, 2));
		}
		// The hit box: a plain block from the floor to the seat.
		Prism(S[5], Along, -kBenchHalfW, kBenchHalfW, -0.01, kBenchTop);
	}

	// ================================================================ The 1874 case

	/** A box in the case's frame (its centre O, axes Ax along, Ay across), from local (x0, y0, z0) to (x1, y1, z1). */
	struct FCaseFrame
	{
		FVector O, Ax, Ay;
		FVector At(double X, double Y, double Z) const { return O + Ax * X + Ay * Y + FVector(0.0, 0.0, Z); }
		void Box(FMeshData& M, double X0, double Y0, double Z0, double X1, double Y1, double Z1, int32 Faces) const
		{
			auto Face = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N) { M.Rect(A, B, C, D, N); };
			if (Faces & FMeshData::NegX) { Face(At(X0, Y0, Z0), At(X0, Y1, Z0), At(X0, Y1, Z1), At(X0, Y0, Z1), -Ax); }
			if (Faces & FMeshData::PosX) { Face(At(X1, Y0, Z0), At(X1, Y1, Z0), At(X1, Y1, Z1), At(X1, Y0, Z1), Ax); }
			if (Faces & FMeshData::NegY) { Face(At(X0, Y0, Z0), At(X1, Y0, Z0), At(X1, Y0, Z1), At(X0, Y0, Z1), -Ay); }
			if (Faces & FMeshData::PosY) { Face(At(X0, Y1, Z0), At(X1, Y1, Z0), At(X1, Y1, Z1), At(X0, Y1, Z1), Ay); }
			if (Faces & FMeshData::NegZ) { Face(At(X0, Y0, Z0), At(X1, Y0, Z0), At(X1, Y1, Z0), At(X0, Y1, Z0), -kUp); }
			if (Faces & FMeshData::PosZ) { Face(At(X0, Y0, Z1), At(X1, Y0, Z1), At(X1, Y1, Z1), At(X0, Y1, Z1), kUp); }
		}
		/** A document lying on the felt: a sheet with its thickness, the image on its face (UV 0…1, top edge away from the reader). */
		void Sheet(FMeshData& M, double CX, double CY, double W, double H, double Z, double Thick, double Tilt) const
		{
			// Its far edge raised by Tilt, so it leans towards the reader (who stands on the axis's side, +Ay).
			const int32 Base = M.Positions.Num();
			const FVector N = (kUp + Ay * FMath::Sin(Tilt)).GetSafeNormal();
			auto P = [&](double U, double V)
			{
				const double X = CX + (U - 0.5) * W, Y = CY + (V - 0.5) * H;
				return At(X, Y, Z + Thick + (1.0 - V) * H * FMath::Tan(Tilt));   // V 0: the far edge
			};
			M.Vertex(P(0, 0), N, FVector2D(0, 0));
			M.Vertex(P(1, 0), N, FVector2D(1, 0));
			M.Vertex(P(1, 1), N, FVector2D(1, 1));
			M.Vertex(P(0, 1), N, FVector2D(0, 1));
			M.Quad(Base, Base + 1, Base + 2, Base + 3);
		}
	};

	// ================================================================ The oval's wall (Plan.Oval: a 11 × b 7.5 about x −86.5)

	constexpr double kOvalX = -86.5, kOvalA = 11.0, kOvalB = 7.5, kDoorHalf = 1.5;

	/** The wall from the door's north jamb, north, west and south, round to its south jamb: dense points and their lengths. */
	struct FWallLine
	{
		TArray<FVector2D> P;
		TArray<FVector2D> In;    // unit normals into the room
		TArray<double> S;

		static const FWallLine& Get()
		{
			static FWallLine Line = []
			{
				FWallLine L;
				const double TJ = FMath::Asin(kDoorHalf / kOvalB);
				constexpr int32 N = 8000;
				for (int32 i = 0; i <= N; ++i)
				{
					// From T = −TJ (the north jamb, y −1.5) down through −π/2 (north), −π (west), −3π/2 (south) to −(2π − TJ).
					const double T = -TJ - (2.0 * UE_DOUBLE_PI - 2.0 * TJ) * i / N;
					const FVector2D Q(kOvalX + kOvalA * FMath::Cos(T), kOvalB * FMath::Sin(T));
					const FVector2D G = FVector2D((Q.X - kOvalX) / (kOvalA * kOvalA), Q.Y / (kOvalB * kOvalB)).GetSafeNormal();
					L.S.Add(i == 0 ? 0.0 : L.S.Last() + FVector2D::Distance(L.P.Last(), Q));
					L.P.Add(Q);
					L.In.Add(-G);
				}
				return L;
			}();
			return Line;
		}

		void At(double Along, FVector2D& OutP, FVector2D& OutIn) const
		{
			const int32 N = S.Num();
			if (Along <= 0.0) { OutP = P[0]; OutIn = In[0]; return; }
			if (Along >= S[N - 1]) { OutP = P[N - 1]; OutIn = In[N - 1]; return; }
			int32 Lo = 0, Hi = N - 1;
			while (Hi - Lo > 1) { const int32 Mid = (Lo + Hi) / 2; (S[Mid] <= Along ? Lo : Hi) = Mid; }
			const double T = (Along - S[Lo]) / FMath::Max(1e-12, S[Hi] - S[Lo]);
			OutP = FMath::Lerp(P[Lo], P[Hi], T);
			OutIn = FMath::Lerp(In[Lo], In[Hi], T).GetSafeNormal();
		}
	};

}

// ==================================================================== ASalonBenches

ASalonBenches::ASalonBenches()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Pieces = SalonInteriorBuild::MakeMesh(this, TEXT("Pieces"), RootComponent, true);
	OakMaterial = SalonInteriorBuild::Soft(TEXT("/Game/Museum/Materials/M_Oak_Fumed.M_Oak_Fumed"));
	for (int32 Bay = 2; Bay <= 5; ++Bay)
	{
		VelvetMaterials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
			FString::Printf(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Velvet_%d.MI_Salon_Velvet_%d"), Bay, Bay))));
	}
	Tags.AddUnique(FName(TEXT("musee.building")));
}

TArray<FVector2D> ASalonBenches::GetBenchCentres()
{
	TArray<FVector2D> Out;
	for (const double X : SalonInteriorBuild::kBenchX)
	{
		Out.Add(FVector2D(X, -SalonInteriorBuild::kBenchY));
		Out.Add(FVector2D(X, SalonInteriorBuild::kBenchY));
	}
	return Out;
}

void ASalonBenches::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void ASalonBenches::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
}

void ASalonBenches::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	using namespace SalonInteriorBuild;
	FMeshData S[6];
	const TArray<FVector2D> Centres = GetBenchCentres();
	for (int32 i = 0; i < Centres.Num(); ++i) { Bench(S, Centres[i], 1 + i / 2); }
	Pieces->ClearAllMeshSections();
	for (int32 s = 0; s < 5; ++s) { S[s].Write(Pieces, s, false); }
	S[5].Write(Pieces, 5, true);
	Pieces->SetMeshSectionVisible(5, false);
}

void ASalonBenches::ApplyMaterials()
{
	TArray<TSoftObjectPtr<UMaterialInterface>> Wanted = {OakMaterial};
	Wanted.Append(VelvetMaterials);
	Wanted.Add(OakMaterial);   // the hit boxes (never drawn)
	const TArray<const TCHAR*> Fallbacks = {TEXT("/Game/Museum/Materials/M_Wood.M_Wood"), TEXT("/Game/Museum/Materials/M_Fabric.M_Fabric"),
		TEXT("/Game/Museum/Materials/M_Fabric.M_Fabric"), TEXT("/Game/Museum/Materials/M_Fabric.M_Fabric"),
		TEXT("/Game/Museum/Materials/M_Fabric.M_Fabric"), TEXT("/Game/Museum/Materials/M_Wood.M_Wood")};
	SalonInteriorBuild::ApplyAll(Pieces, Wanted, Fallbacks);
}

// ==================================================================== ASalonCase

ASalonCase::ASalonCase()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Pieces = SalonInteriorBuild::MakeMesh(this, TEXT("Pieces"), RootComponent, true);
	using SalonInteriorBuild::Soft;
	Materials = {
		Soft(TEXT("/Game/Museum/Materials/M_Oak_Fumed.M_Oak_Fumed")),
		Soft(TEXT("/Game/Museum/Materials/M_Bronze_Patina.M_Bronze_Patina")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_CaseFelt.MI_Salon_CaseFelt")),
		Soft(TEXT("/Game/Museum/Materials/M_Glass.M_Glass")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Catalogue1874.MI_Salon_Catalogue1874")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Charivari1874.MI_Salon_Charivari1874")),
		Soft(TEXT("/Game/Museum/Materials/M_Oak_Fumed.M_Oak_Fumed"))};
	Tags.AddUnique(FName(TEXT("musee.building")));
}

void ASalonCase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void ASalonCase::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
}

void ASalonCase::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	using namespace SalonInteriorBuild;
	enum { Oak, Bronze, Felt, Glass, Catalogue, Charivari, Hit, Count };
	FMeshData S[Count];
	const double B = FMath::DegreesToRadians(double(Bearing));
	FCaseFrame F;
	F.O = FVector(Centre.X, Centre.Y, 0.0);
	F.Ax = FVector(FMath::Cos(B), FMath::Sin(B), 0.0);
	F.Ay = FVector(-FMath::Sin(B), FMath::Cos(B), 0.0);
	constexpr double L = 0.80, W = 0.35;                         // half sizes: 1.6 × 0.7 m
	constexpr double PlinthTop = 0.09, BodyTop = 0.80, RailTop = 0.86, HoodTop = 0.99, Glass0 = RailTop + 0.004;
	auto BarX = [&](double Y0, double Y1, double Z0, double Z1, double X0, double X1, double Rad, FMeshData& M)
	{
		// A bar along the case's length, its section Y0…Y1 × Z0…Z1 (local), its arrises eased.
		const FPath P = FPath::Line(F.At(X0, 0.5 * (Y0 + Y1), 0.0), F.At(X1, 0.5 * (Y0 + Y1), 0.0));
		Bar(M, P, RectSection(0.0, 0.5 * (Z0 + Z1), 0.5 * (Y1 - Y0), 0.5 * (Z1 - Z0), Rad, 3, 1), Rounded(Rad, 2));
	};
	// Plinth (recessed 40 mm), carcase, top rail (12 mm proud, eased).
	BarX(-W + 0.04, W - 0.04, -0.002, PlinthTop + 0.002, -L + 0.04, L - 0.04, 0.003, S[Oak]);
	BarX(-W, W, PlinthTop, BodyTop + 0.002, -L, L, 0.004, S[Oak]);
	BarX(-W - 0.012, W + 0.012, BodyTop, RailTop, -L - 0.012, L + 0.012, 0.006, S[Oak]);
	// The carcase's long faces framed and panelled: stiles and rails 5 mm proud, two fielded panels each side.
	for (const double SY : {-1.0, 1.0})
	{
		const double Y0 = SY < 0 ? -W - 0.005 : W, Y1 = SY < 0 ? -W : W + 0.005;
		const int32 Faces = FMeshData::AllFaces & ~(SY < 0 ? FMeshData::PosY : FMeshData::NegY);
		F.Box(S[Oak], -L, Y0, PlinthTop + 0.02, L, Y1, PlinthTop + 0.08, Faces);        // bottom rail
		F.Box(S[Oak], -L, Y0, BodyTop - 0.07, L, Y1, BodyTop - 0.01, Faces);           // top rail
		for (const double X : {-L, -0.03, L - 0.06})
		{
			F.Box(S[Oak], X, Y0, PlinthTop + 0.08, X + 0.06, Y1, BodyTop - 0.07, Faces);   // stiles
		}
	}
	// The display: the felt floor, a gently sloped mount under Le Charivari.
	F.Box(S[Felt], -L + 0.02, -W + 0.02, RailTop - 0.002, L - 0.02, W - 0.02, RailTop + 0.004, FMeshData::PosZ);
	const double CatX = -0.40, ChaX = 0.28;
	F.Sheet(S[Catalogue], CatX, 0.0, CatalogueSize.X, CatalogueSize.Y, RailTop + 0.004, 0.0015, 0.0);
	// Le Charivari on a felt-covered board raised 10 mm, tilted 6° towards the reader.
	const double BW = CharivariSize.X + 0.04, BH = CharivariSize.Y + 0.04;
	F.Box(S[Felt], ChaX - 0.5 * BW, -0.5 * BH, RailTop + 0.004, ChaX + 0.5 * BW, 0.5 * BH, RailTop + 0.014,
		  FMeshData::PosZ | FMeshData::NegX | FMeshData::PosX | FMeshData::NegY | FMeshData::PosY);
	F.Sheet(S[Charivari], ChaX, 0.0, CharivariSize.X, CharivariSize.Y, RailTop + 0.014, 0.001, FMath::DegreesToRadians(0.0));
	// The hood: low-iron glass, four sides and a top, its twelve edges in 12 mm bronze angles.
	const double GL = L - 0.004, GW = W - 0.004;
	F.Box(S[Glass], -GL, -GW, Glass0, GL, GW, HoodTop, FMeshData::AllFaces & ~FMeshData::NegZ);
	const double E = 0.012;
	for (const double SX : {-1.0, 1.0})
	{
		for (const double SY : {-1.0, 1.0})
		{
			const double X0 = SX < 0 ? -GL - 0.002 : GL - E + 0.002, Y0 = SY < 0 ? -GW - 0.002 : GW - E + 0.002;
			F.Box(S[Bronze], X0, Y0, Glass0, X0 + E, Y0 + E, HoodTop + 0.002, FMeshData::AllFaces & ~FMeshData::NegZ);   // corners
		}
	}
	for (const double Z : {Glass0, HoodTop - E + 0.002})
	{
		for (const double SY : {-1.0, 1.0})
		{
			const double Y0 = SY < 0 ? -GW - 0.002 : GW - E + 0.002;
			F.Box(S[Bronze], -GL, Y0, Z, GL, Y0 + E, Z + E, FMeshData::AllFaces);
		}
		for (const double SX : {-1.0, 1.0})
		{
			const double X0 = SX < 0 ? -GL - 0.002 : GL - E + 0.002;
			F.Box(S[Bronze], X0, -GW, Z, X0 + E, GW, Z + E, FMeshData::AllFaces);
		}
	}
	// A bronze escutcheon on the front rail (the axis's side), for the hood's lock.
	F.Box(S[Bronze], -0.02, W + 0.012, RailTop - 0.045, 0.02, W + 0.016, RailTop - 0.01, FMeshData::AllFaces & ~FMeshData::NegY);
	// The hit box: the whole case.
	F.Box(S[Hit], -L - 0.012, -W - 0.012, -0.01, L + 0.012, W + 0.012, HoodTop + 0.002, FMeshData::AllFaces);

	Pieces->ClearAllMeshSections();
	for (int32 s = 0; s < Hit; ++s) { S[s].Write(Pieces, s, false); }
	S[Hit].Write(Pieces, Hit, true);
	Pieces->SetMeshSectionVisible(Hit, false);
}

void ASalonCase::ApplyMaterials()
{
	const TArray<const TCHAR*> Fallbacks = {TEXT("/Game/Museum/Materials/M_Wood.M_Wood"), TEXT("/Game/Museum/Materials/M_Metal.M_Metal"),
		TEXT("/Game/Museum/Materials/M_Felt.M_Felt"), TEXT("/Game/Museum/Materials/M_Glass.M_Glass"),
		TEXT("/Game/Museum/Materials/M_Felt.M_Felt"), TEXT("/Game/Museum/Materials/M_Felt.M_Felt"), TEXT("/Game/Museum/Materials/M_Wood.M_Wood")};
	SalonInteriorBuild::ApplyAll(Pieces, Materials, Fallbacks);
}

// ==================================================================== ASalonCanvas

ASalonCanvas::ASalonCanvas()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Canvas = SalonInteriorBuild::MakeMesh(this, TEXT("Canvas"), RootComponent, true);
	LinenMaterial = SalonInteriorBuild::Soft(TEXT("/Game/Museum/Materials/M_Frame_Linen.M_Frame_Linen"));
	Tags.AddUnique(FName(TEXT("musee.building")));
}

double ASalonCanvas::WallLength()
{
	const SalonInteriorBuild::FWallLine& L = SalonInteriorBuild::FWallLine::Get();
	return L.S.Last();
}

void ASalonCanvas::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void ASalonCanvas::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
}

void ASalonCanvas::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	using namespace SalonInteriorBuild;
	const FWallLine& Wall = FWallLine::Get();
	FMeshData Face, Edges;
	constexpr double Proud = 0.035, Joint = 0.0015, Step = 0.06;
	const double Z0 = Bottom, Z1 = Bottom + Height;
	const int32 N = FMath::Max(1, Canvases);
	const double W = Length / N;
	auto Point = [&](double Along, double Out, double Z)
	{
		FVector2D P, In;
		Wall.At(Along, P, In);
		return FVector(P.X + In.X * Out, P.Y + In.Y * Out, Z);
	};
	auto Normal = [&](double Along) { FVector2D P, In; Wall.At(Along, P, In); return FVector(In.X, In.Y, 0.0); };
	for (int32 c = 0; c < N; ++c)
	{
		const double S0 = StartAlong + c * W + (c > 0 ? 0.5 * Joint : 0.0);
		const double S1 = StartAlong + (c + 1) * W - (c < N - 1 ? 0.5 * Joint : 0.0);
		const int32 NU = FMath::Max(2, FMath::CeilToInt((S1 - S0) / Step));
		// The painted face: U left to right as seen from the room (the far end of the run first), V down from the top.
		Face.Patch(NU, 2, [&](int32 I, int32 J)
			{
				const double Along = FMath::Lerp(S0, S1, double(I) / NU);
				return Point(Along, Proud, FMath::Lerp(Z1, Z0, double(J) / 2));
			},
			[&](const FVector& P)
			{
				// Recover the arc position from the point: the nearest station (the face is a strip of the wall's offset).
				double Best = S0, BestD = 1e30;
				for (int32 K = 0; K <= NU; ++K)
				{
					const double A = FMath::Lerp(S0, S1, double(K) / NU);
					const FVector Q = Point(A, Proud, P.Z);
					const double D = FVector::DistSquared(Q, P);
					if (D < BestD) { BestD = D; Best = A; }
				}
				return FVector2D(1.0 - (Best - StartAlong) / Length, (Z1 - P.Z) / Height);
			},
			[&](const FVector& P) { return FVector(P.X - kOvalX, P.Y, 0.0) * -1.0; });
		// The turned edges: top and bottom along the run, and each end.
		for (const double Z : {Z0, Z1})
		{
			Edges.Patch(NU, 1, [&](int32 I, int32 J)
				{
					const double Along = FMath::Lerp(S0, S1, double(I) / NU);
					return Point(Along, J == 0 ? 0.002 : Proud, Z);
				}, [](const FVector& P) { return FVector2D(P.X, P.Y); },
				[Z, Z0](const FVector&) { return FVector(0.0, 0.0, Z == Z0 ? -1.0 : 1.0); });
		}
		for (const double Along : {S0, S1})
		{
			// Up × In runs the way the wall's length grows: the run's start faces back along it, its end on.
			const FVector T = FVector::CrossProduct(FVector::UpVector, Normal(Along)) * (Along == S0 ? -1.0 : 1.0);
			Edges.Patch(1, 1, [&](int32 I, int32 J) { return Point(Along, I == 0 ? 0.002 : Proud, J == 0 ? Z0 : Z1); },
						[](const FVector& P) { return FVector2D(P.X + P.Y, P.Z); }, [T](const FVector&) { return T; });
		}
	}
	// Behind each joint between panels, a strip of the stretcher's linen 12 mm back (the flicker check: through the 1.5 mm
	// joint, thinner than a pixel from the room, the lamplit wall behind showed at a different pixel every frame, a line of
	// sparkles; now the joint is what a real one is from across the room, a dark line in its own shadow).
	for (int32 c = 1; c < N; ++c)
	{
		const double J = StartAlong + c * W;
		Edges.Patch(2, 1, [&](int32 I, int32 K)
			{
				return Point(J + (I - 1) * 0.012, Proud - 0.012, K == 0 ? Z0 + 0.002 : Z1 - 0.002);
			}, [](const FVector& P) { return FVector2D(P.X + P.Y, P.Z); },
			[&](const FVector& P) { return FVector(P.X - kOvalX, P.Y, 0.0) * -1.0; });
	}
	Canvas->ClearAllMeshSections();
	Face.Write(Canvas, 0, true);
	Edges.Write(Canvas, 1, false);
}

void ASalonCanvas::ApplyMaterials()
{
	const TArray<const TCHAR*> Fallbacks = {TEXT("/Game/Museum/Materials/M_Frame_Linen.M_Frame_Linen"),
		TEXT("/Game/Museum/Materials/M_Frame_Linen.M_Frame_Linen")};
	SalonInteriorBuild::ApplyAll(Canvas, {PaintingMaterial, LinenMaterial}, Fallbacks);
}

// ASalonPond and ASalonPondPlants: Salon/SalonPond.cpp (the pond-edge redesign).
