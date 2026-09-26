#include "Chenghuai/ChenghuaiScroll.h"

#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/** The scroll's mesh building, in a named namespace (unity builds). */
namespace ChenghuaiScrollImpl
{
	using SalonKit::FMeshData;

	enum ESection : int32 { SecMount = 0, SecBand = 1, SecWood = 2, SecKnob = 3, SecImage = 4 };

	/** The silk's cockle: a gentle ripple across the scroll (mm), held flat at its ends. */
	struct FRipple
	{
		double Amp = 0.0012;
		double Z0 = 0.0, Z1 = 1.0;  // where it fades out (hanging: stick and roller)
		double Y0 = 0.0, Y1 = 1.0;  // (handscroll: its two ends)
		bool bAlongZ = true;

		double At(double Y, double Z) const
		{
			const double Fz = bAlongZ ? FMath::Clamp(FMath::Min(Z - Z0, Z1 - Z) / 0.06, 0.0, 1.0) : 1.0;
			const double Fy = bAlongZ ? 1.0 : FMath::Clamp(FMath::Min(Y - Y0, Y1 - Y) / 0.05, 0.0, 1.0);
			const double W = bAlongZ ? FMath::Sin(Z * 27.0 + 0.8 * FMath::Sin(Y * 3.1)) * 0.7 + FMath::Sin(Z * 11.0 + 1.3) * 0.3
									 : FMath::Sin(Y * 23.0 + 0.9 * FMath::Sin(Z * 13.0)) * 0.6 + FMath::Sin(Y * 7.3 + 0.4) * 0.4;
			return Amp * W * Fz * Fy;
		}
	};

	/**
	 * One flat field of the scroll's face (YZ, facing +X) with the ripple, as a grid. UV: either the field's own share of an
	 * image (U0..U1 across, V0..V1 down) or metres (Y, −Z) for the silk.
	 */
	void Field(FMeshData& M, const FRipple& R, double Y0, double Y1, double Z0, double Z1, double X, bool bFront, bool bImageUV,
			   const FVector2D& UV0 = FVector2D(0, 0), const FVector2D& UV1 = FVector2D(1, 1))
	{
		if (Y1 - Y0 < 1e-5 || Z1 - Z0 < 1e-5) { return; }
		const int32 NY = FMath::Clamp(FMath::CeilToInt32((Y1 - Y0) / 0.04), 1, 400);
		const int32 NZ = FMath::Clamp(FMath::CeilToInt32((Z1 - Z0) / 0.02), 1, 200);
		const double Sign = bFront ? 1.0 : -1.0;
		M.Patch(NY, NZ,
				[&](int32 i, int32 j)
				{
					const double Y = FMath::Lerp(Y0, Y1, double(i) / NY), Z = FMath::Lerp(Z1, Z0, double(j) / NZ);
					return FVector(X + R.At(Y, Z), -Y, Z);
				},
				[&](const FVector& P)
				{
					if (!bImageUV) { return FVector2D(-P.Y, -P.Z); }
					const double U = (-P.Y - Y0) / (Y1 - Y0), V = (Z1 - P.Z) / (Z1 - Z0);
					return FVector2D(FMath::Lerp(UV0.X, UV1.X, U), FMath::Lerp(UV0.Y, UV1.Y, V));
				},
				[&](const FVector&) { return FVector(Sign, 0, 0); });
	}

	/** The viewer's frame (x out of the face, y to the viewer's right, z up) to the actor's (Unreal's frame is left-handed,
	 * so seen from +X the actor's −Y is on the right). */
	FVector V(const FVector& P) { return FVector(P.X, -P.Y, P.Z); }

	/** A cylinder along an axis (closed with flat ends), UV U around, V along (metres); ends in the viewer's frame. */
	void Cylinder(FMeshData& M, const FVector& AV, const FVector& BV, double RA, double RB, int32 Segs, bool bCaps)
	{
		const FVector A = V(AV), B = V(BV);
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector S = FVector::CrossProduct(Axis, FMath::Abs(Axis.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
		const FVector T = FVector::CrossProduct(Axis, S);
		const double L = (B - A).Size();
		const int32 Base = M.Positions.Num();
		for (int32 k = 0; k <= Segs; ++k)
		{
			const double An = 2.0 * PI * k / Segs;
			const FVector D = S * FMath::Cos(An) + T * FMath::Sin(An);
			const FVector N = (D + Axis * (RA - RB) / FMath::Max(L, 1e-6)).GetSafeNormal();
			M.Vertex(A + D * RA, N, FVector2D(An * RA, 0));
			M.Vertex(B + D * RB, N, FVector2D(An * RA, L));
		}
		for (int32 k = 0; k < Segs; ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 2, Base + 2 * k + 3, Base + 2 * k + 1); }
		if (!bCaps) { return; }
		for (int32 e = 0; e < 2; ++e)
		{
			const FVector C = e ? B : A;
			const double Rr = e ? RB : RA;
			const FVector N = e ? Axis : -Axis;
			TArray<FVector> Pts;
			for (int32 k = 0; k < Segs; ++k)
			{
				const double An = 2.0 * PI * k / Segs;
				Pts.Add(C + (S * FMath::Cos(An) + T * FMath::Sin(An)) * Rr);
			}
			M.Poly(Pts, N);
		}
	}

	/** A small box between two corners (the viewer's frame). */
	void Block(FMeshData& M, const FVector& AV, const FVector& BV)
	{
		const FVector A = V(AV), B = V(BV);
		M.Box(A.ComponentMin(B), A.ComponentMax(B), FMeshData::AllFaces);
	}
}

AChenghuaiScroll::AChenghuaiScroll()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetCastShadow(true);
	Tags.AddUnique(MuseeBake::BakeableTag());
	Tags.AddUnique(FName(TEXT("musee.wing:Chenghuai")));
}

void AChenghuaiScroll::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AChenghuaiScroll::BeginPlay()
{
	Super::BeginPlay();
	if (Mesh && Mesh->GetNumSections() == 0) { Build(); }
}

void AChenghuaiScroll::Rebuild() { Build(); }

float AChenghuaiScroll::GetMountedLength() const
{
	if (Kind == EChenghuaiScrollKind::Hanging) { return Heaven + 2.f * Band + ImageHeight + Earth; }
	return ImageWidth + 3.f * Band + Lead + FMath::Max(0.f, Wrapper) + (Wrapper > 0.f ? 0.2f : 0.013f) + 2.f * RollRadius;
}

void AChenghuaiScroll::Build()
{
	using namespace ChenghuaiScrollImpl;
	if (MuseeBake::IsBaked(this) || !Mesh) { return; }
	Mesh->ClearAllMeshSections();

	const int32 Tiles = FMath::Max(1, ImageMaterials.Num());
	FMeshData Sec[4];
	TArray<FMeshData> Img;
	Img.SetNum(Tiles);
	FMeshData& Mount = Sec[SecMount];
	FMeshData& Bands = Sec[SecBand];
	FMeshData& Wood = Sec[SecWood];
	FMeshData& Knob = Sec[SecKnob];
	const double IW = ImageWidth, IH = ImageHeight, Bd = Band, Br = Border;

	if (Kind == EChenghuaiScrollKind::Hanging)
	{
		// Top down from the stick: 天头, 隔水, the painting between its borders, 隔水, 地头.
		const double W = IW + 2.0 * Br, H = Heaven + 2.0 * Bd + IH + Earth;
		const double Y0 = -W / 2, Y1 = W / 2, YI0 = -IW / 2, YI1 = IW / 2;
		const double ZA = -Heaven, ZB = ZA - Bd, ZC = ZB - IH, ZD = ZC - Bd, ZE = -H;
		FRipple R;
		R.Z0 = ZE;
		R.Z1 = 0.0;
		for (int32 Face = 0; Face < 2; ++Face)
		{
			const bool bF = Face == 0;
			const double X = bF ? 0.0 : -0.0012;
			if (!bF)
			{
				Field(Mount, R, Y0, Y1, ZE, 0.0, X, false, false);
				continue;
			}
			Field(Mount, R, Y0, Y1, ZA, 0.0, X, true, false);
			Field(Bands, R, Y0, Y1, ZB, ZA, X, true, false);
			Field(Bands, R, Y0, YI0, ZC, ZB, X, true, false);
			Field(Bands, R, YI1, Y1, ZC, ZB, X, true, false);
			for (int32 t = 0; t < Tiles; ++t)
			{
				const double TY0 = FMath::Lerp(YI0, YI1, double(t) / Tiles), TY1 = FMath::Lerp(YI0, YI1, double(t + 1) / Tiles);
				Field(Img[t], R, TY0, TY1, ZC, ZB, X, true, true);
			}
			Field(Bands, R, Y0, Y1, ZD, ZC, X, true, false);
			Field(Mount, R, Y0, Y1, ZE, ZD, X, true, false);
		}
		// The mount's edges (a sliver, so it isn't a card seen edge on).
		Block(Mount, FVector(-0.0012, Y0 - 0.0004, ZE), FVector(0.0, Y0, 0.0));
		Block(Mount, FVector(-0.0012, Y1, ZE), FVector(0.0, Y1 + 0.0004, 0.0));
		// 惊燕: two ribbons down the heaven, a quarter in from each side.
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double Yc = s * W / 4, Wr = FMath::Clamp(W * 0.028, 0.014, 0.024);
			Block(Bands, FVector(0.0008, Yc - Wr / 2, ZA + 0.004), FVector(0.0016, Yc + Wr / 2, -0.012));
		}
		// 天杆: the flat top stick under the silk; the cord from its rings to the hook.
		Block(Bands, FVector(-0.009, Y0 - 0.002, -0.016), FVector(0.003, Y1 + 0.002, 0.002));
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector Ring(-0.003, s * W * 0.3, 0.002);
			Cylinder(Knob, Ring + FVector(0, 0, 0.0), Ring + FVector(0, 0, 0.006), 0.004, 0.004, 8, true);
			Cylinder(Wood, Ring + FVector(0, 0, 0.006), FVector(-0.003, 0.0, 0.085), 0.0014, 0.0014, 6, false);
		}
		// 地杆: the roller the earth wraps, and its knobs (轴头) past the mount.
		const double RR = 0.0135;
		Cylinder(Mount, FVector(-0.0006, Y0, ZE + RR * 0.2), FVector(-0.0006, Y1, ZE + RR * 0.2), RR, RR, 20, false);
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double Ye = s > 0 ? Y1 : Y0;
			Cylinder(Wood, FVector(-0.0006, Ye, ZE + RR * 0.2), FVector(-0.0006, Ye + s * 0.004, ZE + RR * 0.2), RR * 0.9, RR * 0.9, 16, false);
			Cylinder(Knob, FVector(-0.0006, Ye + s * 0.004, ZE + RR * 0.2), FVector(-0.0006, Ye + s * 0.034, ZE + RR * 0.2), 0.0165, 0.0145, 20, true);
		}
	}
	else
	{
		// Right to left from the beginning: wrapper (包首) with its end stick, 隔水, frontispiece field (引首), 隔水, the work,
		// 隔水, then the rest rolled up round the roller. Borders run along the top and bottom of the whole length.
		const double Hh = IH / 2 + Br;
		const double YWrap0 = Bd + Lead + Bd, YWrap1 = YWrap0 + FMath::Max(0.0, double(Wrapper));
		const double YEnd = -IW - Bd;
		FRipple R;
		R.bAlongZ = false;
		R.Amp = 0.0007;
		R.Y0 = YEnd;
		R.Y1 = YWrap1;
		const double X = 0.0;
		Field(Mount, R, YEnd, YWrap1, -Hh, Hh, -0.0012, false, false);
		Field(Bands, R, YWrap0, YWrap1, -Hh, Hh, X, true, false);
		Field(Bands, R, Bd + Lead, YWrap0, -IH / 2, IH / 2, X, true, false);
		Field(Mount, R, Bd, Bd + Lead, -IH / 2, IH / 2, X, true, false);
		Field(Bands, R, 0.0, Bd, -IH / 2, IH / 2, X, true, false);
		for (int32 t = 0; t < Tiles; ++t)
		{
			const double TY0 = FMath::Lerp(-IW, 0.0, double(t) / Tiles), TY1 = FMath::Lerp(-IW, 0.0, double(t + 1) / Tiles);
			Field(Img[t], R, TY0, TY1, -IH / 2, IH / 2, X, true, true);
		}
		Field(Bands, R, YEnd, -IW, -IH / 2, IH / 2, X, true, false);
		Field(Bands, R, YEnd, YWrap0, IH / 2, Hh, X, true, false);
		Field(Bands, R, YEnd, YWrap0, -Hh, -IH / 2, X, true, false);
		// The end stick (天杆) with its ribbon and jade clasp when the wrapper lies open; else the wrapper rolled round it.
		if (Wrapper > 0.f)
		{
			Block(Wood, FVector(-0.0012, YWrap1, -Hh - 0.001), FVector(0.005, YWrap1 + 0.012, Hh + 0.001));
			Block(Bands, FVector(0.0, YWrap1 + 0.004, -0.009), FVector(0.0022, YWrap1 + 0.16, 0.009));
			Cylinder(Knob, FVector(0.003, YWrap1 + 0.16, 0.0), FVector(0.003, YWrap1 + 0.19, 0.0), 0.004, 0.004, 10, true);
		}
		else
		{
			const double RW = 0.013;
			Cylinder(Bands, FVector(RW - 0.0012, YWrap1, -Hh), FVector(RW - 0.0012, YWrap1, Hh), RW, RW, 20, true);
		}
		const double RR = RollRadius;
		const FVector RollA(RR - 0.0012, YEnd - 0.0, -Hh), RollB(RR - 0.0012, YEnd, Hh);
		Cylinder(Mount, RollA, RollB, RR, RR, 28, true);
		Cylinder(Knob, RollA, RollA - FVector(0, 0, 0.018), 0.011, 0.0095, 16, true);
		Cylinder(Knob, RollB, RollB + FVector(0, 0, 0.018), 0.011, 0.0095, 16, true);
	}

	for (int32 s = 0; s < 4; ++s) { Sec[s].Write(Mesh, s, s <= SecBand); }
	for (int32 t = 0; t < Tiles; ++t) { Img[t].Write(Mesh, SecImage + t, true); }

	auto Load = [](const TSoftObjectPtr<UMaterialInterface>& P, const TCHAR* Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* M = P.IsNull() ? nullptr : P.LoadSynchronous()) { return M; }
		return LoadObject<UMaterialInterface>(nullptr, Fallback);
	};
	const TCHAR* Default = TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial");
	Mesh->SetMaterial(SecMount, Load(MountMaterial, Default));
	Mesh->SetMaterial(SecBand, Load(BandMaterial, Default));
	Mesh->SetMaterial(SecWood, Load(WoodMaterial, Default));
	Mesh->SetMaterial(SecKnob, Load(KnobMaterial, Default));
	for (int32 t = 0; t < Tiles; ++t)
	{
		Mesh->SetMaterial(SecImage + t, ImageMaterials.IsValidIndex(t) ? Load(ImageMaterials[t], Default) : Load(TSoftObjectPtr<UMaterialInterface>(), Default));
	}
}
