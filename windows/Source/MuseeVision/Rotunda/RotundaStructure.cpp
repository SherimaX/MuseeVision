#include "Rotunda/RotundaStructure.h"

#include "Chenghuai/ChenghuaiPlan.h"

#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeMesh.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"

/**
 * The Rotunda's geometry, in metres in the actor's frame: X east, Y south, Z up, plan angles measured
 * from east towards south. UVs are in metres (V down the walls), so tiling materials keep their scale.
 *
 * Watertight by construction: where two surfaces meet they are built from the same vertices (the wall's
 * faces and the reveals, the entablature and the dome, the coffers and the dome's bands, the eye's curb
 * and its glass), or one sinks a centimetre into the other (pilasters and mouldings into the wall).
 */
namespace RotundaBuild
{
	namespace R = MuseePlan::Rotunda;

	/** Segments in every semicircular arch: the doors, the niches, their soffits and mouldings. */
	constexpr int32 ArchSegments = 48;
	/** Samples along each side of a coffer; every ring round the dome uses the same 280 angles. */
	constexpr int32 CofferSamples = 10;
	/** Largest angle between two stations of the drum's faces (320 round). */
	constexpr double WallStep = 2 * UE_DOUBLE_PI / 320;
	constexpr double OuterRadius = R::Radius + R::Wall;
	/** The shadow gap at the foot of the drum: 150 mm high, 40 mm deep. */
	constexpr double GapTop = 0.15;
	constexpr double GapRadius = R::Radius + 0.04;
	/** How far mouldings sink into the wall behind them. */
	constexpr double Sink = 0.01;
	/** Finite-difference step for the normals of swept surfaces. */
	constexpr double DiffStep = 1e-5;

	/** Pilasters: the face 0.43 m proud of the wall; the back sunk into it, past the shadow gap. */
	constexpr double PilasterFront = R::Radius - R::PilasterDepth + 0.02;
	constexpr double PilasterBack = R::Radius + 0.06;
	constexpr double PilasterHalf = R::PilasterWidth / 2;
	constexpr double PilasterReach = PilasterHalf + 0.115;   // half the abacus, the widest part
	constexpr double ShaftBottom = 0.32;
	constexpr double FluteBottom = 0.60;
	constexpr double FluteTop = 8.62;
	constexpr double ShaftTop = 8.90;

	/** The cornice's top, where the dome's surface begins. */
	constexpr double CorniceTop = 10.5;
	/** The dome's outer shell (board section: R 318 px at 30 px/m). */
	constexpr double ShellRadius = 10.6;
	/** The eye: the dome stops at r 3 m, steps up twice to the Ø 5 m curb, which rises to 20.45 m. */
	constexpr double EyeRim = 3.0;
	constexpr double CurbOuter = 2.9;
	constexpr double CurbTop = 20.45;
	/** The lattice's ring (r 2.42–2.5 m) carries the glass at 20.38 m, just over the node. */
	constexpr double RingInner = 2.42;
	constexpr double RingBottom = 20.22;
	constexpr double GlassHeight = 20.38;

	/** A profile's outward normal: its tangent turned clockwise (the solid lies on the left). */
	FVector2D Outward(const FVector2D& Tangent) { return FVector2D(Tangent.Y, -Tangent.X); }

	/** A triangle, unless it has no area (a pole, a reveal of zero depth at an arch's crown). */
	void AddTri(FMuseeMesh& Out, const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB,
				const FVector& NC, const FVector2D& UA, const FVector2D& UB, const FVector2D& UC)
	{
		if (FVector::CrossProduct(B - A, C - A).SizeSquared() > 1e-14) { Out.Tri(A, B, C, NA, NB, NC, UA, UB, UC); }
	}

	void AddQuad(FMuseeMesh& Out, const FVector& A, const FVector& B, const FVector& C, const FVector& D,
				 const FVector& NA, const FVector& NB, const FVector& NC, const FVector& ND,
				 const FVector2D& UA, const FVector2D& UB, const FVector2D& UC, const FVector2D& UD)
	{
		AddTri(Out, A, B, C, NA, NB, NC, UA, UB, UC);
		AddTri(Out, A, C, D, NA, NC, ND, UA, UC, UD);
	}

	/**
	 * A moulding's section: straight runs (hard edges between them) and arcs (smooth), traversed with the
	 * solid on the left.
	 */
	struct FProfile
	{
		struct FPiece
		{
			FVector2D A, B;
			FVector2D TangentA, TangentB;
		};
		TArray<FPiece> Pieces;
		FVector2D Cursor;

		explicit FProfile(const FVector2D& Start) : Cursor(Start) {}

		FProfile& LineTo(const FVector2D& Target)
		{
			const FVector2D Dir = (Target - Cursor).GetSafeNormal();
			Pieces.Add({Cursor, Target, Dir, Dir});
			Cursor = Target;
			return *this;
		}

		FProfile& LineTo(double X, double Y) { return LineTo(FVector2D(X, Y)); }

		/** An arc about Centre from the cursor, which lies on it at angle From, to angle To (radians). */
		FProfile& ArcTo(const FVector2D& Centre, double Radius, double From, double To, int32 Steps)
		{
			const double Sign = To > From ? 1.0 : -1.0;
			for (int32 i = 1; i <= Steps; ++i)
			{
				const double A0 = From + (To - From) * (i - 1) / Steps;
				const double A1 = i == Steps ? To : From + (To - From) * i / Steps;
				const FVector2D Target = Centre + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * Radius;
				Pieces.Add({Cursor, Target, FVector2D(-FMath::Sin(A0), FMath::Cos(A0)) * Sign, FVector2D(-FMath::Sin(A1), FMath::Cos(A1)) * Sign});
				Cursor = Target;
			}
			return *this;
		}

		/** The section as a closed polygon, without points in line with their neighbours (for end caps). */
		TArray<FVector2D> Outline() const
		{
			TArray<FVector2D> Points;
			for (const FPiece& Piece : Pieces) { Points.Add(Piece.A); }
			if (FVector2D::DistSquared(Cursor, Points[0]) > 1e-12) { Points.Add(Cursor); }
			for (int32 i = 0; i < Points.Num() && Points.Num() > 3;)
			{
				const FVector2D Prev = Points[(i + Points.Num() - 1) % Points.Num()];
				const FVector2D Next = Points[(i + 1) % Points.Num()];
				if (FMath::Abs(FVector2D::CrossProduct(Points[i] - Prev, Next - Points[i])) < 1e-12) { Points.RemoveAt(i); }
				else { ++i; }
			}
			return Points;
		}
	};

	/** Where a section point (a, b) lands in space at path parameter T. */
	using FPlace = TFunctionRef<FVector(double, const FVector2D&)>;

	/** Turns a (radius, height) point about the vertical axis by an angle. */
	struct FRevolve
	{
		FVector operator()(double Angle, const FVector2D& Q) const { return FVector(Q.X * FMath::Cos(Angle), Q.X * FMath::Sin(Angle), Q.Y); }
	};

	/** The swept surface's normal at a section point, from its derivatives, facing the profile's outside. */
	FVector SweepNormal(FPlace Place, double T, const FVector2D& Q, const FVector2D& Tangent)
	{
		const FVector Along = Place(T + DiffStep, Q) - Place(T - DiffStep, Q);
		const FVector Across = Place(T, Q + Tangent * DiffStep) - Place(T, Q - Tangent * DiffStep);
		const FVector Hint = Place(T, Q + Outward(Tangent) * DiffStep) - Place(T, Q);
		const FVector N = FVector::CrossProduct(Along, Across);
		if (N.SizeSquared() < 1e-30) { return Hint.GetSafeNormal(1e-40); }   // on the axis (a pole)
		return FVector::DotProduct(N, Hint) < 0 ? -N.GetSafeNormal(1e-40) : N.GetSafeNormal(1e-40);
	}

	/**
	 * Sweeps a section along a path (parameters Ts): smooth along the path and round the section's arcs,
	 * creased between its straight runs. U runs along the path, V round the section, both in metres.
	 */
	void Sweep(FMuseeMesh& Out, const FProfile& Profile, const TArray<double>& Ts, FPlace Place)
	{
		double V0 = 0;
		for (const FProfile::FPiece& Piece : Profile.Pieces)
		{
			const double V1 = V0 + FVector2D::Distance(Piece.A, Piece.B);
			FVector PA = Place(Ts[0], Piece.A), PB = Place(Ts[0], Piece.B);
			FVector NA = SweepNormal(Place, Ts[0], Piece.A, Piece.TangentA), NB = SweepNormal(Place, Ts[0], Piece.B, Piece.TangentB);
			double UA = 0, UB = 0;
			for (int32 i = 1; i < Ts.Num(); ++i)
			{
				const FVector PA1 = Place(Ts[i], Piece.A), PB1 = Place(Ts[i], Piece.B);
				const FVector NA1 = SweepNormal(Place, Ts[i], Piece.A, Piece.TangentA), NB1 = SweepNormal(Place, Ts[i], Piece.B, Piece.TangentB);
				const double UA1 = UA + FVector::Dist(PA, PA1), UB1 = UB + FVector::Dist(PB, PB1);
				AddQuad(Out, PA, PA1, PB1, PB, NA, NA1, NB1, NB, FVector2D(UA, V0), FVector2D(UA1, V0), FVector2D(UB1, V1), FVector2D(UB, V1));
				PA = PA1; PB = PB1; NA = NA1; NB = NB1; UA = UA1; UB = UB1;
			}
			V0 = V1;
		}
	}

	/** Ear-clips a simple polygon into index triples. */
	void Triangulate(const TArray<FVector2D>& Poly, TArray<int32>& OutIndices)
	{
		TArray<int32> Left;
		double Area = 0;
		for (int32 i = 0; i < Poly.Num(); ++i)
		{
			Left.Add(i);
			Area += FVector2D::CrossProduct(Poly[i], Poly[(i + 1) % Poly.Num()]);
		}
		const double Turn = Area > 0 ? 1.0 : -1.0;
		auto Turning = [&Poly, Turn](int32 P0, int32 P1, int32 P2) { return FVector2D::CrossProduct(Poly[P1] - Poly[P0], Poly[P2] - Poly[P0]) * Turn; };
		while (Left.Num() > 3)
		{
			bool bClipped = false;
			for (int32 i = 0; i < Left.Num() && !bClipped; ++i)
			{
				const int32 A = Left[(i + Left.Num() - 1) % Left.Num()], B = Left[i], C = Left[(i + 1) % Left.Num()];
				if (Turning(A, B, C) <= 1e-14) { continue; }   // a reflex corner
				bool bBlocked = false;
				for (const int32 Other : Left)
				{
					if (Other != A && Other != B && Other != C && Turning(A, B, Other) >= 0 && Turning(B, C, Other) >= 0 && Turning(C, A, Other) >= 0)
					{
						bBlocked = true;
						break;
					}
				}
				if (!bBlocked)
				{
					OutIndices.Append({A, B, C});
					Left.RemoveAt(i);
					bClipped = true;
				}
			}
			if (!bClipped) { return; }
		}
		OutIndices.Append({Left[0], Left[1], Left[2]});
	}

	/** Closes the end of a swept moulding at path parameter T, facing along the path or back. */
	void Cap(FMuseeMesh& Out, const FProfile& Profile, double T, bool bForward, FPlace Place)
	{
		const TArray<FVector2D> Poly = Profile.Outline();
		TArray<int32> Indices;
		Triangulate(Poly, Indices);
		FVector2D Mid = FVector2D::ZeroVector;
		for (const FVector2D& Point : Poly) { Mid += Point / static_cast<double>(Poly.Num()); }
		const FVector Along = (Place(T + DiffStep, Mid) - Place(T - DiffStep, Mid)).GetSafeNormal(1e-40);
		const FVector N = bForward ? Along : -Along;
		for (int32 i = 0; i + 2 < Indices.Num(); i += 3)
		{
			const FVector2D& A = Poly[Indices[i]];
			const FVector2D& B = Poly[Indices[i + 1]];
			const FVector2D& C = Poly[Indices[i + 2]];
			AddTri(Out, Place(T, A), Place(T, B), Place(T, C), N, N, N, A, B, C);
		}
	}

	/** Tangents along each triangle's U direction (for normal maps); the mesh has three vertices per triangle. */
	void ComputeTangents(FMuseeMesh& Mesh3)
	{
		for (int32 i = 0; i + 2 < Mesh3.Vertices.Num(); i += 3)
		{
			const FVector E1 = Mesh3.Vertices[i + 1] - Mesh3.Vertices[i], E2 = Mesh3.Vertices[i + 2] - Mesh3.Vertices[i];
			const FVector2D D1 = Mesh3.UVs[i + 1] - Mesh3.UVs[i], D2 = Mesh3.UVs[i + 2] - Mesh3.UVs[i];
			const double Det = D1.X * D2.Y - D2.X * D1.Y;
			FVector TangentU = E1, TangentV = E2;
			if (FMath::Abs(Det) > 1e-12)
			{
				TangentU = (E1 * D2.Y - E2 * D1.Y) / Det;
				TangentV = (E2 * D1.X - E1 * D2.X) / Det;
			}
			for (int32 k = i; k < i + 3; ++k)
			{
				const FVector& N = Mesh3.Normals[k];
				FVector T = TangentU - N * FVector::DotProduct(N, TangentU);
				if (T.SizeSquared() < 1e-12) { T = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector); }
				T = T.GetSafeNormal(1e-30);
				Mesh3.Tangents[k] = FProcMeshTangent(T, FVector::DotProduct(FVector::CrossProduct(N, T), TangentV) < 0);
			}
		}
	}

	/** Along an axis from the Rotunda's centre, the distance to a circle of this radius at this lateral offset. */
	double AlongAt(double Radius, double Lateral) { return FMath::Sqrt(Radius * Radius - Lateral * Lateral); }

	/** A frame on a plan angle: U out along it, D across it (towards increasing angle). */
	struct FFrame
	{
		FVector U;
		FVector D;

		explicit FFrame(double Angle) : U(FMath::Cos(Angle), FMath::Sin(Angle), 0.0), D(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0) {}

		FVector At(double Along, double Lateral, double Height) const { return U * Along + D * Lateral + FVector(0.0, 0.0, Height); }
	};

	/** A door (through the wall) or a niche (a recess), round-arched, on its axis. */
	struct FOpening
	{
		double Axis;        // plan angle
		double HalfWidth;
		double Spring;
		double Sill;        // 0 for the doors
		bool bDoor;
		double RevealEnd;   // doors: along the axis to the passage's end, or 0 for the drum's outer face; niches: the recess's plane

		FFrame Frame() const { return FFrame(Axis); }
		/** Lateral offset and height of the opening's edge at arch station K (0: left springing, ArchSegments: right). */
		double Across(int32 K) const { return -HalfWidth * FMath::Cos(UE_DOUBLE_PI * K / ArchSegments); }
		double Head(int32 K) const { return Spring + HalfWidth * FMath::Sin(UE_DOUBLE_PI * K / ArchSegments); }
		double EndAt(double Lateral) const { return RevealEnd > 0 ? RevealEnd : AlongAt(OuterRadius, Lateral); }
	};

	/** The eight openings in order round the drum, from the east door. */
	TArray<FOpening> MakeOpenings()
	{
		auto Door = [](double Degrees, double Width, double End)
		{
			return FOpening{FMath::DegreesToRadians(Degrees), Width / 2, R::DoorSpring, 0.0, true, End};
		};
		auto Niche = [](double Degrees)
		{
			return FOpening{FMath::DegreesToRadians(Degrees), R::NicheWidth / 2, R::NicheSpring, R::NicheSill, false, R::Radius};
		};
		// Chenghuai (when Chenghuai::bNorthDoorOpen): the north door opens onto Chenghuai's open porch (the Sculpture Hall's
		// passage is retired), so its reveal ends at the drum's outer face, as the south door's does.
		return {Door(0, R::DoorWidthWestEast, 0), Niche(45), Door(90, R::DoorWidthNorthSouth, 0), Niche(135),
				Door(180, R::DoorWidthWestEast, R::WestPassageEnd), Niche(225), Door(270, R::DoorWidthNorthSouth, Chenghuai::bNorthDoorOpen ? 0.0 : R::NorthPassageEnd), Niche(315)};
	}

	/** A vertical line of a drum face: where the face's strips meet, dense across the openings. */
	struct FStation
	{
		FVector2D P;        // on the face
		FVector2D Recess;   // on the back of the shadow gap (inner face)
		double Phi;         // plan angle, increasing round the drum
		int32 Opening;      // the opening whose edge this station is on, or INDEX_NONE
		int32 K;            // its arch station
	};

	/**
	 * The stations of a drum face: across each opening at its arch stations (so the face, the reveal and the
	 * mouldings share their vertices), and at most WallStep apart between them.
	 */
	TArray<FStation> MakeStations(const TArray<FOpening>& Openings, double Radius, bool bDoorsOnly)
	{
		TArray<int32> Cut;
		for (int32 i = 0; i < Openings.Num(); ++i)
		{
			if (Openings[i].bDoor || !bDoorsOnly) { Cut.Add(i); }
		}
		auto InPlan = [](const FVector& V) { return FVector2D(V.X, V.Y); };
		TArray<FStation> Ring;
		for (int32 c = 0; c < Cut.Num(); ++c)
		{
			const FOpening& O = Openings[Cut[c]];
			const FFrame F = O.Frame();
			for (int32 k = 0; k <= ArchSegments; ++k)
			{
				const double Lateral = O.Across(k);
				Ring.Add({InPlan(F.At(AlongAt(Radius, Lateral), Lateral, 0)), InPlan(F.At(AlongAt(GapRadius, Lateral), Lateral, 0)),
						  O.Axis + FMath::Asin(Lateral / Radius), Cut[c], k});
			}
			const FOpening& Next = Openings[Cut[(c + 1) % Cut.Num()]];
			const double From = Ring.Last().Phi;
			double To = Next.Axis + FMath::Asin(Next.Across(0) / Radius);
			while (To <= From) { To += 2 * UE_DOUBLE_PI; }
			const int32 Count = FMath::Max(1, FMath::CeilToInt((To - From) / WallStep));
			for (int32 i = 1; i < Count; ++i)
			{
				const double Phi = From + (To - From) * i / Count;
				const FVector2D Dir(FMath::Cos(Phi), FMath::Sin(Phi));
				Ring.Add({Dir * Radius, Dir * GapRadius, Phi, INDEX_NONE, 0});
			}
		}
		return Ring;
	}

	/** The heights in a list between a span's bottom and top (inclusive). */
	TArray<double> Within(const TArray<double>& Heights, const FVector2D& Span)
	{
		TArray<double> Result;
		for (const double Z : Heights)
		{
			if (Z >= Span.X - 1e-9 && Z <= Span.Y + 1e-9) { Result.Add(Z); }
		}
		return Result;
	}

	/** Triangulates a vertical strip between two stations whose edges carry different break heights (no T-junctions). */
	void Zip(FMuseeMesh& Out, const TArray<double>& ZA, const TArray<double>& ZB, const FVector2D& PA, const FVector2D& PB,
			 const FVector& NA, const FVector& NB, double SA, double SB)
	{
		auto At3 = [](const FVector2D& P, double Z) { return FVector(P.X, P.Y, Z); };
		int32 i = 0, j = 0;
		while (i + 1 < ZA.Num() || j + 1 < ZB.Num())
		{
			if (j + 1 >= ZB.Num() || (i + 1 < ZA.Num() && ZA[i + 1] <= ZB[j + 1]))
			{
				AddTri(Out, At3(PA, ZA[i]), At3(PA, ZA[i + 1]), At3(PB, ZB[j]), NA, NA, NB, FVector2D(SA, -ZA[i]), FVector2D(SA, -ZA[i + 1]), FVector2D(SB, -ZB[j]));
				++i;
			}
			else
			{
				AddTri(Out, At3(PA, ZA[i]), At3(PB, ZB[j + 1]), At3(PB, ZB[j]), NA, NB, NB, FVector2D(SA, -ZA[i]), FVector2D(SB, -ZB[j + 1]), FVector2D(SB, -ZB[j]));
				++j;
			}
		}
	}

	/**
	 * One face of the drum, from the floor (inner face: from the shadow gap) to Top, cut round the openings.
	 * The inner face also gets the shadow gap's back and soffit wherever it reaches the floor.
	 */
	void WallFace(FMuseeMesh& Out, const TArray<FOpening>& Openings, const TArray<FStation>& Ring, double Radius, double Top, bool bInner)
	{
		const double Base = bInner ? GapTop : 0.0;
		const double Facing = bInner ? -1.0 : 1.0;
		auto Breaks = [&Openings, Base, Top](const FStation& S)
		{
			TArray<double> Heights = {Base};
			if (S.Opening != INDEX_NONE)
			{
				const FOpening& O = Openings[S.Opening];
				if (O.Sill > 0) { Heights.Add(O.Sill); }
				Heights.Add(O.Head(S.K));
			}
			Heights.Add(Top);
			return Heights;
		};
		auto At3 = [](const FVector2D& P, double Z) { return FVector(P.X, P.Y, Z); };
		for (int32 i = 0; i < Ring.Num(); ++i)
		{
			const FStation& A = Ring[i];
			const FStation& B = Ring[(i + 1) % Ring.Num()];
			const double SA = A.Phi * Radius;
			const double SB = (i + 1 < Ring.Num() ? B.Phi : B.Phi + 2 * UE_DOUBLE_PI) * Radius;
			const FVector NA = FVector(A.P.X, A.P.Y, 0.0).GetSafeNormal() * Facing;
			const FVector NB = FVector(B.P.X, B.P.Y, 0.0).GetSafeNormal() * Facing;
			// The solid spans (bottom, top) at each end: all of it, or below the sill and above the head.
			TArray<FVector2D> SpansA, SpansB;
			bool bFoot = true;
			if (A.Opening != INDEX_NONE && A.Opening == B.Opening)
			{
				const FOpening& O = Openings[A.Opening];
				bFoot = O.Sill > 0;
				if (bFoot)
				{
					SpansA.Add(FVector2D(Base, O.Sill));
					SpansB.Add(FVector2D(Base, O.Sill));
				}
				SpansA.Add(FVector2D(O.Head(A.K), Top));
				SpansB.Add(FVector2D(O.Head(B.K), Top));
			}
			else
			{
				SpansA.Add(FVector2D(Base, Top));
				SpansB.Add(FVector2D(Base, Top));
			}
			const TArray<double> ZA = Breaks(A), ZB = Breaks(B);
			for (int32 s = 0; s < SpansA.Num(); ++s) { Zip(Out, Within(ZA, SpansA[s]), Within(ZB, SpansB[s]), A.P, B.P, NA, NB, SA, SB); }
			if (bInner && bFoot)
			{
				const FVector Down(0, 0, -1);
				AddQuad(Out, At3(A.Recess, 0), At3(B.Recess, 0), At3(B.Recess, GapTop), At3(A.Recess, GapTop), NA, NB, NB, NA,
						FVector2D(SA, 0), FVector2D(SB, 0), FVector2D(SB, -GapTop), FVector2D(SA, -GapTop));
				AddQuad(Out, At3(A.P, GapTop), At3(B.P, GapTop), At3(B.Recess, GapTop), At3(A.Recess, GapTop), Down, Down, Down, Down,
						FVector2D(SA, 0), FVector2D(SB, 0), FVector2D(SB, 0.04), FVector2D(SA, 0.04));
			}
		}
	}

	/**
	 * An opening's reveal: the arched soffit and the jambs, from the drum's inner face to its end (the outer
	 * face, the passage's end, or a niche's recess). A door's jambs step back into the shadow gap at the floor.
	 */
	void Reveal(FMuseeMesh& Out, const FOpening& O)
	{
		const FFrame F = O.Frame();
		auto Inner = [](double Lateral) { return AlongAt(R::Radius, Lateral); };
		double V = 0;
		for (int32 k = 0; k < ArchSegments; ++k)
		{
			const double X0 = UE_DOUBLE_PI * k / ArchSegments, X1 = UE_DOUBLE_PI * (k + 1) / ArchSegments;
			const double L0 = O.Across(k), L1 = O.Across(k + 1), Z0 = O.Head(k), Z1 = O.Head(k + 1);
			const FVector N0 = F.D * FMath::Cos(X0) - FVector::UpVector * FMath::Sin(X0);   // towards the arch's centre
			const FVector N1 = F.D * FMath::Cos(X1) - FVector::UpVector * FMath::Sin(X1);
			const double V1 = V + O.HalfWidth * (X1 - X0);
			AddQuad(Out, F.At(Inner(L0), L0, Z0), F.At(Inner(L1), L1, Z1), F.At(O.EndAt(L1), L1, Z1), F.At(O.EndAt(L0), L0, Z0), N0, N1, N1, N0,
					FVector2D(Inner(L0), V), FVector2D(Inner(L1), V1), FVector2D(O.EndAt(L1), V1), FVector2D(O.EndAt(L0), V));
			V = V1;
		}
		for (const int32 Side : {-1, 1})
		{
			const double Lateral = Side * O.HalfWidth;
			const FVector N = F.D * -Side;   // facing the opening's middle
			const double U0 = Inner(Lateral), U1 = O.EndAt(Lateral);
			auto P = [&F, Lateral](double Along, double Z) { return F.At(Along, Lateral, Z); };
			auto UV = [](double Along, double Z) { return FVector2D(Along, -Z); };
			if (O.bDoor)
			{
				const double UG = AlongAt(GapRadius, Lateral);
				AddQuad(Out, P(UG, 0), P(U1, 0), P(U1, GapTop), P(UG, GapTop), N, N, N, N, UV(UG, 0), UV(U1, 0), UV(U1, GapTop), UV(UG, GapTop));
				AddTri(Out, P(U0, O.Spring), P(U0, GapTop), P(UG, GapTop), N, N, N, UV(U0, O.Spring), UV(U0, GapTop), UV(UG, GapTop));
				AddTri(Out, P(U0, O.Spring), P(UG, GapTop), P(U1, GapTop), N, N, N, UV(U0, O.Spring), UV(UG, GapTop), UV(U1, GapTop));
				AddTri(Out, P(U0, O.Spring), P(U1, GapTop), P(U1, O.Spring), N, N, N, UV(U0, O.Spring), UV(U1, GapTop), UV(U1, O.Spring));
			}
			else
			{
				AddQuad(Out, P(U0, O.Sill), P(U1, O.Sill), P(U1, O.Spring), P(U0, O.Spring), N, N, N, N,
						UV(U0, O.Sill), UV(U1, O.Sill), UV(U1, O.Spring), UV(U0, O.Spring));
			}
		}
	}

	/** A niche's recess behind its reveal: a half-round back from the sill to the springing, under a half dome. */
	void NicheRecess(FMuseeMesh& Out, const FOpening& O)
	{
		const FFrame F = O.Frame();
		const double Rn = O.HalfWidth;
		auto Angle = [](int32 K) { return UE_DOUBLE_PI * K / ArchSegments; };
		auto Back = [&F, &O, Rn, &Angle](int32 K, double Z) { return F.At(R::Radius + Rn * FMath::Sin(Angle(K)), O.Across(K), Z); };
		auto BackNormal = [&F, &Angle](int32 K) { return -F.U * FMath::Sin(Angle(K)) + F.D * FMath::Cos(Angle(K)); };
		for (int32 k = 0; k < ArchSegments; ++k)
		{
			AddQuad(Out, Back(k, O.Sill), Back(k + 1, O.Sill), Back(k + 1, O.Spring), Back(k, O.Spring),
					BackNormal(k), BackNormal(k + 1), BackNormal(k + 1), BackNormal(k),
					FVector2D(Rn * Angle(k), -O.Sill), FVector2D(Rn * Angle(k + 1), -O.Sill), FVector2D(Rn * Angle(k + 1), -O.Spring), FVector2D(Rn * Angle(k), -O.Spring));
		}
		// The half dome: its rim is the arch (J = Rings), its pole at the back (J = 0), its foot on the back's top.
		const int32 Rings = ArchSegments / 2;
		const FVector Centre = F.At(R::Radius, 0, O.Spring);
		auto Elevation = [Rings](int32 J) { return UE_DOUBLE_PI / 2 * J / Rings; };
		auto HalfDome = [&F, &O, Rn, &Angle, &Elevation](int32 K, int32 J)
		{
			const double G = Elevation(J);
			return F.At(R::Radius + Rn * FMath::Cos(G), O.Across(K) * FMath::Sin(G), O.Spring + Rn * FMath::Sin(Angle(K)) * FMath::Sin(G));
		};
		for (int32 k = 0; k < ArchSegments; ++k)
		{
			for (int32 j = 0; j < Rings; ++j)
			{
				const FVector P00 = HalfDome(k, j), P10 = HalfDome(k + 1, j), P11 = HalfDome(k + 1, j + 1), P01 = HalfDome(k, j + 1);
				AddQuad(Out, P00, P10, P11, P01, Centre - P00, Centre - P10, Centre - P11, Centre - P01,
						FVector2D(Rn * Angle(k), Rn * Elevation(j)), FVector2D(Rn * Angle(k + 1), Rn * Elevation(j)),
						FVector2D(Rn * Angle(k + 1), Rn * Elevation(j + 1)), FVector2D(Rn * Angle(k), Rn * Elevation(j + 1)));
			}
		}
	}

	/** Places a section (s out from the intrados, p proud of the wall) round an arch at angle X from the left springing. */
	FVector OnArch(const FFrame& F, const FOpening& O, double X, const FVector2D& Q)
	{
		const double Rise = O.HalfWidth + Q.X;
		const double Lateral = -Rise * FMath::Cos(X);
		return F.At(AlongAt(R::Radius, Lateral) - Q.Y, Lateral, O.Spring + Rise * FMath::Sin(X));
	}

	TArray<double> ArchAngles()
	{
		TArray<double> Ts;
		for (int32 k = 0; k <= ArchSegments; ++k) { Ts.Add(UE_DOUBLE_PI * k / ArchSegments); }
		return Ts;
	}

	/** The archivolt round a door's arch: two fasciae and an ovolo, dying into the pilasters at the springing. */
	void Archivolt(FMuseeMesh& Out, const FOpening& O)
	{
		const FProfile Section = FProfile(FVector2D(0.30, -Sink))
			.LineTo(0.30, 0.10)
			.ArcTo(FVector2D(0.27, 0.10), 0.03, 0, UE_DOUBLE_PI / 2, 4)
			.LineTo(0.20, 0.13)
			.LineTo(0.20, 0.10)
			.LineTo(0.09, 0.10)
			.LineTo(0.09, 0.08)
			.LineTo(0, 0.08)
			.LineTo(0, 0);   // the inner return continues the soffit
		const FFrame F = O.Frame();
		const TArray<double> Ts = ArchAngles();
		auto Place = [&F, &O](double X, const FVector2D& Q) { return OnArch(F, O, X, Q); };
		Sweep(Out, Section, Ts, Place);
		Cap(Out, Section, Ts[0], false, Place);
		Cap(Out, Section, Ts.Last(), true, Place);
	}

	/** Impost blocks on a door's jambs under the springing, through the drum wall. */
	void Imposts(FMuseeMesh& Out, const FOpening& O)
	{
		const FProfile Section = FProfile(FVector2D(-Sink, -0.24)).LineTo(0.02, -0.24).LineTo(0.05, -0.21).LineTo(0.05, 0).LineTo(-Sink, 0);
		const FFrame F = O.Frame();
		const TArray<double> Ts = {AlongAt(R::Radius, O.HalfWidth), AlongAt(OuterRadius, O.HalfWidth)};
		for (const int32 Side : {-1, 1})
		{
			auto Place = [&F, &O, Side](double Along, const FVector2D& Q) { return F.At(Along, Side * (O.HalfWidth - Q.X), O.Spring + Q.Y); };
			Sweep(Out, Section, Ts, Place);
			Cap(Out, Section, Ts[0], false, Place);
			Cap(Out, Section, Ts[1], true, Place);
		}
	}

	/** A niche's surround: an architrave up the jambs from the sill and round the arch. */
	void NicheSurround(FMuseeMesh& Out, const FOpening& O)
	{
		const FProfile Section = FProfile(FVector2D(0.20, -Sink))
			.LineTo(0.20, 0.05)
			.ArcTo(FVector2D(0.17, 0.05), 0.03, 0, UE_DOUBLE_PI / 2, 4)
			.LineTo(0.07, 0.08)
			.LineTo(0.07, 0.06)
			.LineTo(0, 0.06)
			.LineTo(0, 0);
		const FFrame F = O.Frame();
		// T: −1…0 up the left jamb, 0…π round the arch, π…π+1 down the right jamb.
		TArray<double> Ts = {-1.0};
		Ts.Append(ArchAngles());
		Ts.Add(UE_DOUBLE_PI + 1);
		auto Place = [&F, &O](double T, const FVector2D& Q)
		{
			const double Rise = O.Spring - O.Sill;
			if (T < 0)
			{
				const double Lateral = -O.HalfWidth - Q.X;
				return F.At(AlongAt(R::Radius, Lateral) - Q.Y, Lateral, O.Spring + Rise * T);
			}
			if (T > UE_DOUBLE_PI)
			{
				const double Lateral = O.HalfWidth + Q.X;
				return F.At(AlongAt(R::Radius, Lateral) - Q.Y, Lateral, O.Spring - Rise * (T - UE_DOUBLE_PI));
			}
			return OnArch(F, O, T, Q);
		};
		Sweep(Out, Section, Ts, Place);
		Cap(Out, Section, Ts[0], false, Place);
		Cap(Out, Section, Ts.Last(), true, Place);
	}

	/** A niche's sill: a moulded ledge under it, whose top runs back as the niche's floor. */
	void NicheLedge(FMuseeMesh& Out, const FOpening& O)
	{
		const double Proud = 0.16, Reach = O.HalfWidth + 0.26;
		const FProfile Section = FProfile(FVector2D(-Sink, O.Sill - Proud))
			.LineTo(0.13, O.Sill - Proud)
			.ArcTo(FVector2D(0.13, O.Sill - 0.13), 0.03, -UE_DOUBLE_PI / 2, 0, 4)
			.LineTo(Proud, O.Sill);
		FProfile Closed = Section;
		Closed.LineTo(-Sink, O.Sill);
		const FFrame F = O.Frame();
		TArray<double> Lats = {-Reach};
		for (int32 k = 0; k <= ArchSegments; ++k) { Lats.Add(O.Across(k)); }
		Lats.Add(Reach);
		auto Place = [&F](double Lateral, const FVector2D& Q) { return F.At(AlongAt(R::Radius, Lateral) - Q.X, Lateral, Q.Y); };
		Sweep(Out, Section, Lats, Place);
		Cap(Out, Closed, Lats[0], false, Place);
		Cap(Out, Closed, Lats.Last(), true, Place);
		// The top, from the ledge's edge back to the niche's half-round back (beside it, into the wall).
		auto Back = [&](int32 Index)
		{
			if (Index == 0 || Index == Lats.Num() - 1) { return F.At(AlongAt(R::Radius, Lats[Index]) + Sink, Lats[Index], O.Sill); }
			const int32 K = Index - 1;
			return F.At(R::Radius + O.HalfWidth * FMath::Sin(UE_DOUBLE_PI * K / ArchSegments), O.Across(K), O.Sill);
		};
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i + 1 < Lats.Num(); ++i)
		{
			const FVector A = Place(Lats[i], FVector2D(Proud, O.Sill)), B = Place(Lats[i + 1], FVector2D(Proud, O.Sill));
			const FVector C = Back(i + 1), D = Back(i);
			auto UV = [&F](const FVector& P) { return FVector2D(FVector::DotProduct(P, F.U), FVector::DotProduct(P, F.D)); };
			AddQuad(Out, A, B, C, D, Up, Up, Up, Up, UV(A), UV(B), UV(C), UV(D));
		}
	}

	/** Closes a passage's tunnel (the door's reveal run on) in a solid: side walls and a flat roof, 0.3 m thick. */
	void PassageShell(FMuseeMesh& Out, const FOpening& O)
	{
		const FFrame F = O.Frame();
		const double Half = O.HalfWidth + 0.3, Roof = O.Spring + O.HalfWidth + 0.3;
		const double U0 = AlongAt(OuterRadius, Half) - 0.05, U1 = O.RevealEnd;   // from inside the drum wall
		for (const int32 Side : {-1, 1})
		{
			const FVector N = F.D * Side;
			AddQuad(Out, F.At(U0, Side * Half, 0), F.At(U1, Side * Half, 0), F.At(U1, Side * Half, Roof), F.At(U0, Side * Half, Roof), N, N, N, N,
					FVector2D(U0, 0), FVector2D(U1, 0), FVector2D(U1, -Roof), FVector2D(U0, -Roof));
		}
		const FVector Up(0, 0, 1);
		AddQuad(Out, F.At(U0, -Half, Roof), F.At(U1, -Half, Roof), F.At(U1, Half, Roof), F.At(U0, Half, Roof), Up, Up, Up, Up,
				FVector2D(U0, -Half), FVector2D(U1, -Half), FVector2D(U1, Half), FVector2D(U0, Half));
	}

	/** One vertical column across a pilaster's face: a flat fillet or a slice of a flute (depth into the shaft, and its slope). */
	struct FFluteColumn
	{
		double Lat0, Lat1;
		double Depth0, Depth1;
		double Slope0, Slope1;
	};

	/** Seven flutes 85 mm wide and 30 mm deep, between fillets, with plain margins at the arrises. */
	TArray<FFluteColumn> FluteColumns()
	{
		const double Margin = 0.07, Width = 0.085, Fillet = 0.0275, Depth = 0.03;
		const int32 Flutes = 7, Steps = 6;
		TArray<FFluteColumn> Columns;
		double Lateral = -PilasterHalf + Margin;
		Columns.Add({-PilasterHalf, Lateral, 0, 0, 0, 0});
		for (int32 f = 0; f < Flutes; ++f)
		{
			for (int32 s = 0; s < Steps; ++s)
			{
				const double A0 = UE_DOUBLE_PI * s / Steps, A1 = UE_DOUBLE_PI * (s + 1) / Steps;
				const double Slope = Depth * UE_DOUBLE_PI / Width;
				Columns.Add({Lateral + Width * s / Steps, Lateral + Width * (s + 1) / Steps,
							 Depth * FMath::Sin(A0), Depth * FMath::Sin(A1), Slope * FMath::Cos(A0), Slope * FMath::Cos(A1)});
			}
			Lateral += Width;
			if (f + 1 < Flutes)
			{
				Columns.Add({Lateral, Lateral + Fillet, 0, 0, 0, 0});
				Lateral += Fillet;
			}
		}
		Columns.Add({Lateral, PilasterHalf, 0, 0, 0, 0});
		return Columns;
	}

	/** The pilaster's base, as offsets out from the shaft's faces: plinth, torus, fillet and apophyge. */
	FProfile BaseProfile()
	{
		return FProfile(FVector2D(0.10, 0))
			.LineTo(0.10, 0.18)
			.LineTo(0.03, 0.18)
			.ArcTo(FVector2D(0.03, 0.24), 0.06, -UE_DOUBLE_PI / 2, UE_DOUBLE_PI / 2, 10)
			.LineTo(0.02, 0.30)
			.ArcTo(FVector2D(0.02, ShaftBottom), 0.02, 1.5 * UE_DOUBLE_PI, UE_DOUBLE_PI, 4);
	}

	/** The pilaster's capital: astragal, necking, annulet, echinus and abacus. */
	FProfile CapitalProfile()
	{
		return FProfile(FVector2D(0, ShaftTop))
			.ArcTo(FVector2D(0, 8.93), 0.03, -UE_DOUBLE_PI / 2, UE_DOUBLE_PI / 2, 6)
			.LineTo(0, 9.14)
			.LineTo(0.025, 9.14)
			.LineTo(0.025, 9.17)
			.ArcTo(FVector2D(0.025, 9.26), 0.09, -UE_DOUBLE_PI / 2, 0, 8)
			.LineTo(0.115, R::PilasterHeight);
	}

	/** One fluted pilaster on a plan angle, its back sunk into the wall. */
	void Pilaster(FMuseeMesh& Out, double Angle)
	{
		const FFrame F(Angle);
		const TArray<FFluteColumn> Columns = FluteColumns();
		// The base and capital's faces break where the flutes do, so the shaft meets them vertex for vertex.
		TArray<double> FrontTs = {0.0};
		for (const FFluteColumn& Column : Columns) { FrontTs.Add((Column.Lat1 + PilasterHalf) / (2 * PilasterHalf)); }
		const TArray<double> SideTs = {0.0, 1.0};
		auto LeftPlace = [&F](double T, const FVector2D& Q) { return F.At(FMath::Lerp(PilasterBack, PilasterFront - Q.X, T), -(PilasterHalf + Q.X), Q.Y); };
		auto FrontPlace = [&F](double T, const FVector2D& Q) { return F.At(PilasterFront - Q.X, FMath::Lerp(-(PilasterHalf + Q.X), PilasterHalf + Q.X, T), Q.Y); };
		auto RightPlace = [&F](double T, const FVector2D& Q) { return F.At(FMath::Lerp(PilasterFront - Q.X, PilasterBack, T), PilasterHalf + Q.X, Q.Y); };
		for (const FProfile& Section : {BaseProfile(), CapitalProfile()})
		{
			Sweep(Out, Section, SideTs, LeftPlace);
			Sweep(Out, Section, FrontTs, FrontPlace);
			Sweep(Out, Section, SideTs, RightPlace);
		}
		// The abacus's top, in strips matching its face.
		const FVector2D Abacus = CapitalProfile().Cursor;
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i + 1 < FrontTs.Num(); ++i)
		{
			const FVector A = FrontPlace(FrontTs[i], Abacus), B = FrontPlace(FrontTs[i + 1], Abacus);
			const FVector C = F.At(PilasterBack, FVector::DotProduct(B, F.D), Abacus.Y), D = F.At(PilasterBack, FVector::DotProduct(A, F.D), Abacus.Y);
			AddQuad(Out, A, B, C, D, Up, Up, Up, Up, FVector2D(0, FVector::DotProduct(A, F.D)), FVector2D(0, FVector::DotProduct(B, F.D)),
					FVector2D(0.5, FVector::DotProduct(B, F.D)), FVector2D(0.5, FVector::DotProduct(A, F.D)));
		}
		// The shaft's sides, broken at the flutes' ends to meet the face's columns.
		const double Heights[] = {ShaftBottom, FluteBottom, FluteTop, ShaftTop};
		for (const int32 Side : {-1, 1})
		{
			const FVector N = F.D * Side;
			for (int32 h = 0; h < 3; ++h)
			{
				const double Z0 = Heights[h], Z1 = Heights[h + 1];
				AddQuad(Out, F.At(PilasterBack, Side * PilasterHalf, Z0), F.At(PilasterFront, Side * PilasterHalf, Z0),
						F.At(PilasterFront, Side * PilasterHalf, Z1), F.At(PilasterBack, Side * PilasterHalf, Z1), N, N, N, N,
						FVector2D(PilasterBack, -Z0), FVector2D(PilasterFront, -Z0), FVector2D(PilasterFront, -Z1), FVector2D(PilasterBack, -Z1));
			}
		}
		// The shaft's face: plain at the foot and the neck, fluted between; the flutes stop square.
		auto Face = [&F](double Lateral, double Depth, double Z) { return F.At(PilasterFront + Depth, Lateral, Z); };
		const FVector Flat = -F.U;
		const FVector Down(0, 0, -1);
		for (const FFluteColumn& C : Columns)
		{
			const FVector N0 = (-F.U + F.D * C.Slope0).GetSafeNormal(), N1 = (-F.U + F.D * C.Slope1).GetSafeNormal();
			auto UV = [](double Lateral, double Z) { return FVector2D(Lateral, -Z); };
			AddQuad(Out, Face(C.Lat0, 0, ShaftBottom), Face(C.Lat1, 0, ShaftBottom), Face(C.Lat1, 0, FluteBottom), Face(C.Lat0, 0, FluteBottom),
					Flat, Flat, Flat, Flat, UV(C.Lat0, ShaftBottom), UV(C.Lat1, ShaftBottom), UV(C.Lat1, FluteBottom), UV(C.Lat0, FluteBottom));
			AddQuad(Out, Face(C.Lat0, C.Depth0, FluteBottom), Face(C.Lat1, C.Depth1, FluteBottom), Face(C.Lat1, C.Depth1, FluteTop), Face(C.Lat0, C.Depth0, FluteTop),
					N0, N1, N1, N0, UV(C.Lat0, FluteBottom), UV(C.Lat1, FluteBottom), UV(C.Lat1, FluteTop), UV(C.Lat0, FluteTop));
			AddQuad(Out, Face(C.Lat0, 0, FluteTop), Face(C.Lat1, 0, FluteTop), Face(C.Lat1, 0, ShaftTop), Face(C.Lat0, 0, ShaftTop),
					Flat, Flat, Flat, Flat, UV(C.Lat0, FluteTop), UV(C.Lat1, FluteTop), UV(C.Lat1, ShaftTop), UV(C.Lat0, ShaftTop));
			AddQuad(Out, Face(C.Lat0, 0, FluteBottom), Face(C.Lat1, 0, FluteBottom), Face(C.Lat1, C.Depth1, FluteBottom), Face(C.Lat0, C.Depth0, FluteBottom),
					Up, Up, Up, Up, FVector2D(C.Lat0, 0), FVector2D(C.Lat1, 0), FVector2D(C.Lat1, C.Depth1), FVector2D(C.Lat0, C.Depth0));
			AddQuad(Out, Face(C.Lat0, 0, FluteTop), Face(C.Lat1, 0, FluteTop), Face(C.Lat1, C.Depth1, FluteTop), Face(C.Lat0, C.Depth0, FluteTop),
					Down, Down, Down, Down, FVector2D(C.Lat0, 0), FVector2D(C.Lat1, 0), FVector2D(C.Lat1, C.Depth1), FVector2D(C.Lat0, C.Depth0));
		}
	}

	/**
	 * How far the pilasters beside a door stand from its axis. The plan puts them at 11.25°, which for the
	 * 4 m doors (and by a few centimetres for the 3 m ones) would stand their bases and capitals in the
	 * opening; they step out just enough that their widest part clears the jamb.
	 */
	double DoorFlank(const FOpening& O)
	{
		const double Reach = FMath::Sqrt(PilasterBack * PilasterBack + PilasterReach * PilasterReach);
		return FMath::Max(FMath::DegreesToRadians(11.25), FMath::Asin(O.HalfWidth / Reach) + FMath::Atan2(PilasterReach, PilasterBack));
	}

	/** Pilaster K's plan angle, 11.25° + K·22.5°, stepped clear of a door beside it. */
	double PilasterAngle(int32 K, const TArray<FOpening>& Openings)
	{
		const double Planned = FMath::DegreesToRadians(11.25 + 22.5 * K);
		for (const FOpening& O : Openings)
		{
			const double Offset = FMath::UnwindRadians(Planned - O.Axis);
			const double Flank = DoorFlank(O);
			if (O.bDoor && FMath::Abs(Offset) < Flank) { return O.Axis + (Offset < 0 ? -Flank : Flank); }
		}
		return Planned;
	}

	/** A point on the dome's inner surface (radius R, centred on the springing) at an elevation, in (radius, height). */
	FVector2D OnSphere(double Radius, double Elevation) { return FVector2D(0, R::DrumHeight) + FVector2D(FMath::Cos(Elevation), FMath::Sin(Elevation)) * Radius; }

	/** A point on the dome, pushed Depth into it (for the coffers); computed as FRevolve does, so rings match. */
	FVector DomePoint(double Theta, double Elevation, double Depth)
	{
		const double Radius = R::Radius + Depth;
		const double Ring = FMath::Cos(Elevation) * Radius;
		return FVector(Ring * FMath::Cos(Theta), Ring * FMath::Sin(Theta), R::DrumHeight + FMath::Sin(Elevation) * Radius);
	}

	double CellAngle(int32 Cell) { return 2 * UE_DOUBLE_PI * Cell / R::CoffersPerRing; }

	/** The 280 angles round the dome (10 per coffer, then back to the start), shared by every ring so they meet. */
	TArray<double> DomeAngles()
	{
		TArray<double> Ts;
		for (int32 c = 0; c < R::CoffersPerRing; ++c)
		{
			for (int32 i = 0; i < CofferSamples; ++i) { Ts.Add(FMath::Lerp(CellAngle(c), CellAngle(c + 1), static_cast<double>(i) / CofferSamples)); }
		}
		Ts.Add(CellAngle(R::CoffersPerRing));
		return Ts;
	}

	/** The coffer rings' bounding elevations, 4° to 64° in five equal rings. */
	TArray<double> CofferElevations()
	{
		TArray<double> Elevations;
		for (int32 j = 0; j <= R::CofferRings; ++j)
		{
			Elevations.Add(FMath::DegreesToRadians(R::CofferFromDegrees + (R::CofferToDegrees - R::CofferFromDegrees) * j / R::CofferRings));
		}
		return Elevations;
	}

	double CorniceElevation() { return FMath::Asin((CorniceTop - R::DrumHeight) / R::Radius); }

	/**
	 * The entablature, from the dome's first ring out and down to the wall over the capitals: sima, corona and
	 * bed moulding (the cornice), the frieze, and the architrave's crown and two fasciae.
	 */
	FProfile EntablatureProfile()
	{
		return FProfile(OnSphere(R::Radius, CorniceElevation()))
			.LineTo(9.22, CorniceTop)
			.LineTo(9.22, 10.46)
			.ArcTo(FVector2D(9.22, 10.40), 0.06, UE_DOUBLE_PI / 2, 0, 6)
			.LineTo(9.28, 10.24)
			.LineTo(9.42, 10.24)
			.LineTo(9.42, 10.21)
			.ArcTo(FVector2D(9.51, 10.21), 0.09, UE_DOUBLE_PI, 1.5 * UE_DOUBLE_PI, 8)
			.LineTo(9.51, 9.84)
			.LineTo(9.43, 9.84)
			.LineTo(9.43, 9.82)
			.ArcTo(FVector2D(9.48, 9.82), 0.05, UE_DOUBLE_PI, 1.5 * UE_DOUBLE_PI, 6)
			.LineTo(9.48, 9.63)
			.LineTo(9.50, 9.63)
			.LineTo(9.50, R::PilasterHeight)
			.LineTo(R::Radius + Sink, R::PilasterHeight);
	}

	/** The dome's plain bands: from the cornice to the coffers, and from the coffers up to the eye's stepped curb. */
	void DomeInside(FMuseeMesh& Out, const TArray<double>& Ts, const TArray<double>& Elevations)
	{
		const FVector2D Centre(0, R::DrumHeight);
		const FProfile Lower = FProfile(OnSphere(R::Radius, Elevations[0])).ArcTo(Centre, R::Radius, Elevations[0], CorniceElevation(), 2);
		const double EyeElevation = FMath::Acos(EyeRim / R::Radius);
		const FProfile Upper = FProfile(FVector2D(R::OculusRadius, CurbTop))
			.LineTo(R::OculusRadius, 19.58)
			.LineTo(2.75, 19.58)
			.LineTo(2.75, 19.42)
			.LineTo(EyeRim, 19.42)
			.LineTo(OnSphere(R::Radius, EyeElevation))
			.ArcTo(Centre, R::Radius, EyeElevation, Elevations.Last(), 8);
		Sweep(Out, Lower, Ts, FRevolve());
		Sweep(Out, Upper, Ts, FRevolve());
	}

	/**
	 * 5 rings × 28 coffers, tiling the dome from 4° to 64°: each a rib frame on the dome's surface, two steps
	 * and a bevel down to a panel 0.45 m deep. Ribs and steps scale with the cell (narrower up the dome).
	 */
	void Coffers(FMuseeMesh& Out, const TArray<double>& Elevations)
	{
		const FVector Centre(0, 0, R::DrumHeight);
		TArray<double> SideTs;
		for (int32 i = 0; i <= CofferSamples; ++i) { SideTs.Add(static_cast<double>(i) / CofferSamples); }
		for (int32 j = 0; j < R::CofferRings; ++j)
		{
			const double E0 = Elevations[j], E1 = Elevations[j + 1];
			const double PerTheta = 1 / (R::Radius * FMath::Cos((E0 + E1) / 2)), PerElevation = 1 / R::Radius;
			const double Cell = (CellAngle(1) - CellAngle(0)) / PerTheta;
			const double Rib = 0.11 * Cell, Step = 0.045 * Cell, Bevel = 0.05 * Cell;
			const FProfile Section = FProfile(FVector2D(0, 0))
				.LineTo(Rib, 0)
				.LineTo(Rib, 0.12)
				.LineTo(Rib + Step, 0.12)
				.LineTo(Rib + Step, 0.26)
				.LineTo(Rib + 2 * Step, 0.26)
				.LineTo(Rib + 2 * Step + Bevel, R::CofferDepth);
			const FVector2D Panel = Section.Cursor;
			for (int32 c = 0; c < R::CoffersPerRing; ++c)
			{
				const double T0 = CellAngle(c), T1 = CellAngle(c + 1);
				// The four sides run the same way as the neighbouring cells' and the plain bands' rings, so they share vertices.
				auto Bottom = [T0, T1, E0, PerTheta, PerElevation](double S, const FVector2D& Q)
				{
					return DomePoint(FMath::Lerp(T0 + Q.X * PerTheta, T1 - Q.X * PerTheta, S), E0 + Q.X * PerElevation, Q.Y);
				};
				auto Top = [T0, T1, E1, PerTheta, PerElevation](double S, const FVector2D& Q)
				{
					return DomePoint(FMath::Lerp(T0 + Q.X * PerTheta, T1 - Q.X * PerTheta, S), E1 - Q.X * PerElevation, Q.Y);
				};
				auto Left = [T0, E0, E1, PerTheta, PerElevation](double S, const FVector2D& Q)
				{
					return DomePoint(T0 + Q.X * PerTheta, FMath::Lerp(E0 + Q.X * PerElevation, E1 - Q.X * PerElevation, S), Q.Y);
				};
				auto Right = [T1, E0, E1, PerTheta, PerElevation](double S, const FVector2D& Q)
				{
					return DomePoint(T1 - Q.X * PerTheta, FMath::Lerp(E0 + Q.X * PerElevation, E1 - Q.X * PerElevation, S), Q.Y);
				};
				Sweep(Out, Section, SideTs, Bottom);
				Sweep(Out, Section, SideTs, Top);
				Sweep(Out, Section, SideTs, Left);
				Sweep(Out, Section, SideTs, Right);
				// The panel, on the sphere 0.45 m out.
				auto PanelPoint = [&SideTs, T0, T1, E0, E1, PerTheta, PerElevation, Panel](int32 A, int32 B)
				{
					return DomePoint(FMath::Lerp(T0 + Panel.X * PerTheta, T1 - Panel.X * PerTheta, SideTs[A]),
									 FMath::Lerp(E0 + Panel.X * PerElevation, E1 - Panel.X * PerElevation, SideTs[B]), Panel.Y);
				};
				for (int32 a = 0; a < CofferSamples; ++a)
				{
					for (int32 b = 0; b < CofferSamples; ++b)
					{
						const FVector P00 = PanelPoint(a, b), P10 = PanelPoint(a + 1, b), P11 = PanelPoint(a + 1, b + 1), P01 = PanelPoint(a, b + 1);
						const double U0 = Cell * SideTs[a], U1 = Cell * SideTs[a + 1], V0 = R::Radius * (E1 - E0) * SideTs[b], V1 = R::Radius * (E1 - E0) * SideTs[b + 1];
						AddQuad(Out, P00, P10, P11, P01, Centre - P00, Centre - P10, Centre - P11, Centre - P01,
								FVector2D(U0, V0), FVector2D(U1, V0), FVector2D(U1, V1), FVector2D(U0, V1));
					}
				}
			}
		}
	}

	/** The top of the drum: a flat ring from the outer face's stations in to the shell's foot, zipped by angle. */
	void DrumTop(FMuseeMesh& Out, const TArray<FStation>& OuterRing, const TArray<double>& Ts, const FVector2D& Foot)
	{
		struct FRingPoint
		{
			double Angle;
			FVector P;
		};
		TArray<FRingPoint> Outer, Inner;
		for (const FStation& S : OuterRing) { Outer.Add({S.Phi, FVector(S.P.X, S.P.Y, Foot.Y)}); }
		const double Start = Outer[0].Angle;
		for (int32 k = 0; k + 1 < Ts.Num(); ++k)
		{
			double Angle = Ts[k];
			while (Angle < Start) { Angle += 2 * UE_DOUBLE_PI; }
			while (Angle >= Start + 2 * UE_DOUBLE_PI) { Angle -= 2 * UE_DOUBLE_PI; }
			Inner.Add({Angle, FRevolve()(Ts[k], Foot)});
		}
		Inner.Sort([](const FRingPoint& L, const FRingPoint& M) { return L.Angle < M.Angle; });
		Outer.Add({Outer[0].Angle + 2 * UE_DOUBLE_PI, Outer[0].P});
		Inner.Add({Inner[0].Angle + 2 * UE_DOUBLE_PI, Inner[0].P});
		const FVector Up(0, 0, 1);
		auto UV = [](const FVector& P) { return FVector2D(P.X, P.Y); };
		int32 i = 0, j = 0;
		while (i + 1 < Outer.Num() || j + 1 < Inner.Num())
		{
			if (j + 1 >= Inner.Num() || (i + 1 < Outer.Num() && Outer[i + 1].Angle <= Inner[j + 1].Angle))
			{
				AddTri(Out, Outer[i].P, Outer[i + 1].P, Inner[j].P, Up, Up, Up, UV(Outer[i].P), UV(Outer[i + 1].P), UV(Inner[j].P));
				++i;
			}
			else
			{
				AddTri(Out, Outer[i].P, Inner[j + 1].P, Inner[j].P, Up, Up, Up, UV(Outer[i].P), UV(Inner[j + 1].P), UV(Inner[j].P));
				++j;
			}
		}
	}

	/** The foot of the dome's shell, on the drum's top (about 11 m). */
	double ShellFoot() { return FMath::Asin(1.0 / ShellRadius); }

	/** Outside: the dome's shell from the drum's top up to a raised curb round the eye, and the drum's top. */
	void DomeOutside(FMuseeMesh& Out, const TArray<double>& Ts, const TArray<FStation>& OuterRing)
	{
		const FVector2D Foot = OnSphere(ShellRadius, ShellFoot());
		const FProfile Shell = FProfile(Foot)
			.ArcTo(FVector2D(0, R::DrumHeight), ShellRadius, ShellFoot(), FMath::Acos(CurbOuter / ShellRadius), 64)
			.LineTo(CurbOuter, CurbTop)
			.LineTo(R::OculusRadius, CurbTop);
		Sweep(Out, Shell, Ts, FRevolve());
		DrumTop(Out, OuterRing, Ts, Foot);
	}

	/** A box on three orthonormal axes, all six faces. */
	void Box(FMuseeMesh& Out, const FVector& Centre, const FVector& X, const FVector& Y, const FVector& Half)
	{
		const FVector Axes[3] = {X * Half.X, Y * Half.Y, FVector::CrossProduct(X, Y) * Half.Z};
		for (int32 a = 0; a < 3; ++a)
		{
			const FVector& P = Axes[(a + 1) % 3];
			const FVector& Q = Axes[(a + 2) % 3];
			for (const double S : {-1.0, 1.0})
			{
				const FVector C = Centre + Axes[a] * S;
				const FVector N = Axes[a].GetSafeNormal() * S;
				const double PL = P.Size(), QL = Q.Size();
				AddQuad(Out, C - P - Q, C + P - Q, C + P + Q, C - P + Q, N, N, N, N,
						FVector2D(-PL, -QL), FVector2D(PL, -QL), FVector2D(PL, QL), FVector2D(-PL, QL));
			}
		}
	}

	/**
	 * The lattice in the eye: a ring carrying the glass, and bars in three directions 60° apart, 0.8 m apart
	 * and all crossing at the centre, so the grid is triangulated with the node at its heart.
	 */
	void Lattice(FMuseeMesh& Out, const TArray<double>& Ts)
	{
		const FProfile Ring = FProfile(FVector2D(R::OculusRadius, GlassHeight)).LineTo(RingInner, GlassHeight).LineTo(RingInner, RingBottom).LineTo(R::OculusRadius, RingBottom);
		Sweep(Out, Ring, Ts, FRevolve());
		for (const double Degrees : {0.0, 60.0, 120.0})
		{
			const double A = FMath::DegreesToRadians(Degrees);
			const FVector Along(FMath::Cos(A), FMath::Sin(A), 0), Lateral(-FMath::Sin(A), FMath::Cos(A), 0);
			for (int32 k = -3; k <= 3; ++k)
			{
				const double Offset = 0.8 * k;
				const double Half = FMath::Sqrt(FMath::Square(RingInner + 0.04) - Offset * Offset);   // into the ring
				if (Half < 0.1) { continue; }
				Box(Out, Lateral * Offset + FVector(0, 0, R::LatticeHeight), Along, Lateral, FVector(Half, 0.025, 0.05));
			}
		}
	}

	/** The eye's glass: a pane 10 mm thick resting on the ring, its edge on the curb's face. */
	void OculusGlass(FMuseeMesh& Out, const TArray<double>& Ts)
	{
		for (const double Z : {GlassHeight, GlassHeight + 0.01})
		{
			const FVector N(0, 0, Z > GlassHeight ? 1 : -1);
			const FVector Centre(0, 0, Z);
			for (int32 k = 0; k + 1 < Ts.Num(); ++k)
			{
				const FVector P0 = FRevolve()(Ts[k], FVector2D(R::OculusRadius, Z)), P1 = FRevolve()(Ts[k + 1], FVector2D(R::OculusRadius, Z));
				AddTri(Out, Centre, P0, P1, N, N, N, FVector2D::ZeroVector, FVector2D(P0.X, P0.Y), FVector2D(P1.X, P1.Y));
			}
		}
	}

	/** The bronze node, a 0.12 m sphere at the lattice's centre. */
	void BronzeNode(FMuseeMesh& Out)
	{
		const FVector2D Centre(0, R::NodeHeight);
		const FProfile Meridian = FProfile(Centre - FVector2D(0, R::NodeRadius)).ArcTo(Centre, R::NodeRadius, -UE_DOUBLE_PI / 2, UE_DOUBLE_PI / 2, 24);
		TArray<double> Ts;
		for (int32 i = 0; i <= 48; ++i) { Ts.Add(2 * UE_DOUBLE_PI * i / 48); }
		Sweep(Out, Meridian, Ts, FRevolve());
	}

	/** Every mesh of the Rotunda, one per material section. */
	struct FRotundaMeshes
	{
		FMuseeMesh Stone, Pilasters, Mouldings, Plaster, Outside, Steel, Glass, Bronze;
	};

	FRotundaMeshes BuildMeshes()
	{
		FRotundaMeshes Meshes;
		const TArray<FOpening> Openings = MakeOpenings();
		const TArray<double> DomeTs = DomeAngles();
		const TArray<double> Elevations = CofferElevations();
		const TArray<FStation> InnerRing = MakeStations(Openings, R::Radius, false);
		const TArray<FStation> OuterRing = MakeStations(Openings, OuterRadius, true);

		// The drum: its faces, the reveals, niches and passages.
		WallFace(Meshes.Stone, Openings, InnerRing, R::Radius, R::DrumHeight, true);
		WallFace(Meshes.Stone, Openings, OuterRing, OuterRadius, OnSphere(ShellRadius, ShellFoot()).Y, false);
		for (const FOpening& O : Openings)
		{
			Reveal(Meshes.Stone, O);
			if (O.bDoor)
			{
				Archivolt(Meshes.Mouldings, O);
				Imposts(Meshes.Mouldings, O);
				if (O.RevealEnd > 0) { PassageShell(Meshes.Stone, O); }
			}
			else
			{
				NicheRecess(Meshes.Stone, O);
				NicheSurround(Meshes.Mouldings, O);
				NicheLedge(Meshes.Mouldings, O);
			}
		}

		// The order: pilasters and the entablature at the springing.
		for (int32 k = 0; k < R::Pilasters; ++k) { Pilaster(Meshes.Pilasters, PilasterAngle(k, Openings)); }
		Sweep(Meshes.Mouldings, EntablatureProfile(), DomeTs, FRevolve());

		// The dome, inside and out, and the eye.
		DomeInside(Meshes.Plaster, DomeTs, Elevations);
		Coffers(Meshes.Plaster, Elevations);
		DomeOutside(Meshes.Outside, DomeTs, OuterRing);
		Lattice(Meshes.Steel, DomeTs);
		OculusGlass(Meshes.Glass, DomeTs);
		BronzeNode(Meshes.Bronze);

		for (FMuseeMesh* Each : {&Meshes.Stone, &Meshes.Pilasters, &Meshes.Mouldings, &Meshes.Plaster, &Meshes.Outside, &Meshes.Steel, &Meshes.Glass, &Meshes.Bronze})
		{
			ComputeTangents(*Each);
		}
		return Meshes;
	}
}

ARotundaStructure::ARotundaStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollision)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		if (bCollision)
		{
			// The walk collides with the walls and pilasters themselves (complex as simple).
			Component->bUseComplexAsSimpleCollision = true;
			Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};
	Walls = Make(TEXT("Walls"), true);
	Mouldings = Make(TEXT("Mouldings"), true);
	Dome = Make(TEXT("Dome"), false);
	Oculus = Make(TEXT("Oculus"), false);

	auto Path = [](const TCHAR* Name) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Name, Name))); };
	WallMaterial = Path(TEXT("M_Travertine_Honed"));
	PilasterMaterial = Path(TEXT("M_Travertine_Honed"));
	MouldingMaterial = Path(TEXT("M_Plaster_Moulding"));
	DomeMaterial = Path(TEXT("M_Plaster_Coffer"));
	SteelMaterial = Path(TEXT("M_RibSteel"));
	GlassMaterial = Path(TEXT("M_Glass"));
	NodeMaterial = Path(TEXT("M_Gilt"));
}

void ARotundaStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void ARotundaStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const RotundaBuild::FRotundaMeshes Meshes = RotundaBuild::BuildMeshes();
	auto Write = [](UProceduralMeshComponent* Component, int32 Section, const FMuseeMesh& Data, bool bCollision, const TSoftObjectPtr<UMaterialInterface>& Material)
	{
		Component->CreateMeshSection(Section, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, TArray<FColor>(), Data.Tangents, bCollision);
		// Materials still being made are skipped until they exist.
		if (UMaterialInterface* Loaded = Material.LoadSynchronous()) { Component->SetMaterial(Section, Loaded); }
	};
	for (UProceduralMeshComponent* Component : {Walls.Get(), Mouldings.Get(), Dome.Get(), Oculus.Get()}) { Component->ClearAllMeshSections(); }
	Write(Walls, 0, Meshes.Stone, true, WallMaterial);
	Write(Mouldings, 0, Meshes.Pilasters, true, PilasterMaterial);
	Write(Mouldings, 1, Meshes.Mouldings, true, MouldingMaterial);
	Write(Dome, 0, Meshes.Plaster, false, DomeMaterial);
	Write(Dome, 1, Meshes.Outside, false, WallMaterial);
	Write(Oculus, 0, Meshes.Steel, false, SteelMaterial);
	Write(Oculus, 1, Meshes.Glass, false, GlassMaterial);
	Write(Oculus, 2, Meshes.Bronze, false, NodeMaterial);
}
