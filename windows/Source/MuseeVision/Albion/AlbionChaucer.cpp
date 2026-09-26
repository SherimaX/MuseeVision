#include "Albion/AlbionChaucer.h"

#include "Albion/AlbionKit.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture2D.h"
#include "Geometry/MuseeBake.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MuseeVision.h"
#include "ProceduralMeshComponent.h"
#include "Visitor/MuseeCharacter.h"

#define LOCTEXT_NAMESPACE "AlbionChaucer"

/**
 * The lectern and the book, in the actor's frame (metres): X the way the reader faces, Y to the reader's right, Z up; the
 * origin at the lectern's foot. The desk slopes up away from the reader at 18°; the book lies on it, its spine up the
 * slope, the left-hand page at −Y and the right at +Y.
 */
namespace AlbionChaucerImpl
{
	using namespace AlbionKit;

	constexpr double kSlope = 18.0;                    // degrees
	constexpr double kDeskFar = 1.20, kDeskDepth = 0.60, kDeskWidth = 0.74, kDeskBoard = 0.03;
	constexpr double kLip = 0.035;                     // the book rests on a ledge this far up the desk
	constexpr double kPageH = 0.425, kPageW = 0.290;   // a folio leaf
	constexpr double kBoard = 0.003, kSquare = 0.005;  // the boards and their overhang
	constexpr double kBlock = 0.066;                   // the whole book's leaves
	constexpr int32 kLeaves = 277;                     // 554 pages
	constexpr int32 NU = 14, NS = 28;                  // the pages' grids: along the spine, across the page

	/** The desk's top surface frame: S0 its near edge's middle; U up the slope, V to the right, N out of it. */
	struct FDesk
	{
		FVector S0, U, V, N;
		FDesk()
		{
			const double A = FMath::DegreesToRadians(kSlope);
			U = FVector(FMath::Cos(A), 0.0, FMath::Sin(A));
			V = FVector(0.0, 1.0, 0.0);
			N = FVector(-FMath::Sin(A), 0.0, FMath::Cos(A));
			S0 = FVector(-0.5 * kDeskDepth * FMath::Cos(A), 0.0, kDeskFar - kDeskDepth * FMath::Sin(A));
		}
		FVector At(double Uu, double Vv, double Nn) const { return S0 + U * Uu + V * Vv + N * Nn; }
	};

	/** A stack's top: it rises from the gutter and arches a little across the page. D: distance from the spine (m). */
	double StackTop(double H, double D)
	{
		const double Rise = 1.0 - FMath::Exp(-FMath::Max(D, 0.0) / 0.018);
		const double Arch = 0.006 * FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(D / kPageW, 0.0, 1.0));
		return kBoard + 0.001 + H * Rise + Arch * Rise;
	}

	double LeftH(int32 Opening) { return 0.003 + kBlock * FMath::Clamp(double(2 * Opening) / (2.0 * kLeaves), 0.0, 1.0); }
	double RightH(int32 Opening) { return 0.003 + kBlock * (1.0 - FMath::Clamp(double(2 * Opening) / (2.0 * kLeaves), 0.0, 1.0)); }

	double EaseInOut(double T) { return T * T * (3.0 - 2.0 * T); }

	/** A grid of points G[i][j] (i along the spine 0 … NU, j across 0 … NSeg) with its UVs, normals from the grid, each
	 *  turned to agree with Hint (a direction the face's normal should broadly have). */
	void Grid(FMeshData& M, const TArray<FVector>& G, const TArray<FVector2D>& UV, int32 NSeg, const TFunction<FVector(int32, int32)>& Hint)
	{
		const int32 W = NSeg + 1;
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NU; ++i)
		{
			for (int32 j = 0; j <= NSeg; ++j)
			{
				const FVector& P = G[i * W + j];
				const FVector DU = G[FMath::Min(i + 1, NU) * W + j] - G[FMath::Max(i - 1, 0) * W + j];
				const FVector DS = G[i * W + FMath::Min(j + 1, NSeg)] - G[i * W + FMath::Max(j - 1, 0)];
				FVector N = FVector::CrossProduct(DU, DS);
				if (!N.Normalize(1e-18)) { N = Hint(i, j); }
				if (FVector::DotProduct(N, Hint(i, j)) < 0.0) { N = -N; }
				M.Vertex(P, N, UV[i * W + j]);
			}
		}
		for (int32 i = 0; i < NU; ++i)
		{
			for (int32 j = 0; j < NSeg; ++j) { M.Quad(Base + i * W + j, Base + (i + 1) * W + j, Base + (i + 1) * W + j + 1, Base + i * W + j + 1); }
		}
	}

	/** A resting page on a stack: Side −1 left, +1 right. Its texture's left edge is the fore-edge on the left, the gutter on the right. */
	void RestingPage(FMeshData& M, const FDesk& D, int32 Side, double H)
	{
		TArray<FVector> G;
		TArray<FVector2D> UV;
		for (int32 i = 0; i <= NU; ++i)
		{
			const double Uu = kLip + kSquare + kPageH * i / NU;
			for (int32 j = 0; j <= NS; ++j)
			{
				const double S = kPageW * j / NS;   // from the spine
				G.Add(D.At(Uu, Side * S, StackTop(H, S) + 0.0004));
				const double TexU = Side > 0 ? S / kPageW : 1.0 - S / kPageW;
				UV.Add(FVector2D(TexU, 1.0 - double(i) / NU));
			}
		}
		Grid(M, G, UV, NS, [&D](int32, int32) { return D.N; });
	}

	/**
	 * The turning leaf at T (0 … 1), forward (right to left) or, mirrored, back. Its shape across the page: the angle
	 * from the right-hand stack rises with T, the free edge trailing in a curl that is greatest mid-turn and the page's
	 * head lagging its foot (the reader lifts the lower corner); eased in from the stack it leaves and onto the one it
	 * reaches. bUpSide: the side facing up at the start (its texture reads from the spine on the right-hand page).
	 */
	void TurningLeaf(FMeshData& M, const FDesk& D, double T, bool bForward, double HFrom, double HTo, bool bUpSide)
	{
		const double Te = EaseInOut(T);
		const double A = UE_DOUBLE_PI * Te;
		const double Curl = 1.15 * FMath::Sin(UE_DOUBLE_PI * T);
		const double Lift = 0.004;
		const double Mirror = bForward ? 1.0 : -1.0;
		const double LeafOff = bUpSide ? 0.00015 : -0.00015;
		TArray<FVector> G;
		TArray<FVector2D> UV;
		TArray<FVector> Hints;
		for (int32 i = 0; i <= NU; ++i)
		{
			const double Uu = kLip + kSquare + kPageH * i / NU;
			const double Head = double(i) / NU;
			double Vv = 0.0, Nn = kBoard + 0.002 + Lift * FMath::Sin(UE_DOUBLE_PI * T);
			double PrevPhi = 0.0;
			for (int32 j = 0; j <= NS; ++j)
			{
				const double S = kPageW * j / NS;
				const double Phi = A - Curl * FMath::Pow(S / kPageW, 1.5) * (0.6 + 0.8 * Head) * (1.0 - Te * 0.35);
				if (j > 0)
				{
					const double Ds = kPageW / NS;
					const double Pm = 0.5 * (Phi + PrevPhi);
					Vv += FMath::Cos(Pm) * Ds;
					Nn += FMath::Sin(Pm) * Ds;
				}
				PrevPhi = Phi;
				// Where the leaf lies at rest: on the stack it leaves (at +S) and on the one it reaches (at −S).
				const FVector2D From(S, StackTop(HFrom, S) + 0.0006);
				const FVector2D To(-S, StackTop(HTo, S) + 0.0006);
				FVector2D Here(Vv, Nn);
				const double In = FMath::SmoothStep(0.0, 0.16, T), Out = FMath::SmoothStep(0.84, 1.0, T);
				Here = FMath::Lerp(From, Here, In);
				Here = FMath::Lerp(Here, To, Out);
				// The side's normal in (v, n): the leaf's up side faces (−sin φ, cos φ).
				const FVector2D Nrm2(-FMath::Sin(Phi), FMath::Cos(Phi));
				const FVector2D Off = Nrm2 * LeafOff;
				G.Add(D.At(Uu, Mirror * (Here.X + Off.X), Here.Y + Off.Y));
				const double TexU = bUpSide ? S / kPageW : 1.0 - S / kPageW;
				UV.Add(FVector2D(TexU, 1.0 - double(i) / NU));
				const FVector Hn = D.V * (Mirror * Nrm2.X) + D.N * Nrm2.Y;
				Hints.Add(bUpSide ? Hn : -Hn);
			}
		}
		Grid(M, G, UV, NS, [&Hints](int32 i, int32 j) { return Hints[i * (NS + 1) + j]; });
	}
}

AAlbionChaucer::AAlbionChaucer()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	auto Make = [this](const TCHAR* Name, bool bCollision)
	{
		UProceduralMeshComponent* C = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		return C;
	};
	Lectern = Make(TEXT("Lectern"), true);
	Block = Make(TEXT("Block"), true);
	Pages = Make(TEXT("Pages"), false);
	Pages->SetMobility(EComponentMobility::Movable);
	Pages->bUseAsyncCooking = true;
	// The pages answer the look (the visitor's trace), without blocking anyone.
	Pages->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Pages->SetCollisionResponseToAllChannels(ECR_Ignore);
	Pages->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MuseeBake::NoBake(Pages);
	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	OakMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_Oak.MI_Albion_Oak"));   // the museum's oak, Victorian finish
	BoardMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_BookBoards.MI_Albion_BookBoards"));
	ClothMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_BookCloth.MI_Albion_BookCloth"));
	EdgeMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_PaperEdge.MI_Albion_PaperEdge"));
	PageMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_Page.MI_Albion_Page"));
	Tags.AddUnique(MuseeBake::BakeableTag());
	Tags.AddUnique(FName(TEXT("work:albion-kelmscott-chaucer")));
}

void AAlbionChaucer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildStatic();
	BuildPages();
}

void AAlbionChaucer::BeginPlay()
{
	Super::BeginPlay();
	BuildStatic();
	LoadPages();
	BuildPages();
}

void AAlbionChaucer::BuildStatic()
{
	using namespace AlbionChaucerImpl;
	if (MuseeBake::IsBaked(this)) { return; }
	const FDesk D;
	FMeshData Oak, Boards, Cloth, Edges;
	// The lectern: a cross foot, a chamfered post tapering up, two brackets, the sloped desk with its ledge and a fascia.
	{
		auto Bar = [&](const FVector& A, const FVector& B, double HA, double HB, const FVector& Up)
		{
			FurnitureKit::FBarEnds Ends;
			Ends.E0 = Ends.E1 = 0.004;
			FurnitureKit::Bar(Oak, FurnitureKit::FPath::Line(A, B, Up), FurnitureKit::RectSection(0.0, 0.0, HA, HB, 0.006), Ends);
		};
		Bar(FVector(-0.30, 0.0, 0.04), FVector(0.30, 0.0, 0.04), 0.045, 0.04, FVector(0, 0, 1));
		Bar(FVector(0.0, -0.30, 0.04), FVector(0.0, 0.30, 0.04), 0.045, 0.04, FVector(0, 0, 1));
		for (const FVector2D& Pad : {FVector2D(-0.28, 0), FVector2D(0.28, 0), FVector2D(0, -0.28), FVector2D(0, 0.28)})
		{
			Oak.Box(FVector(Pad.X - 0.05, Pad.Y - 0.05, 0.0), FVector(Pad.X + 0.05, Pad.Y + 0.05, 0.012), FMeshData::AllFaces);
		}
		// The post, square with stopped chamfers, 0.11 m tapering to 0.085 m.
		FurnitureKit::FBarEnds PostEnds;
		FurnitureKit::Bar(Oak, FurnitureKit::FPath::Line(FVector(0, 0, 0.07), FVector(0, 0, 0.90), FVector(1, 0, 0)), [](double S, double Inset)
		{
			const double H = FMath::Lerp(0.055, 0.0425, S / 0.83) - Inset;
			return FurnitureKit::RoundRect(0.0, 0.0, H, H, 0.012, 0.012, 2, 1);
		}, PostEnds, {0.1, 0.4, 0.7});
		// A moulded collar at the post's head and its brackets up to the desk.
		Oak.Box(FVector(-0.075, -0.075, 0.88), FVector(0.075, 0.075, 0.92), FMeshData::AllFaces);
		for (int32 s = -1; s <= 1; s += 2)
		{
			Bar(FVector(0.0, s * 0.05, 0.90), FVector(-0.22, s * 0.24, D.At(0.05, 0, -kDeskBoard).Z), 0.02, 0.022, FVector(0, 0, 1));
			Bar(FVector(0.0, s * 0.05, 0.90), FVector(0.20, s * 0.24, D.At(0.52, 0, -kDeskBoard).Z), 0.02, 0.022, FVector(0, 0, 1));
		}
		// The desk: a board, its fascia round it, the ledge the book stands on.
		TArray<FStation> Desk = {{D.At(0.0, 0.0, -kDeskBoard * 0.5), D.V, D.N}, {D.At(kDeskDepth, 0.0, -kDeskBoard * 0.5), D.V, D.N}};
		SweepStations(Oak, Desk, RectSection(-kDeskWidth * 0.5, kDeskWidth * 0.5, -kDeskBoard * 0.5, kDeskBoard * 0.5));
		TArray<FStation> Fascia = {{D.At(0.02, 0.0, -kDeskBoard - 0.04), D.V, D.N}, {D.At(kDeskDepth - 0.02, 0.0, -kDeskBoard - 0.04), D.V, D.N}};
		SweepStations(Oak, Fascia, RectSection(-kDeskWidth * 0.5 + 0.02, kDeskWidth * 0.5 - 0.02, -0.04, 0.04));
		TArray<FStation> Ledge = {{D.At(0.0, -kDeskWidth * 0.5, 0.02), D.U, D.N}, {D.At(0.0, kDeskWidth * 0.5, 0.02), D.U, D.N}};
		SweepStations(Oak, Ledge, RectSection(0.0, 0.03, -0.02, 0.02));
	}
	// The book's boards (blue-grey paper over board), the holland spine between them, and the two stacks' edges.
	{
		const double U0 = kLip, U1 = kLip + kPageH + 2.0 * kSquare, W = kPageW + kSquare;
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector A = D.At(U0, s * 0.006, 0.0), B = D.At(U1, s * 0.006, 0.0);
			TArray<FStation> St = {{A, D.V, D.N}, {B, D.V, D.N}};
			SweepStations(Boards, St, RectSection(s < 0 ? -W : 0.0, s < 0 ? 0.0 : W, 0.0, kBoard));
		}
		TArray<FStation> Spine = {{D.At(U0, 0.0, 0.0), D.V, D.N}, {D.At(U1, 0.0, 0.0), D.V, D.N}};
		SweepStations(Cloth, Spine, RectSection(-0.03, 0.03, -0.0015, kBoard - 0.0005));
		// The stacks: their fore-edge, head and tail faces, and their tops under the pages.
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double H = s < 0 ? LeftH(Opening) : RightH(Opening);
			const double Ua = kLip + kSquare, Ub = kLip + kSquare + kPageH;
			TArray<FVector> Top;
			TArray<FVector2D> UV;
			for (int32 i = 0; i <= NU; ++i)
			{
				for (int32 j = 0; j <= NS; ++j)
				{
					const double Sd = kPageW * j / NS;
					Top.Add(D.At(FMath::Lerp(Ua, Ub, double(i) / NU), s * Sd, StackTop(H, Sd)));
					UV.Add(FVector2D(Sd, double(i) / NU * kPageH));
				}
			}
			Grid(Edges, Top, UV, NS, [&D](int32, int32) { return D.N; });
			// Fore-edge.
			Edges.Rect(D.At(Ua, s * kPageW, kBoard), D.At(Ub, s * kPageW, kBoard), D.At(Ub, s * kPageW, StackTop(H, kPageW)),
					   D.At(Ua, s * kPageW, StackTop(H, kPageW)), D.V * s);
			// Head and tail: the stack's profile, closed down to the board.
			for (const double Uu : {Ua, Ub})
			{
				const FVector Facing = Uu == Ua ? -D.U : D.U;
				for (int32 j = 0; j < NS; ++j)
				{
					const double S0 = kPageW * j / NS, S1 = kPageW * (j + 1) / NS;
					Edges.Poly({D.At(Uu, s * S0, kBoard), D.At(Uu, s * S1, kBoard), D.At(Uu, s * S1, StackTop(H, S1)), D.At(Uu, s * S0, StackTop(H, S0))}, Facing);
				}
			}
		}
	}
	Oak.Write(Lectern, 0, true);
	Boards.Write(Block, 0, true);
	Cloth.Write(Block, 1, true);
	Edges.Write(Block, 2, true);
	auto Apply = [](UProceduralMeshComponent* C, int32 Section, const TSoftObjectPtr<UMaterialInterface>& Ref)
	{
		if (UMaterialInterface* M = Ref.LoadSynchronous()) { C->SetMaterial(Section, M); }
	};
	Apply(Lectern, 0, OakMaterial);
	Apply(Block, 0, BoardMaterial);
	Apply(Block, 1, ClothMaterial);
	Apply(Block, 2, EdgeMaterial);
}

void AAlbionChaucer::LoadPages()
{
	PageTextures.Reset();
	for (int32 i = 0; i < 1000; ++i)
	{
		const FString Name = FString::Printf(TEXT("T_chaucer_%03d"), i);
		UTexture2D* T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), *PageFolder, *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!T) { break; }
		PageTextures.Add(T);
	}
	UE_LOG(LogMusee, Log, TEXT("Albion: the Kelmscott Chaucer has %d pages to turn."), PageTextures.Num());
	Opening = FMath::Clamp(Opening, 0, FMath::Max(0, PageTextures.Num() / 2 - 1));
}

UTexture2D* AAlbionChaucer::PageTexture(int32 Index) const
{
	return PageTextures.IsValidIndex(Index) ? PageTextures[Index].Get() : nullptr;
}

void AAlbionChaucer::BuildPages()
{
	using namespace AlbionChaucerImpl;
	const FDesk D;
	FMeshData Left, Right, Front, Back;
	const bool bForward = TurnDirection > 0;
	const int32 N = Opening;
	// While a leaf turns, the stacks show what lies under it.
	RestingPage(Left, D, -1, LeftH(N));
	RestingPage(Right, D, 1, RightH(N));
	if (TurnDirection != 0)
	{
		const double HFrom = bForward ? RightH(N) : LeftH(N), HTo = bForward ? LeftH(N) : RightH(N);
		TurningLeaf(Front, D, TurnT, bForward, HFrom, HTo, true);
		TurningLeaf(Back, D, TurnT, bForward, HFrom, HTo, false);
	}
	Pages->ClearAllMeshSections();
	Left.Write(Pages, 0, true);
	Right.Write(Pages, 1, true);
	Front.Write(Pages, 2, false);
	Back.Write(Pages, 3, false);
	ApplyPageTextures();
}

void AAlbionChaucer::ApplyPageTextures()
{
	UMaterialInterface* Base = PageMaterial.LoadSynchronous();
	if (!Base) { return; }
	while (PageMIDs.Num() < 4)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
		PageMIDs.Add(MID);
	}
	const int32 N = Opening;
	int32 Show[4] = {2 * N, 2 * N + 1, -1, -1};
	if (TurnDirection > 0) { Show[1] = 2 * N + 3; Show[2] = 2 * N + 1; Show[3] = 2 * N + 2; }
	if (TurnDirection < 0) { Show[0] = 2 * N - 2; Show[2] = 2 * N; Show[3] = 2 * N - 1; }
	for (int32 s = 0; s < 4; ++s)
	{
		if (UTexture2D* T = PageTexture(Show[s])) { PageMIDs[s]->SetTextureParameterValue(TEXT("Page"), T); }
		Pages->SetMaterial(s, PageMIDs[s]);
	}
}

bool AAlbionChaucer::TurnPage(int32 Direction)
{
	if (TurnDirection != 0 || Direction == 0) { return false; }
	const int32 Openings = PageTextures.Num() / 2;
	if (Direction > 0 && Opening + 1 >= Openings) { return false; }
	if (Direction < 0 && Opening <= 0) { return false; }
	TurnDirection = Direction > 0 ? 1 : -1;
	TurnT = 0.f;
	BuildPages();
	return true;
}

void AAlbionChaucer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (TurnDirection == 0) { return; }
	TurnT = FMath::Min(1.f, TurnT + DeltaSeconds / FMath::Max(TurnSeconds, 0.1f));
	if (TurnT >= 1.f)
	{
		Opening += TurnDirection;
		TurnDirection = 0;
		TurnT = 0.f;
		// The stacks' edges change height with the leaves read.
		BuildStatic();
	}
	BuildPages();
}

bool AAlbionChaucer::Near(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	return FVector::Dist2D(Visitor->FeetLocation(), GetActorLocation()) < PromptDistance * 100.0;
}

bool AAlbionChaucer::CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const
{
	return TurnDirection == 0 && PageTextures.Num() > 1 && Hit.GetComponent() == Pages.Get();
}

void AAlbionChaucer::Interact(AMuseeCharacter* Visitor, const FHitResult& Hit)
{
	// The right-hand page (the reader's right, +Y) turns forward; the left back.
	const FVector Local = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
	TurnPage(Local.Y >= 0.0 ? 1 : -1);
}

FText AAlbionChaucer::InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
	if (Local.Y >= 0.0) { return Opening + 1 < PageTextures.Num() / 2 ? LOCTEXT("Next", "Turn the page") : FText::GetEmpty(); }
	return Opening > 0 ? LOCTEXT("Prev", "Turn back") : FText::GetEmpty();
}

void AAlbionChaucer::GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const
{
	if (!Near(Visitor) || TurnDirection != 0 || PageTextures.Num() < 2) { return; }
	if (Opening + 1 < PageTextures.Num() / 2) { Out.Add({TEXT("chaucer.next"), LOCTEXT("PromptNext", "Turn the page")}); }
	if (Opening > 0) { Out.Add({TEXT("chaucer.prev"), LOCTEXT("PromptPrev", "Turn back")}); }
}

void AAlbionChaucer::RunPrompt(AMuseeCharacter* Visitor, FName Id)
{
	if (Id == TEXT("chaucer.next")) { TurnPage(1); }
	else if (Id == TEXT("chaucer.prev")) { TurnPage(-1); }
}

#undef LOCTEXT_NAMESPACE
