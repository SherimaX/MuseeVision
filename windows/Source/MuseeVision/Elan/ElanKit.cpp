#include "Elan/ElanKit.h"

#include "Elan/ElanStructure.h"
#include "Components/BoxComponent.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/SoftObjectPtr.h"

namespace ElanGlassKit
{
	const TCHAR* const GlassPath = TEXT("/Game/Museum/Materials/M_Glass.M_Glass");
	const TCHAR* const GiltPath = TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt");
	const TCHAR* const MetalPath = TEXT("/Game/Museum/Materials/M_Metal.M_Metal");
	const TCHAR* const PearlPath = TEXT("/Game/Museum/Materials/M_RibPearl.M_RibPearl");
	const TCHAR* const TravertinePath = TEXT("/Game/Museum/Materials/M_Elan_Mirror.M_Elan_Mirror");   // the Future's floor (materials.py)
	const TCHAR* const DaylitPath = TEXT("/Game/Museum/Materials/M_Daylit.M_Daylit");
	const TCHAR* const IrisGlowPath = TEXT("/Game/Museum/Materials/M_IrisGlow.M_IrisGlow");
	const TCHAR* const WhitePath = TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture");

	double CollarTop() { return E::RingHeight() - AElanStructure::RingDepth - 0.03; }

	const TArray<FVector2D>& DoorSpans()
	{
		static const TArray<FVector2D> Spans = {
			FVector2D(24, 48), FVector2D(48, 136), FVector2D(136, 224), FVector2D(224, 312), FVector2D(312, 336)};
		return Spans;
	}

	const TArray<FVector2D>& CarSpans()
	{
		static const TArray<FVector2D> Spans = {FVector2D(24, 92), FVector2D(92, 180), FVector2D(180, 268), FVector2D(268, 336)};
		return Spans;
	}

	const TArray<FVector2D>& TubeSpans()
	{
		static const TArray<FVector2D> Spans = {
			FVector2D(-24, 24), FVector2D(24, 48), FVector2D(48, 136), FVector2D(136, 224), FVector2D(224, 312), FVector2D(312, 336)};
		return Spans;
	}

	// Geometry.

	FVector Polar(double R, double A, double Z) { return FVector(R * FMath::Cos(A), R * FMath::Sin(A), Z); }
	FVector Outward(double A) { return FVector(FMath::Cos(A), FMath::Sin(A), 0); }
	FVector Along(double A) { return FVector(-FMath::Sin(A), FMath::Cos(A), 0); }
	int32 Segments(double A0, double A1) { return FMath::Max(2, FMath::CeilToInt32(Round * FMath::Abs(A1 - A0) / Turn)); }

	void Pane(FMuseeMesh& M, double R, double A0, double A1, double Z0, double Z1)
	{
		const int32 N = Segments(A0, A1);
		for (int32 i = 0; i < N; ++i)
		{
			// The end angles exactly: a neighbouring pane starts on the very same vertices.
			const double T0 = i == 0 ? A0 : A0 + (A1 - A0) * i / N, T1 = i + 1 == N ? A1 : A0 + (A1 - A0) * (i + 1) / N;
			M.Quad(Polar(R, T0, Z0), Polar(R, T1, Z0), Polar(R, T1, Z1), Polar(R, T0, Z1),
				   Outward(T0), Outward(T1), Outward(T1), Outward(T0),
				   FVector2D(R * T0, Z0), FVector2D(R * T1, Z0), FVector2D(R * T1, Z1), FVector2D(R * T0, Z1));
		}
	}

	void Flat(FMuseeMesh& M, double R0, double R1, double Z, const FVector& N)
	{
		for (int32 i = 0; i < Round; ++i)
		{
			const double T0 = Turn * i / Round, T1 = Turn * (i + 1) / Round;
			if (R0 <= 0) { M.Tri(FVector(0, 0, Z), Polar(R1, T0, Z), Polar(R1, T1, Z), N, N, N); }
			else { M.Quad(Polar(R0, T0, Z), Polar(R0, T1, Z), Polar(R1, T1, Z), Polar(R1, T0, Z), N); }
		}
	}

	void Band(FMuseeMesh& M, double R0, double R1, double Z0, double Z1, double A0, double A1, bool bCaps)
	{
		const FVector Up(0, 0, 1);
		const int32 N = Segments(A0, A1);
		for (int32 i = 0; i < N; ++i)
		{
			const double T0 = A0 + (A1 - A0) * i / N, T1 = A0 + (A1 - A0) * (i + 1) / N;
			M.Quad(Polar(R1, T0, Z0), Polar(R1, T1, Z0), Polar(R1, T1, Z1), Polar(R1, T0, Z1), Outward(T0), Outward(T1), Outward(T1), Outward(T0));
			if (R0 > 0)
			{
				M.Quad(Polar(R0, T0, Z0), Polar(R0, T1, Z0), Polar(R0, T1, Z1), Polar(R0, T0, Z1), -Outward(T0), -Outward(T1), -Outward(T1), -Outward(T0));
				M.Quad(Polar(R0, T0, Z1), Polar(R0, T1, Z1), Polar(R1, T1, Z1), Polar(R1, T0, Z1), Up);
				M.Quad(Polar(R0, T0, Z0), Polar(R0, T1, Z0), Polar(R1, T1, Z0), Polar(R1, T0, Z0), -Up);
			}
			else
			{
				M.Tri(FVector(0, 0, Z1), Polar(R1, T0, Z1), Polar(R1, T1, Z1), Up, Up, Up);
				M.Tri(FVector(0, 0, Z0), Polar(R1, T0, Z0), Polar(R1, T1, Z0), -Up, -Up, -Up);
			}
		}
		if (bCaps)
		{
			M.Quad(Polar(R0, A0, Z0), Polar(R1, A0, Z0), Polar(R1, A0, Z1), Polar(R0, A0, Z1), -Along(A0));
			M.Quad(Polar(R0, A1, Z0), Polar(R1, A1, Z0), Polar(R1, A1, Z1), Polar(R0, A1, Z1), Along(A1));
		}
	}

	void Tube(FMuseeMesh& M, double Major, double Minor, double Zc, double A0, double A1)
	{
		constexpr int32 Around = 16;
		const FVector Up(0, 0, 1);
		auto Dir = [&Up](double T, double P) { return Outward(T) * FMath::Cos(P) + Up * FMath::Sin(P); };
		const int32 N = Segments(A0, A1);
		for (int32 i = 0; i < N; ++i)
		{
			const double T0 = A0 + (A1 - A0) * i / N, T1 = A0 + (A1 - A0) * (i + 1) / N;
			for (int32 j = 0; j < Around; ++j)
			{
				const double P0 = Turn * j / Around, P1 = Turn * (j + 1) / Around;
				M.Quad(Polar(Major, T0, Zc) + Dir(T0, P0) * Minor, Polar(Major, T1, Zc) + Dir(T1, P0) * Minor,
					   Polar(Major, T1, Zc) + Dir(T1, P1) * Minor, Polar(Major, T0, Zc) + Dir(T0, P1) * Minor,
					   Dir(T0, P0), Dir(T1, P0), Dir(T1, P1), Dir(T0, P1));
			}
		}
	}

	void Ball(FMuseeMesh& M, const FVector& C, double R)
	{
		constexpr int32 Lon = 16, Lat = 8;
		auto Dir = [](int32 i, int32 j)
		{
			const double La = -UE_DOUBLE_PI / 2 + UE_DOUBLE_PI * i / Lat, Lo = Turn * j / Lon;
			return FVector(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
		};
		for (int32 i = 0; i < Lat; ++i)
		{
			for (int32 j = 0; j < Lon; ++j)
			{
				const FVector D00 = Dir(i, j), D01 = Dir(i, j + 1), D11 = Dir(i + 1, j + 1), D10 = Dir(i + 1, j);
				M.Quad(C + D00 * R, C + D01 * R, C + D11 * R, C + D10 * R, D00, D01, D11, D10);
			}
		}
	}

	void Block(FMuseeMesh& M, const FVector& C, const FVector& X, const FVector& Y, const FVector& Z, const FVector& Half)
	{
		const FVector Axes[3] = {X * Half.X, Y * Half.Y, Z * Half.Z};
		for (int32 k = 0; k < 3; ++k)
		{
			const FVector& N = Axes[k];
			const FVector& U = Axes[(k + 1) % 3];
			const FVector& V = Axes[(k + 2) % 3];
			for (const double S : {-1.0, 1.0})
			{
				const FVector F = C + N * S;
				M.Quad(F - U - V, F + U - V, F + U + V, F - U + V, N.GetSafeNormal() * S);
			}
		}
	}

	void Rod(FMuseeMesh& M, const FVector& A, const FVector& B, double R)
	{
		constexpr int32 Around = 12;
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector U = FVector::CrossProduct(Axis, FMath::Abs(Axis.Z) > 0.9 ? FVector(1, 0, 0) : FVector(0, 0, 1)).GetSafeNormal();
		const FVector V = FVector::CrossProduct(Axis, U);
		for (int32 j = 0; j < Around; ++j)
		{
			const double P0 = Turn * j / Around, P1 = Turn * (j + 1) / Around;
			const FVector N0 = U * FMath::Cos(P0) + V * FMath::Sin(P0), N1 = U * FMath::Cos(P1) + V * FMath::Sin(P1);
			M.Quad(A + N0 * R, A + N1 * R, B + N1 * R, B + N0 * R, N0, N1, N1, N0);
			M.Tri(A, A + N1 * R, A + N0 * R, -Axis, -Axis, -Axis);
			M.Tri(B, B + N0 * R, B + N1 * R, Axis, Axis, Axis);
		}
	}

	void PolygonRing(FMuseeMesh& Top, FMuseeMesh& Sides, int32 N, double RIn, double ROut, double Z0, double Z1)
	{
		const FVector Up(0, 0, 1);
		constexpr int32 Sub = 12;   // steps along each side of the polygon, so the outer circle stays round
		for (int32 k = 0; k < N; ++k)
		{
			const double A0 = Turn * k / N - UE_DOUBLE_PI / N, A1 = Turn * (k + 1) / N - UE_DOUBLE_PI / N;
			const FVector C0 = Polar(RIn, A0, 0), C1 = Polar(RIn, A1, 0);
			const FVector Inward = -Outward((A0 + A1) / 2);
			for (int32 s = 0; s < Sub; ++s)
			{
				const double F0 = double(s) / Sub, F1 = double(s + 1) / Sub;
				const FVector I0 = FMath::Lerp(C0, C1, F0), I1 = FMath::Lerp(C0, C1, F1);
				const double T0 = A0 + (A1 - A0) * F0, T1 = A0 + (A1 - A0) * F1;
				const FVector O0 = Polar(ROut, T0, 0), O1 = Polar(ROut, T1, 0);
				const FVector Z1v(0, 0, Z1), Z0v(0, 0, Z0);
				Top.Quad(I0 + Z1v, I1 + Z1v, O1 + Z1v, O0 + Z1v, Up);
				Sides.Quad(I0 + Z0v, I1 + Z0v, O1 + Z0v, O0 + Z0v, -Up);
				Sides.Quad(O0 + Z0v, O1 + Z0v, O1 + Z1v, O0 + Z1v, Outward(T0), Outward(T1), Outward(T1), Outward(T0));
			}
			Sides.Quad(C0 + FVector(0, 0, Z0), C1 + FVector(0, 0, Z0), C1 + FVector(0, 0, Z1), C0 + FVector(0, 0, Z1), Inward);
		}
	}

	// Materials.

	UMaterialInterface* LoadMaterial(const TCHAR* Path)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)).LoadSynchronous();
	}

	UMaterialInstanceDynamic* GlassInstance(UObject* Outer, float Opacity, float Roughness, float Specular)
	{
		UMaterialInterface* Base = LoadMaterial(GlassPath);
		if (!Base) { return nullptr; }
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
		// The faintest pale-teal sheen (a twentieth of a diffuse surface, through its opacity): enough for the
		// glass to read against a dark sky or a dark wall, far too little to sparkle.
		MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.72f, 0.86f, 0.84f));
		MID->SetScalarParameterValue(TEXT("Diffuse"), 0.05f);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		MID->SetScalarParameterValue(TEXT("Specular"), Specular);
		// Real glass (M_Glass): towards grazing it mirrors and closes (the curved panes' flanks read as a
		// sheet of glass, not a hole), and a faint haze of use on it.
		MID->SetScalarParameterValue(TEXT("GrazingOpacity"), FMath::Max(Opacity, 0.62f));
		MID->SetScalarParameterValue(TEXT("Smudge"), 0.4f);
		return MID;
	}

	UMaterialInstanceDynamic* GlowInstance(UObject* Outer, const FLinearColor& Tint, float Nits)
	{
		if (UMaterialInterface* Daylit = LoadMaterial(DaylitPath))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Daylit, Outer);
			if (UTexture* White = TSoftObjectPtr<UTexture>(FSoftObjectPath(WhitePath)).LoadSynchronous())
			{
				MID->SetTextureParameterValue(TEXT("Image"), White);
			}
			MID->SetVectorParameterValue(TEXT("Tint"), Tint);
			MID->SetScalarParameterValue(TEXT("NightFloor"), 1.f);
			MID->SetScalarParameterValue(TEXT("Luminance"), Nits);
			return MID;
		}
		if (UMaterialInterface* Iris = LoadMaterial(IrisGlowPath))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Iris, Outer);
			MID->SetScalarParameterValue(TEXT("Luminance"), Nits);
			return MID;
		}
		return nullptr;
	}

	UMaterialInstanceDynamic* MetalInstance(UObject* Outer, const FLinearColor& Colour, float Metallic, float Roughness)
	{
		UMaterialInterface* Base = LoadMaterial(MetalPath);
		if (!Base) { return nullptr; }
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Colour);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		MID->SetScalarParameterValue(TEXT("Variation"), 0.03f);
		MID->SetScalarParameterValue(TEXT("PatinaAmount"), 0.f);
		return MID;
	}

	UMaterialInstanceDynamic* EdgeInstance(UObject* Outer)
	{
		// Low-iron glass seen through its thickness: a deep sea green, with a clear gloss.
		return MetalInstance(Outer, FLinearColor(0.035f, 0.115f, 0.095f), 0.f, 0.06f);
	}

	// Components.

	void Register(UActorComponent* Part)
	{
		Part->RegisterComponent();
		if (AActor* Actor = Part->GetOwner()) { Actor->AddInstanceComponent(Part); }
	}

	UProceduralMeshComponent* NewMesh(USceneComponent* Parent, const TCHAR* Name, bool bShadow)
	{
		UProceduralMeshComponent* PM = NewPart<UProceduralMeshComponent>(Parent, Name);
		PM->bUseAsyncCooking = false;
		PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PM->SetCanEverAffectNavigation(false);
		PM->SetCastShadow(bShadow);
		return PM;
	}

	void Section(UProceduralMeshComponent* PM, int32 Index, const FMuseeMesh& Geometry, UMaterialInterface* Material)
	{
		if (Geometry.Vertices.Num() == 0) { return; }
		Geometry.Write(PM, Index);
		PM->SetMaterial(Index, Material);
	}

	void NoRayTracing(UPrimitiveComponent* Part)
	{
		Part->SetVisibleInRayTracing(false);
		Part->bVisibleInReflectionCaptures = false;
	}

	void Solid(UPrimitiveComponent* Part, bool bWalkable)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->CanCharacterStepUpOn = bWalkable ? ECB_Yes : ECB_No;
	}

	void Wall(USceneComponent* Parent, const TCHAR* Name, double R, double A0, double A1, double Z0, double Z1, TArray<TObjectPtr<UBoxComponent>>& Out)
	{
		const int32 N = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(A1 - A0) / (BlockerStepDeg * Deg)));
		for (int32 i = 0; i < N; ++i)
		{
			const double T0 = A0 + (A1 - A0) * i / N, T1 = A0 + (A1 - A0) * (i + 1) / N;
			const double Mid = (T0 + T1) / 2, Half = FMath::Abs(T1 - T0) / 2;
			UBoxComponent* Box = NewPart<UBoxComponent>(Parent, Name);
			Box->InitBoxExtent(FVector(BlockerThick / 2, R * FMath::Tan(Half) + 0.01, (Z1 - Z0) / 2) * Cm);
			Box->SetRelativeLocationAndRotation(Polar(R, Mid, (Z0 + Z1) / 2) * Cm, FRotator(0, FMath::RadiansToDegrees(Mid), 0));
			Box->SetHiddenInGame(true);
			Solid(Box, false);
			Register(Box);
			Out.Add(Box);
		}
	}

	void EdgeStrip(FMuseeMesh& M, double R, double A, double Z0, double Z1)
	{
		Block(M, Polar(R, A, (Z0 + Z1) / 2), Outward(A), Along(A), FVector(0, 0, 1), FVector(EdgeDepth / 2, EdgeWidth / 2, (Z1 - Z0) / 2));
	}

	void FixedGlass(USceneComponent* Parent, const TCHAR* Name, double R, double Z0, double Z1, const TArray<FVector2D>& SpansDeg,
					const FLook& Look, TArray<TObjectPtr<UPrimitiveComponent>>& Out)
	{
		TArray<double> Joints;
		for (const FVector2D& Span : SpansDeg)
		{
			const double A0 = DoorTheta + Span.X * Deg + Joint / R, A1 = DoorTheta + Span.Y * Deg - Joint / R;
			FMuseeMesh PaneGeo;
			Pane(PaneGeo, R, A0, A1, Z0, Z1);
			UProceduralMeshComponent* PM = NewMesh(Parent, Name, false);
			Section(PM, 0, PaneGeo, Look.Clear);
			Register(PM);
			Out.Add(PM);
			for (const double J : {Span.X, Span.Y}) { Joints.AddUnique(FMath::Fmod(J + 360.0, 360.0)); }
		}
		// The joints and the jambs: the glass's green edges, opaque, so they read steadily at any angle.
		if (!Look.Edge) { return; }
		FMuseeMesh EdgeGeo;
		for (const double J : Joints) { EdgeStrip(EdgeGeo, R, DoorTheta + J * Deg, Z0, Z1); }
		UProceduralMeshComponent* Edges = NewMesh(Parent, TEXT("GlassEdges"), false);
		Section(Edges, 0, EdgeGeo, Look.Edge);
		Register(Edges);
	}

	/**
	 * One door leaf on its own pivot at the axis. Closed, it spans the doorway from the centre line to
	 * one side (Side −1 or +1) and a degree beyond, behind the pocket pane; opening turns the pivot, so
	 * the leaf slides round the circle into its pocket. Its bronze leading edge, green trailing edge and
	 * collision boxes turn with it.
	 */
	USceneComponent* Leaf(USceneComponent* Parent, const TCHAR* Name, double R, double Z0, double Z1, double Side, const FLook& Look,
						  TArray<TObjectPtr<UPrimitiveComponent>>& Glass, TArray<TObjectPtr<UBoxComponent>>& Boxes)
	{
		USceneComponent* Pivot = NewPart<USceneComponent>(Parent, TEXT("LeafPivot"));
		Register(Pivot);
		const double Lead = DoorTheta + Side * 0.001 / R;   // 1 mm short of the centre line: the two meet on a hairline
		const double Trail = DoorTheta + Side * LeafDeg * Deg;
		const double FinAngle = Lead + Side * (FinWidth / 2) / R;
		FMuseeMesh PaneGeo, FinGeo, EdgeGeo;
		Pane(PaneGeo, R, Lead + Side * FinWidth / R, Trail, Z0, Z1);
		Block(FinGeo, Polar(R, FinAngle, (Z0 + Z1) / 2), Outward(FinAngle), Along(FinAngle), FVector(0, 0, 1),
			  FVector(FinDepth / 2, FinWidth / 2, (Z1 - Z0) / 2));
		EdgeStrip(EdgeGeo, R, Trail - Side * (EdgeWidth / 2) / R, Z0, Z1);
		UProceduralMeshComponent* PM = NewMesh(Pivot, Name, false);
		Section(PM, 0, PaneGeo, Look.Clear);
		Section(PM, 1, FinGeo, Look.Bronze);
		Section(PM, 2, EdgeGeo, Look.Edge);
		Register(PM);
		Glass.Add(PM);
		Wall(Pivot, TEXT("LeafWall"), R, DoorTheta, Trail, Z0 - 0.02, Z1, Boxes);
		return Pivot;
	}

	void TurnLeaves(const TArray<TObjectPtr<USceneComponent>>& Pivots, float Open)
	{
		for (int32 i = 0; i < Pivots.Num(); ++i)
		{
			const double Side = i % 2 == 0 ? -1.0 : 1.0;   // built side −1 first
			if (USceneComponent* Pivot = Pivots[i]) { Pivot->SetRelativeRotation(FRotator(0, Side * Open * DoorDeg, 0)); }
		}
	}
}
