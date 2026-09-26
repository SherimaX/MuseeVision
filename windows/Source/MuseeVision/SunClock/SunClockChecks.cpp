#include "SunClock/SunClock.h"
#include "SunClock/SunClockBuild.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeVision.h"
#include "Visitor/MuseeWorld.h"

/**
 * Console commands for checking the sun clock without the editor (e.g. -ExecCmds in a -game run):
 *
 *   musee.SunClock.Check   builds the geometry, writes Saved/SunClock/check.txt (open edges, the
 *                          clearances between the lowered drum and the shaft, the overlaps of every
 *                          moving part with the fixed ones at several lifts, the stair's steps along
 *                          five walking lines against the hidden ramp, the port's outline) and an OBJ
 *                          of each part (metres).
 *   musee.SunClock.Spawn [up] [noground]   places an ASunClock at the origin in the running game if the
 *                          map has none (and hides the imported floor disc), for a look before it is
 *                          placed; noground hides the imported ground plane, which still covers the shaft.
 *   musee.SunClock.Lift <0…1>   sets the lift at once.
 */
namespace SunClockChecks
{
	using namespace SunClockBuild;

	struct FPart
	{
		FString Name;
		const FMeshData* Mesh;
		double Drop = 0;   // m: moved down by this (the lowered panels)
	};

	static FString Where(const FVector& P)
	{
		const double R = FVector2D(P.X, P.Y).Size();
		return FString::Printf(TEXT("r %.3f phi %.1f deg z %.3f"), R, FMath::RadiansToDegrees(Wrap(FMath::Atan2(P.Y, P.X))), P.Z);
	}

	/** Open edges (used by one triangle) of the parts together, after welding positions 0.05 mm apart. */
	static void OpenEdges(TArray<FString>& Log, const FString& Title, const TArray<const FMeshData*>& Meshes)
	{
		TMap<FIntVector, int32> Weld;
		TArray<FVector> Points;
		auto Id = [&](const FVector& Cm)
		{
			const FIntVector Q(FMath::RoundToInt(Cm.X * 200), FMath::RoundToInt(Cm.Y * 200), FMath::RoundToInt(Cm.Z * 200));
			if (const int32* Found = Weld.Find(Q)) { return *Found; }
			Points.Add(Cm / 100.0);
			return Weld.Add(Q, Points.Num() - 1);
		};
		TMap<TPair<int32, int32>, int32> Edges;
		int32 Tris = 0;
		for (const FMeshData* M : Meshes)
		{
			for (int32 T = 0; T + 2 < M->Indices.Num(); T += 3)
			{
				++Tris;
				const int32 V[3] = {Id(M->Positions[M->Indices[T]]), Id(M->Positions[M->Indices[T + 1]]), Id(M->Positions[M->Indices[T + 2]])};
				for (int32 K = 0; K < 3; ++K)
				{
					const int32 A = V[K], B = V[(K + 1) % 3];
					if (A == B) { continue; }
					Edges.FindOrAdd(TPair<int32, int32>(FMath::Min(A, B), FMath::Max(A, B)))++;
				}
			}
		}
		// Group the open edges by where they are (5 cm cells of r and z).
		TMap<FIntPoint, TPair<int32, double>> Cells;
		TMap<FIntPoint, FVector> Sample;
		int32 Open = 0;
		double Length = 0;
		for (const TPair<TPair<int32, int32>, int32>& E : Edges)
		{
			if (E.Value != 1) { continue; }
			const FVector A = Points[E.Key.Key], B = Points[E.Key.Value], Mid = (A + B) / 2;
			++Open;
			Length += FVector::Distance(A, B);
			const FIntPoint Cell(FMath::RoundToInt(FVector2D(Mid.X, Mid.Y).Size() * 20), FMath::RoundToInt(Mid.Z * 20));
			TPair<int32, double>& C = Cells.FindOrAdd(Cell);
			C.Key++;
			C.Value += FVector::Distance(A, B);
			Sample.FindOrAdd(Cell) = Mid;
		}
		Log.Add(FString::Printf(TEXT("%s: %d triangles, %d open edges (%.2f m)"), *Title, Tris, Open, Length));
		TArray<FIntPoint> Keys;
		Cells.GetKeys(Keys);
		Keys.Sort([&Cells](const FIntPoint& A, const FIntPoint& B) { return Cells[A].Value > Cells[B].Value; });
		for (int32 I = 0; I < Keys.Num() && I < 24; ++I)
		{
			Log.Add(FString::Printf(TEXT("    %4d edges, %6.2f m, near r %.2f z %.2f (e.g. %s)"), Cells[Keys[I]].Key, Cells[Keys[I]].Value,
				Keys[I].X / 20.0, Keys[I].Y / 20.0, *Where(Sample[Keys[I]])));
		}
	}

	/** Fixed parts must stay out of the lowered drum's volume. */
	static void Clearances(TArray<FString>& Log, const TArray<FPart>& Fixed, const TArray<FPart>& Moving)
	{
		// The lowered drum: the dial's slab (r <= 6.99, z >= -0.14), its wall (r 6.545–6.99, down to −3.56),
		// the pendant over the well (r < 0.42, down to −0.265).
		auto InDrum = [](const FVector& P)
		{
			const double R = FVector2D(P.X, P.Y).Size();
			// Under the dial: the ribs at -0.13, the bronze rim (r 6.19 on) at -0.14.
			const double Under = R > GrooveInner - 0.005 && R < SC::DrumWallInner + 0.005 ? GrooveZ : (R > 6.185 ? SoffitZ : RibZ);
			if (R <= SC::DrumOuter + 0.005 && P.Z > Under - 0.005) { return true; }
			if (R >= SC::DrumWallInner - 0.002 && R <= SC::DrumOuter + 0.002 && P.Z > DrumBottom - 0.005) { return true; }
			return R < 0.425 && P.Z > -0.27;
		};
		for (const FPart& F : Fixed)
		{
			int32 Bad = 0;
			FString First;
			for (const FVector& Cm : F.Mesh->Positions)
			{
				const FVector P = Cm / 100.0 - FVector(0, 0, F.Drop);
				if (InDrum(P) && P.Z < 0.02)   // the ring's own top (z 0, r ≥ 7) is the floor
				{
					if (FVector2D(P.X, P.Y).Size() >= SC::DialRadius - 1e-6) { continue; }
					if (Bad++ == 0) { First = Where(P); }
				}
			}
			Log.Add(FString::Printf(TEXT("  %-16s %s"), *F.Name, Bad ? *FString::Printf(TEXT("%d points inside the lowered drum, e.g. %s"), Bad, *First) : TEXT("clear")));
		}
		// The drum itself: within r 6.99, below the dial's underside only in the pocket or over the well, and
		// nothing above the dial but its bronze (Drop holds the highest z allowed).
		for (const FPart& D : Moving)
		{
			int32 Wide = 0, Low = 0, High = 0;
			FString First;
			for (const FVector& Cm : D.Mesh->Positions)
			{
				const FVector P = Cm / 100.0;
				const double R = FVector2D(P.X, P.Y).Size();
				if (R > SC::DrumOuter + 1e-4) { if (Wide++ == 0) { First = Where(P); } }
				else if (P.Z > D.Drop + 1e-4) { if (High++ == 0) { First = Where(P); } }
				else if (P.Z < SoffitZ - 1e-4 && !(R > SC::PocketInner + 0.001 && R < SC::DialRadius - 0.005) && R > 0.425)
				{
					if (Low++ == 0) { First = Where(P); }
				}
			}
			Log.Add(FString::Printf(TEXT("  %-16s %d points beyond r 6.99, %d above z %.3f, %d under the dial outside the pocket%s%s"), *D.Name, Wide,
				High, D.Drop, Low, First.IsEmpty() ? TEXT("") : TEXT(", e.g. "), *First));
		}
	}

	/** Vertical ray down onto the ramp's triangles at (x, y): the highest hit, or NaN. */
	static double RampAt(const FMeshData& Ramp, double X, double Y)
	{
		double Best = -1e9;
		const FVector2D Q(X * 100, Y * 100);
		for (int32 T = 0; T + 2 < Ramp.Indices.Num(); T += 3)
		{
			const FVector A = Ramp.Positions[Ramp.Indices[T]], B = Ramp.Positions[Ramp.Indices[T + 1]], C = Ramp.Positions[Ramp.Indices[T + 2]];
			const FVector2D A2(A.X, A.Y), B2(B.X, B.Y), C2(C.X, C.Y);
			const double D = SalonKit::Cross2(B2 - A2, C2 - A2);
			if (FMath::Abs(D) < 1e-9) { continue; }
			const double U = SalonKit::Cross2(Q - A2, C2 - A2) / D, V = SalonKit::Cross2(B2 - A2, Q - A2) / D;
			if (U < -1e-6 || V < -1e-6 || U + V > 1 + 1e-6) { continue; }
			Best = FMath::Max(Best, (A.Z + (B.Z - A.Z) * U + (C.Z - A.Z) * V) / 100.0);
		}
		return Best;
	}

	/** The tread's top under (r, φ), or the floor. */
	static double TreadTop(double Phi)
	{
		const FStairLayout& L = FStairLayout::Get();
		for (int32 K = 1; K < SC::Risers; ++K)
		{
			if (Phi <= L.Back(K) + 1e-9 && Phi > L.Front[K] - 1e-9) { return L.TreadZ(K); }
		}
		return Phi > L.Top ? TNumericLimits<double>::Lowest() : SC::Floor;
	}

	static void Steps(TArray<FString>& Log, const FMeshData& Ramp)
	{
		const FStairLayout& L = FStairLayout::Get();
		Log.Add(FString::Printf(TEXT("Stair: %d risers of %.0f mm; treads turn %.3f deg, the mid landing %.1f deg; top landing %.1f-%.1f deg, last riser at %.2f deg"),
			SC::Risers, SC::Riser * 1000, FMath::RadiansToDegrees(L.Alpha), FMath::RadiansToDegrees(L.Beta),
			FMath::RadiansToDegrees(L.Front[1]), FMath::RadiansToDegrees(L.Top), FMath::RadiansToDegrees(L.End)));
		for (const double R : {2.7, 3.5, 4.4, 5.3, 6.15})
		{
			const double Going = L.Alpha * R;
			double MaxGap = 0, MaxAbove = 0, MaxSlope = 0, MaxRise = 0, Prev = TreadTop(L.Top - 1e-6);
			int32 Holes = 0, Risers = 0;
			double PrevRamp = RampAt(Ramp, R * FMath::Cos(L.Top - 1e-4), R * FMath::Sin(L.Top - 1e-4));
			const double StepPhi = 0.01 * SunClockBuild::Deg;
			for (double Phi = L.Top - 1e-4; Phi > L.End - L.Alpha; Phi -= StepPhi)
			{
				const double Z = RampAt(Ramp, R * FMath::Cos(Phi), R * FMath::Sin(Phi));
				const double Tread = TreadTop(Phi);
				if (Z < -100) { ++Holes; continue; }
				MaxGap = FMath::Max(MaxGap, Tread - Z);     // the ramp below the tread (feet sink)
				MaxAbove = FMath::Max(MaxAbove, Z - Tread);  // the ramp above it (feet float)
				MaxSlope = FMath::Max(MaxSlope, FMath::Abs(Z - PrevRamp) / (StepPhi * R));
				PrevRamp = Z;
				if (Tread < Prev - 1e-6)
				{
					++Risers;
					MaxRise = FMath::Max(MaxRise, Prev - Tread);
				}
				Prev = Tread;
			}
			Log.Add(FString::Printf(TEXT("  r %.2f: going %.3f m, 2R+G %.3f; %d risers, highest %.3f m; ramp %.3f under / %.3f over the treads, slope %.1f deg; %d samples off the ramp"),
				R, Going, 2 * SC::Riser + Going, Risers, MaxRise, MaxGap, MaxAbove, FMath::RadiansToDegrees(FMath::Atan(MaxSlope)), Holes));
		}
		// Headroom under the raised drum over the top landing, and the ramp's own ends.
		Log.Add(FString::Printf(TEXT("  headroom over the top landing (raised): %.2f m; ramp from %.3f to %.3f m"),
			SC::Lift + RibZ - L.TreadZ(1), L.RampZ(L.Top), L.RampZ(L.End - 10 * SunClockBuild::Deg)));
	}

	static void Port(TArray<FString>& Log)
	{
		Log.Add(TEXT("Port (the joint at r 7.6, facing north): jambs x +-1.8 from -6.0 to the springing -3.4, a half-round arch r 1.8 about (x 0, z -3.4), crown -1.6"));
		for (const int32 J : {0, 12, 24, 36, ArchSegments})
		{
			const double X = PortX(J), Y = -FMath::Sqrt(SC::ShaftOuter * SC::ShaftOuter - X * X);
			Log.Add(FString::Printf(TEXT("    sample %2d: x %+.4f y %.4f z %.4f"), J, X, Y, PortArchZ(J)));
		}
		Log.Add(FString::Printf(TEXT("    sill %.2f; inside r %.2f a tympanum over a lintel at %.2f; the portal onto the stair %.2f x %.2f m"),
			SC::Floor, SC::PortTympanum, SC::PortLintel, SC::PortWidth, SC::PortLintel - SC::Floor));
	}

	// ---- Overlaps: every moving part against every fixed part, triangle against triangle, at several lifts.

	/** A part's triangles (m, 3 points each), in its group's own frame. */
	struct FTriPart
	{
		FString Name;
		TArray<FVector> Tri;

		FTriPart(const FString& InName, const TArray<const FMeshData*>& Meshes) : Name(InName)
		{
			for (const FMeshData* M : Meshes)
			{
				for (int32 T = 0; T + 2 < M->Indices.Num(); T += 3)
				{
					for (int32 K = 0; K < 3; ++K) { Tri.Add(M->Positions[M->Indices[T + K]] / 100.0); }
				}
			}
		}
		int32 Num() const { return Tri.Num() / 3; }
	};

	/** Does the segment PQ pass through the triangle ABC's inside? (Touching, or lying in its plane, is not crossing.) */
	static bool SegmentCrosses(const FVector& P, const FVector& Q, const FVector& A, const FVector& B, const FVector& C)
	{
		constexpr double Eps = 1e-7;
		const FVector E1 = B - A, E2 = C - A, D = Q - P;
		const FVector H = FVector::CrossProduct(D, E2);
		const double Det = FVector::DotProduct(E1, H);
		const double Scale = E1.Size() * E2.Size() * D.Size();
		if (Scale < 1e-18 || FMath::Abs(Det) < 1e-9 * Scale) { return false; }
		const double Inv = 1.0 / Det;
		const FVector S = P - A;
		const double U = Inv * FVector::DotProduct(S, H);
		if (U <= Eps || U >= 1 - Eps) { return false; }
		const FVector Qv = FVector::CrossProduct(S, E1);
		const double V = Inv * FVector::DotProduct(D, Qv);
		if (V <= Eps || U + V >= 1 - Eps) { return false; }
		const double T = Inv * FVector::DotProduct(E2, Qv);
		return T > Eps && T < 1 - Eps;
	}

	static bool TrianglesCross(const FVector* A, const FVector* B)
	{
		for (int32 K = 0; K < 3; ++K)
		{
			if (SegmentCrosses(A[K], A[(K + 1) % 3], B[0], B[1], B[2]) || SegmentCrosses(B[K], B[(K + 1) % 3], A[0], A[1], A[2])) { return true; }
		}
		return false;
	}

	/** A group's triangles hashed into 20 cm cells by their bounds. */
	struct FTriGrid
	{
		static constexpr double Cell = 0.2;
		TArray<const FTriPart*> Parts;
		TArray<TPair<int32, int32>> Items;   // (part, triangle)
		TArray<FBox> Bounds;
		TMap<FIntVector, TArray<int32>> Cells;

		static FIntVector Key(const FVector& P)
		{
			return FIntVector(FMath::FloorToInt(P.X / Cell), FMath::FloorToInt(P.Y / Cell), FMath::FloorToInt(P.Z / Cell));
		}

		explicit FTriGrid(const TArray<const FTriPart*>& InParts) : Parts(InParts)
		{
			for (int32 Pi = 0; Pi < Parts.Num(); ++Pi)
			{
				for (int32 T = 0; T < Parts[Pi]->Num(); ++T)
				{
					const FVector* V = &Parts[Pi]->Tri[3 * T];
					FBox Box(ForceInit);
					Box += V[0];
					Box += V[1];
					Box += V[2];
					const int32 Id = Items.Add(TPair<int32, int32>(Pi, T));
					Bounds.Add(Box);
					const FIntVector Lo = Key(Box.Min), Hi = Key(Box.Max);
					for (int32 X = Lo.X; X <= Hi.X; ++X)
					{
						for (int32 Y = Lo.Y; Y <= Hi.Y; ++Y)
						{
							for (int32 Z = Lo.Z; Z <= Hi.Z; ++Z) { Cells.FindOrAdd(FIntVector(X, Y, Z)).Add(Id); }
						}
					}
				}
			}
		}
	};

	struct FCrossings
	{
		int32 Count = 0;
		FVector Example = FVector::ZeroVector;
	};

	/** The moving part's triangles, raised by Dz, against the grid's: crossings per (moving, still) pair. */
	static void CrossPart(const FTriPart& Moving, double Dz, const FTriGrid& Grid, TMap<FString, FCrossings>& Out)
	{
		TArray<int32> Stamp;
		Stamp.Init(INDEX_NONE, Grid.Items.Num());
		const FVector Lift(0, 0, Dz);
		for (int32 T = 0; T < Moving.Num(); ++T)
		{
			const FVector V[3] = {Moving.Tri[3 * T] + Lift, Moving.Tri[3 * T + 1] + Lift, Moving.Tri[3 * T + 2] + Lift};
			FBox Box(ForceInit);
			Box += V[0];
			Box += V[1];
			Box += V[2];
			const FIntVector Lo = FTriGrid::Key(Box.Min), Hi = FTriGrid::Key(Box.Max);
			for (int32 X = Lo.X; X <= Hi.X; ++X)
			{
				for (int32 Y = Lo.Y; Y <= Hi.Y; ++Y)
				{
					for (int32 Z = Lo.Z; Z <= Hi.Z; ++Z)
					{
						const TArray<int32>* List = Grid.Cells.Find(FIntVector(X, Y, Z));
						if (!List) { continue; }
						for (const int32 Id : *List)
						{
							if (Stamp[Id] == T) { continue; }
							Stamp[Id] = T;
							if (!Box.Intersect(Grid.Bounds[Id])) { continue; }
							const TPair<int32, int32>& Item = Grid.Items[Id];
							const FTriPart& Still = *Grid.Parts[Item.Key];
							if (!TrianglesCross(V, &Still.Tri[3 * Item.Value])) { continue; }
							FCrossings& C = Out.FindOrAdd(Moving.Name + TEXT(" x ") + Still.Name);
							if (C.Count++ == 0) { C.Example = (V[0] + V[1] + V[2]) / 3; }
						}
					}
				}
			}
		}
	}

	/** The lights' fittings (the drum's frame): a box for each cove spot and the well's spot, a notional one for the fill. */
	static FMeshData LightBounds()
	{
		FMeshData L;
		for (int32 I = 0; I < CoveCount; ++I)
		{
			const FVector C = Polar(CoveRadius, CovePhi(I), CoveZ);
			L.Box(C + FVector(-0.04, -0.04, -0.012), C + FVector(0.04, 0.04, 0.008), FMeshData::AllFaces);
		}
		L.Box(FVector(-0.05, -0.05, WellLightZ - 0.02), FVector(0.05, 0.05, WellLightZ + 0.025), FMeshData::AllFaces);
		L.Box(FVector(-0.05, -0.05, FillLightZ - 0.05), FVector(0.05, 0.05, FillLightZ + 0.05), FMeshData::AllFaces);
		return L;
	}

	/** The lowered drum's underside at (x, y) (its own frame): the ribs, the rim, the groove, the pendant, the lights. */
	static double UnderDial(double X, double Y)
	{
		const double R = FVector2D(X, Y).Size();
		if (R < 0.425) { return -0.27; }
		for (int32 I = 0; I < CoveCount; ++I)
		{
			const FVector C = Polar(CoveRadius, CovePhi(I), 0);
			if (FVector2D::Distance(FVector2D(X, Y), FVector2D(C.X, C.Y)) < 0.05) { return CoveZ - 0.012; }
		}
		if (R > GrooveInner - 0.005 && R < SC::DrumWallInner + 0.005) { return GrooveZ; }
		return R > 6.185 ? SoffitZ : RibZ;
	}

	/**
	 * The least vertical clearance (m) between a part (raised by Dz) and the underside of the drum standing Lift up,
	 * over the part's points inside the stair's wall: negative is a clash. At names where it is.
	 */
	static double ClearUnder(const FMeshData& Part, double Dz, double Lift, FString* At = nullptr)
	{
		double Best = 1e9;
		for (const FVector& Cm : Part.Positions)
		{
			const FVector P = Cm / 100.0 + FVector(0, 0, Dz);
			if (FVector2D(P.X, P.Y).Size() >= SC::PocketInner) { continue; }
			const double Gap = Lift + UnderDial(P.X, P.Y) - P.Z;
			if (Gap < Best)
			{
				Best = Gap;
				if (At) { *At = Where(P); }
			}
		}
		return Best;
	}

	static void Overlaps(TArray<FString>& Log, const FSunClockMeshes& M)
	{
		const FMeshData Lights = LightBounds();
		const TArray<FTriPart> Drum = {
			FTriPart(TEXT("Dial"), {&M.DialA, &M.DialB, &M.DialRosso, &M.DialNero}), FTriPart(TEXT("DrumStone"), {&M.DrumStone}),
			FTriPart(TEXT("Soffit"), {&M.DrumSoffit}), FTriPart(TEXT("DrumBronze"), {&M.DrumBronze}), FTriPart(TEXT("Pendant"), {&M.DrumGilt}),
			FTriPart(TEXT("Lights"), {&Lights})};
		const TArray<FTriPart> Panels = {FTriPart(TEXT("Panels"), {&M.TopRailBronze}), FTriPart(TEXT("PanelFence"), {&M.TopRailFence})};
		const TArray<FTriPart> Fixed = {
			FTriPart(TEXT("Ring"), {&M.RingA, &M.RingB, &M.RingBronze, &M.RingGilt}), FTriPart(TEXT("Shaft"), {&M.Masonry, &M.PortStone}),
			FTriPart(TEXT("Landing"), {&M.FloorA, &M.FloorRosso, &M.FloorNero}), FTriPart(TEXT("Treads"), {&M.Treads}),
			FTriPart(TEXT("String"), {&M.Curb}), FTriPart(TEXT("StairSoffit"), {&M.StairSoffit}), FTriPart(TEXT("Ramp"), {&M.Ramp}),
			FTriPart(TEXT("Housing"), {&M.Housing, &M.HousingStone}), FTriPart(TEXT("FixedRails"), {&M.RailBronze}), FTriPart(TEXT("FixedFence"), {&M.RailFence})};
		auto Pointers = [](const TArray<FTriPart>& Parts)
		{
			TArray<const FTriPart*> Out;
			for (const FTriPart& P : Parts) { Out.Add(&P); }
			return Out;
		};
		const FTriGrid FixedGrid(Pointers(Fixed)), DrumGrid(Pointers(Drum));

		struct FCase
		{
			FString Name;
			double Drum, Panel;   // their heights over the lowered drum's / the raised panels' (m)
		};
		auto Panel = [](double H) { return PanelOffset(H); };
		auto Eased = [](double T) { return T * T * (3 - 2 * T) * SC::Lift; };
		const TArray<FCase> Cases = {
			{TEXT("lift 0 (down)"), 0.0, Panel(0.0)},
			{TEXT("lift 0, the drum 5 mm lower (margin)"), -0.005, Panel(0.0)},
			{TEXT("lift 0, the panels 5 mm higher (margin)"), 0.0, Panel(0.0) + 0.005},
			{TEXT("lift 0.2"), Eased(0.2), Panel(Eased(0.2))},
			{TEXT("the panels start to rise"), PanelRiseFrom, Panel(PanelRiseFrom)},
			{TEXT("the panels half up"), PanelRiseFrom + PanelDrop / 2, Panel(PanelRiseFrom + PanelDrop / 2)},
			{TEXT("the panels just up"), PanelRiseFrom + PanelDrop, Panel(PanelRiseFrom + PanelDrop)},
			{TEXT("lift 0.5"), Eased(0.5), Panel(Eased(0.5))},
			{TEXT("lift 1 (up)"), Eased(1.0), Panel(Eased(1.0))}};
		int32 Parts = 0;
		for (const TArray<FTriPart>* G : {&Drum, &Panels, &Fixed})
		{
			for (const FTriPart& P : *G) { Parts += P.Num(); }
		}
		Log.Add(FString::Printf(TEXT("Overlaps (triangles crossing; %d triangles): the drum (dial, stone, soffit and ribs, rim, pendant, the lights' fittings) and the retracting panels (and their fence) against the fixed parts (ring, shaft and sleeve, landing, treads, string, stair's underside, ramp, housings, fixed rails and fence), and the panels against the drum:"), Parts));
		int32 Bad = 0;
		for (const FCase& C : Cases)
		{
			TMap<FString, FCrossings> Hits;
			for (const FTriPart& P : Drum) { CrossPart(P, C.Drum, FixedGrid, Hits); }
			for (const FTriPart& P : Panels)
			{
				CrossPart(P, C.Panel, FixedGrid, Hits);
				CrossPart(P, C.Panel - C.Drum, DrumGrid, Hits);
			}
			FString Line = FString::Printf(TEXT("  %-40s drum %+.3f m, panels %+.3f m: "), *C.Name, C.Drum, C.Panel);
			if (Hits.Num() == 0) { Log.Add(Line + TEXT("clear")); continue; }
			Log.Add(Line + FString::Printf(TEXT("%d pairs of parts cross"), Hits.Num()));
			for (const TPair<FString, FCrossings>& H : Hits)
			{
				Bad += H.Value.Count;
				Log.Add(FString::Printf(TEXT("      %-26s %6d triangles, e.g. at %s (the actor's frame)"), *H.Key, H.Value.Count, *Where(H.Value.Example)));
			}
		}
		Log.Add(FString::Printf(TEXT("  overlaps in all: %d"), Bad));

		// The whole motion, densely (the lift eased as the actor eases it): every moving part against the fixed ones and
		// the panels against the drum, and how close the drum's underside comes to the retracting panels' tops.
		constexpr int32 Samples = 200;
		int32 SweepBad = 0;
		double MinGap = 1e9, MinGapLift = 0;
		FString MinGapWhere;
		for (int32 I = 0; I <= Samples; ++I)
		{
			const double H = Eased(double(I) / Samples), Dp = PanelOffset(H);
			TMap<FString, FCrossings> Hits;
			for (const FTriPart& P : Drum) { CrossPart(P, H, FixedGrid, Hits); }
			for (const FTriPart& P : Panels)
			{
				CrossPart(P, Dp, FixedGrid, Hits);
				CrossPart(P, Dp - H, DrumGrid, Hits);
			}
			for (const TPair<FString, FCrossings>& Hh : Hits)
			{
				Log.Add(FString::Printf(TEXT("      lift %.3f: %-26s %6d triangles, e.g. at %s"), H, *Hh.Key, Hh.Value.Count, *Where(Hh.Value.Example)));
				SweepBad += Hh.Value.Count;
			}
			FString At;
			const double Gap = ClearUnder(M.TopRailBronze, Dp, H, &At);
			if (Gap < MinGap)
			{
				MinGap = Gap;
				MinGapLift = H;
				MinGapWhere = At;
			}
		}
		Log.Add(FString::Printf(TEXT("  the whole motion, %d lifts from 0 to %.2f m: %d crossings; the panels' least clearance under the drum %.3f m (at lift %.3f, %s)"),
			Samples + 1, SC::Lift, SweepBad, MinGap, MinGapLift, *MinGapWhere));
		// Closed: how far under the dial's underside each fixed part and the lowered panels stay.
		const TArray<TPair<FString, const FMeshData*>> Still = {
			{TEXT("fixed rails"), &M.RailBronze}, {TEXT("housings (bronze)"), &M.Housing}, {TEXT("housings (marble)"), &M.HousingStone},
			{TEXT("string"), &M.Curb}, {TEXT("treads"), &M.Treads}};
		for (const TPair<FString, const FMeshData*>& P : Still)
		{
			FString At;
			const double Gap = ClearUnder(*P.Value, 0.0, 0.0, &At);
			Log.Add(FString::Printf(TEXT("  closed: %-18s %.3f m under the dial's underside at the closest (%s)"), *P.Key, Gap, *At));
		}
		{
			FString At;
			const double Gap = ClearUnder(M.TopRailBronze, PanelOffset(0.0), 0.0, &At);
			Log.Add(FString::Printf(TEXT("  closed: %-18s %.3f m under the dial's underside at the closest (%s)"), TEXT("panels (home)"), Gap, *At));
		}
	}

	static void WriteObj(const FString& Dir, const FString& Name, const FMeshData& M)
	{
		FString S;
		S.Reserve(M.Positions.Num() * 40 + M.Indices.Num() * 12);
		for (const FVector& P : M.Positions) { S += FString::Printf(TEXT("v %.5f %.5f %.5f\n"), P.X / 100, P.Y / 100, P.Z / 100); }
		for (int32 T = 0; T + 2 < M.Indices.Num(); T += 3)
		{
			S += FString::Printf(TEXT("f %d %d %d\n"), M.Indices[T] + 1, M.Indices[T + 1] + 1, M.Indices[T + 2] + 1);
		}
		FFileHelper::SaveStringToFile(S, *FPaths::Combine(Dir, Name + TEXT(".obj")));
	}

	static void Check(const TArray<FString>& Args, UWorld* World)
	{
		const double Start = FPlatformTime::Seconds();
		FSunClockMeshes M;
		BuildAll(M);
		const double BuiltIn = FPlatformTime::Seconds() - Start;
		TArray<FString> Log;
		Log.Add(FString::Printf(TEXT("Sun clock geometry, built in %.2f s"), BuiltIn));
		const TArray<TPair<FString, const FMeshData*>> All = {
			{TEXT("DialA"), &M.DialA}, {TEXT("DialB"), &M.DialB}, {TEXT("DialRosso"), &M.DialRosso}, {TEXT("DialNero"), &M.DialNero},
			{TEXT("DrumStone"), &M.DrumStone}, {TEXT("DrumSoffit"), &M.DrumSoffit}, {TEXT("DrumBronze"), &M.DrumBronze}, {TEXT("DrumGilt"), &M.DrumGilt},
			{TEXT("RingA"), &M.RingA}, {TEXT("RingB"), &M.RingB}, {TEXT("RingBronze"), &M.RingBronze}, {TEXT("RingGilt"), &M.RingGilt},
			{TEXT("Masonry"), &M.Masonry}, {TEXT("PortStone"), &M.PortStone}, {TEXT("FloorA"), &M.FloorA}, {TEXT("FloorRosso"), &M.FloorRosso}, {TEXT("FloorNero"), &M.FloorNero},
			{TEXT("Treads"), &M.Treads}, {TEXT("Curb"), &M.Curb}, {TEXT("StairSoffit"), &M.StairSoffit}, {TEXT("Ramp"), &M.Ramp},
			{TEXT("RailBronze"), &M.RailBronze}, {TEXT("RailFence"), &M.RailFence}, {TEXT("Housing"), &M.Housing}, {TEXT("HousingStone"), &M.HousingStone},
			{TEXT("TopRailBronze"), &M.TopRailBronze}, {TEXT("TopRailFence"), &M.TopRailFence}};
		int32 Total = 0;
		for (const TPair<FString, const FMeshData*>& P : All)
		{
			Total += P.Value->Indices.Num() / 3;
			Log.Add(FString::Printf(TEXT("  %-14s %7d triangles, %7d vertices"), *P.Key, P.Value->Indices.Num() / 3, P.Value->Positions.Num()));
		}
		Log.Add(FString::Printf(TEXT("  total %d triangles"), Total));

		Log.Add(TEXT("Open edges (a closed shell has none; expected ones are named):"));
		OpenEdges(Log, TEXT("The drum's shell (dial, stone, soffit, rim) - expect the soffit's edge under the rim (r 6.19-6.20) and the pilasters' and dressings' sunk backs"),
				  {&M.DialA, &M.DialB, &M.DialRosso, &M.DialNero, &M.DrumStone, &M.DrumSoffit});
		OpenEdges(Log, TEXT("The fixed floor and shaft (ring, masonry, landing) - expect only r 7.6 (the joint) and r 10.3 (the ring's edge)"),
				  {&M.RingA, &M.RingB, &M.Masonry, &M.FloorA, &M.FloorRosso, &M.FloorNero});
		OpenEdges(Log, TEXT("The stair (treads, string, underside) - expect the tread ends sunk into the string and wall"),
				  {&M.Treads, &M.Curb, &M.StairSoffit});
		OpenEdges(Log, TEXT("The ramp (hidden)"), {&M.Ramp});

		Log.Add(TEXT("Clearances with the drum lowered (fixed parts outside it; the drum inside its pocket):"));
		const FMeshData Lights = LightBounds();
		Clearances(Log,
			{{TEXT("Masonry"), &M.Masonry}, {TEXT("PortStone"), &M.PortStone}, {TEXT("Floor"), &M.FloorA}, {TEXT("Treads"), &M.Treads}, {TEXT("Curb"), &M.Curb},
			 {TEXT("StairSoffit"), &M.StairSoffit}, {TEXT("RailBronze"), &M.RailBronze}, {TEXT("RailFence"), &M.RailFence},
			 {TEXT("Housing"), &M.Housing}, {TEXT("HousingStone"), &M.HousingStone}, {TEXT("Ramp"), &M.Ramp},
			 {TEXT("Panels (down)"), &M.TopRailBronze, PanelDrop}, {TEXT("PanelFence (down)"), &M.TopRailFence, PanelDrop}},
			{{TEXT("DrumStone"), &M.DrumStone, 0.0}, {TEXT("DrumSoffit"), &M.DrumSoffit, 0.0}, {TEXT("DrumBronze"), &M.DrumBronze, 0.045},
			 {TEXT("DrumGilt"), &M.DrumGilt, 0.09}, {TEXT("Dial"), &M.DialA, 0.0}, {TEXT("Lights"), &Lights, 0.0}});
		const double OverlapStart = FPlatformTime::Seconds();
		Overlaps(Log, M);
		Log.Add(FString::Printf(TEXT("  (the overlap test took %.1f s)"), FPlatformTime::Seconds() - OverlapStart));
		Steps(Log, M.Ramp);
		Port(Log);

		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SunClock"));
		IFileManager::Get().MakeDirectory(*Dir, true);
		if (!Args.Contains(TEXT("noobj")))
		{
			for (const TPair<FString, const FMeshData*>& P : All) { WriteObj(Dir, P.Key, *P.Value); }
		}
		FFileHelper::SaveStringArrayToFile(Log, *FPaths::Combine(Dir, TEXT("check.txt")));
		for (const FString& Line : Log) { UE_LOG(LogMusee, Log, TEXT("SunClock check: %s"), *Line); }
	}

	static ASunClock* Find(UWorld* World)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<ASunClock> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	static void Spawn(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		ASunClock* Clock = Find(World);
		if (!Clock)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Clock = World->SpawnActor<ASunClock>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			UE_LOG(LogMusee, Log, TEXT("SunClock: spawned for this run."));
		}
		for (const FString& Prim : ASunClock::GetReplacedImportPrims())
		{
			if (AActor* Old = MuseeWorld::FindPrim(World, Prim))
			{
				Old->SetActorHiddenInGame(true);
				Old->SetActorEnableCollision(false);
				UE_LOG(LogMusee, Log, TEXT("SunClock: %s hidden for this run."), *Prim);
			}
		}
		if (Args.Contains(TEXT("noground")))
		{
			// The imported ground plane (z -0.03) still runs over the shaft until it has a hole there.
			if (AActor* Ground = MuseeWorld::FindPrim(World, TEXT("/Museum/HallOfLight/Ground")))
			{
				Ground->SetActorHiddenInGame(true);
				Ground->SetActorEnableCollision(false);
				UE_LOG(LogMusee, Log, TEXT("SunClock: the imported ground hidden for this run."));
			}
		}
		if (Clock && Args.Contains(TEXT("up"))) { Clock->SetLiftFraction(1.f); }
	}

	static void Lift(const TArray<FString>& Args, UWorld* World)
	{
		if (ASunClock* Clock = Find(World)) { Clock->SetLiftFraction(Args.Num() ? FCString::Atof(*Args[0]) : 1.f); }
	}

	static FAutoConsoleCommandWithWorldAndArgs CheckCommand(TEXT("musee.SunClock.Check"),
		TEXT("Builds the sun clock's geometry and writes Saved/SunClock/check.txt and OBJs (noobj: none)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Check));
	static FAutoConsoleCommandWithWorldAndArgs SpawnCommand(TEXT("musee.SunClock.Spawn"),
		TEXT("Places a sun clock at the origin for this run if the map has none (up: raised)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spawn));
	static FAutoConsoleCommandWithWorldAndArgs LiftCommand(TEXT("musee.SunClock.Lift"),
		TEXT("Sets the sun clock's lift at once, 0 (down) … 1 (up)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Lift));
}
