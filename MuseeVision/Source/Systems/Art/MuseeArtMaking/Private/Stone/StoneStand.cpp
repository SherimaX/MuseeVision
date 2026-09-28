#include "FormMesh.h"
#include "Stone/StoneMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The fewest feet a stand stands steady on. */
		constexpr int32 FewestFeet = 3;
		/** The bead's centre stands this share of the side's height up it, from the underside. */
		constexpr double BeadRiseShare = 0.4;
		/** The steps a half-round bead is drawn in, and the steps round a foot. */
		constexpr int32 BeadSteps = 8;
		constexpr int32 FootSteps = 16;
		/** A recess angle that no part of the rock reaches is filled from the angles about it; so many either side. */
		constexpr int32 SpreadAngles = 1;

		double AngleOf(int32 Index)
		{
			return 2.0 * UE_DOUBLE_PI * Index / StandAngles;
		}

		/** A closed outline, a radius at each of StandAngles angles, as rings of points about the axis. */
		struct FOutline
		{
			TArray<double> Radii;

			FVector2D At(int32 Index, double Grown) const
			{
				const int32 Wrapped = (Index % StandAngles + StandAngles) % StandAngles;
				const double Angle = AngleOf(Wrapped);
				return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Radii[Wrapped] + Grown);
			}

			/** The outward normal in plan of the outline grown by Grown, from its neighbours on either side. */
			FVector2D Normal(int32 Index, double Grown) const
			{
				const FVector2D Along = At(Index + 1, Grown) - At(Index - 1, Grown);
				return FVector2D(Along.Y, -Along.X).GetSafeNormal();
			}
		};

		/** One point of the stand's section: how far out from the recess's wall, how high, and which way its surface faces (out, up). */
		struct FSectionPoint
		{
			double Out = 0.0;
			double Up = 0.0;
			FVector2D Facing = FVector2D(1.0, 0.0);
		};

		/** The band between two points of the section, all round the outline, each facing its own way. */
		void Band(const FOutline& Outline, const FSectionPoint& A, const FSectionPoint& B, FFormMesh& Out)
		{
			TArray<int32> Lower;
			TArray<int32> Upper;
			for (int32 Index = 0; Index <= StandAngles; ++Index)
			{
				const double Around = AngleOf(Index) * FMath::Max(Outline.Radii[Index % StandAngles], UE_KINDA_SMALL_NUMBER);
				for (const FSectionPoint* Point : { &A, &B })
				{
					const FVector2D Plan = Outline.At(Index, Point->Out);
					const FVector2D Facing = Outline.Normal(Index, Point->Out) * Point->Facing.X;
					const int32 Vertex = Out.AddVertex(FVector(Plan, Point->Up), FVector(Facing, Point->Facing.Y), FVector2D(Around, Point->Up + Point->Out));
					(Point == &A ? Lower : Upper).Add(Vertex);
				}
			}
			for (int32 Index = 0; Index < StandAngles; ++Index)
			{
				Out.AddQuad(Lower[Index], Lower[Index + 1], Upper[Index + 1], Upper[Index]);
			}
		}

		/** A flat face across the outline grown by Grown at a height, facing up or down. */
		void Cap(const FOutline& Outline, double Grown, double Up, bool bFacingUp, FFormMesh& Out)
		{
			const FVector Facing(0.0, 0.0, bFacingUp ? 1.0 : -1.0);
			const int32 Centre = Out.AddVertex(FVector(0.0, 0.0, Up), Facing, FVector2D::ZeroVector);
			TArray<int32> Ring;
			for (int32 Index = 0; Index < StandAngles; ++Index)
			{
				const FVector2D Plan = Outline.At(Index, Grown);
				Ring.Add(Out.AddVertex(FVector(Plan, Up), Facing, Plan));
			}
			for (int32 Index = 0; Index < StandAngles; ++Index)
			{
				Out.AddTriangle(Centre, Ring[Index], Ring[(Index + 1) % StandAngles]);
			}
		}

		/** A short round foot standing on the ground at a point, up to the stand's underside. */
		void Foot(const FVector2D& At, double Radius, double Height, FFormMesh& Out)
		{
			TArray<int32> Lower;
			TArray<int32> Upper;
			const int32 Centre = Out.AddVertex(FVector(At, 0.0), -FVector::UpVector, At);
			TArray<int32> Sole;
			for (int32 Step = 0; Step <= FootSteps; ++Step)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * Step / FootSteps;
				const FVector2D Facing(FMath::Cos(Angle), FMath::Sin(Angle));
				const FVector2D Plan = At + Facing * Radius;
				Lower.Add(Out.AddVertex(FVector(Plan, 0.0), FVector(Facing, 0.0), FVector2D(Angle * Radius, 0.0)));
				Upper.Add(Out.AddVertex(FVector(Plan, Height), FVector(Facing, 0.0), FVector2D(Angle * Radius, Height)));
				Sole.Add(Out.AddVertex(FVector(Plan, 0.0), -FVector::UpVector, Plan));
			}
			for (int32 Step = 0; Step < FootSteps; ++Step)
			{
				Out.AddQuad(Lower[Step], Lower[Step + 1], Upper[Step + 1], Upper[Step]);
				Out.AddTriangle(Centre, Sole[Step + 1], Sole[Step]);
			}
		}

		/**
		 * The recess's wall: at each angle, the farthest the rock's surface reaches from its axis up the recess's depth, at
		 * that angle and those either side, grown by a cell of its surface and the clearance. An angle no part of the rock
		 * reaches takes the most of its neighbours'.
		 */
		FOutline RecessOf(const FFormMesh& Rock, double CellMeters, const FArtStoneStand& Stand)
		{
			TArray<double> Reach;
			Reach.Init(0.0, StandAngles);
			for (const FVector& Point : Rock.AllPositions())
			{
				if (Point.Z > Stand.RecessMeters + CellMeters)
				{
					continue;
				}
				const double Angle = FMath::Atan2(Point.Y, Point.X);
				const int32 Index = (FMath::RoundToInt(Angle / (2.0 * UE_DOUBLE_PI) * StandAngles) % StandAngles + StandAngles) % StandAngles;
				Reach[Index] = FMath::Max(Reach[Index], FVector2D(Point.X, Point.Y).Size());
			}
			FOutline Recess;
			Recess.Radii.Init(0.0, StandAngles);
			for (int32 Index = 0; Index < StandAngles; ++Index)
			{
				double Most = 0.0;
				for (int32 Near = -SpreadAngles; Near <= SpreadAngles; ++Near)
				{
					Most = FMath::Max(Most, Reach[(Index + Near + StandAngles) % StandAngles]);
				}
				Recess.Radii[Index] = Most;
			}
			// An angle between two that the rock reaches, which none of its points fell on, takes the nearer reaches' most.
			for (int32 Pass = 0; Pass < StandAngles && Recess.Radii.Contains(0.0); ++Pass)
			{
				TArray<double> Filled = Recess.Radii;
				for (int32 Index = 0; Index < StandAngles; ++Index)
				{
					if (Filled[Index] <= 0.0)
					{
						Filled[Index] = FMath::Max(Recess.Radii[(Index + 1) % StandAngles], Recess.Radii[(Index + StandAngles - 1) % StandAngles]);
					}
				}
				Recess.Radii = MoveTemp(Filled);
			}
			for (double& Radius : Recess.Radii)
			{
				Radius = Radius > 0.0 ? Radius + CellMeters + Stand.ClearanceMeters : 0.0;
			}
			return Recess;
		}
	}

	TArray<FString> StoneStandFaults(const FArtStoneStand& Stand)
	{
		TArray<FString> Out;
		if (Stand.RecessMeters <= 0.0 || Stand.FootHeightMeters <= 0.0 || Stand.HeightMeters <= Stand.RecessMeters + Stand.FootHeightMeters)
		{
			Out.Add(TEXT("its recess and its feet are more than zero deep and high, and it stands higher than both together"));
		}
		if (Stand.LipMeters <= 0.0 || Stand.ClearanceMeters < 0.0 || Stand.BeadMeters < 0.0
			|| 2.0 * Stand.BeadMeters >= Stand.HeightMeters - Stand.FootHeightMeters)
		{
			Out.Add(TEXT("its lip is more than zero wide, its clearance zero or more, and its bead fits its side"));
		}
		if (Stand.Feet < FewestFeet || Stand.FootRadiusMeters <= 0.0 || Stand.FootInsetMeters < 0.0)
		{
			Out.Add(FString::Printf(TEXT("it stands on %d feet or more, each more than zero across, set in from its side"), FewestFeet));
		}
		return Out;
	}

	FStoneStandForm CarveStoneStand(const FFormMesh& Rock, double CellMeters, const FArtStoneStand& Stand)
	{
		FStoneStandForm Form;
		if (Rock.IsEmpty() || CellMeters <= 0.0 || !StoneStandFaults(Stand).IsEmpty())
		{
			return Form;
		}
		const FOutline Recess = RecessOf(Rock, CellMeters, Stand);
		if (Recess.Radii.Contains(0.0))
		{
			return Form;
		}
		const double Top = Stand.HeightMeters;
		const double Floor = Top - Stand.RecessMeters;
		const double Under = Stand.FootHeightMeters;
		const double Lip = Stand.LipMeters;
		const double Bead = Stand.BeadMeters;
		const double BeadCentre = Under + BeadRiseShare * (Top - Under);
		FFormMesh& Out = Form.Mesh;

		// The side, from the underside up: plumb to the bead, round the bead's half-round, plumb again to the lip.
		const FVector2D Outward(1.0, 0.0);
		TArray<FSectionPoint> Side = { { Lip, Under, Outward }, { Lip, BeadCentre - Bead, Outward } };
		for (int32 Step = 0; Step <= BeadSteps && Bead > 0.0; ++Step)
		{
			const double Angle = -0.5 * UE_DOUBLE_PI + UE_DOUBLE_PI * Step / BeadSteps;
			const FVector2D Facing(FMath::Cos(Angle), FMath::Sin(Angle));
			Side.Add({ Lip + Bead * Facing.X, BeadCentre + Bead * Facing.Y, Facing });
		}
		Side.Add({ Lip, BeadCentre + Bead, Outward });
		Side.Add({ Lip, Top, Outward });
		for (int32 Index = 0; Index + 1 < Side.Num(); ++Index)
		{
			if (Side[Index].Up != Side[Index + 1].Up || Side[Index].Out != Side[Index + 1].Out)
			{
				Band(Recess, Side[Index], Side[Index + 1], Out);
			}
		}
		// The lip across the top, the recess's wall facing in, and its floor, where the rock stands; the underside.
		const FVector2D Up(0.0, 1.0);
		Band(Recess, { Lip, Top, Up }, { 0.0, Top, Up }, Out);
		const FVector2D Inward(-1.0, 0.0);
		Band(Recess, { 0.0, Top, Inward }, { 0.0, Floor, Inward }, Out);
		Cap(Recess, 0.0, Floor, true, Out);
		Cap(Recess, Lip, Under, false, Out);

		// The feet, evenly round it between the angles, each its inset and its radius in from the side.
		for (int32 Index = 0; Index < Stand.Feet; ++Index)
		{
			const int32 At = FMath::RoundToInt((Index + 0.5) * StandAngles / Stand.Feet) % StandAngles;
			const FVector2D Where = Recess.At(At, Lip - Stand.FootInsetMeters - Stand.FootRadiusMeters);
			Foot(Where, Stand.FootRadiusMeters, Under, Out);
		}

		Form.RecessFloorMeters = Floor;
		Form.RecessRadiiMeters = Recess.Radii;
		for (const double Radius : Recess.Radii)
		{
			Form.OutlineRadiiMeters.Add(Radius + Lip);
		}
		return Form;
	}
}
