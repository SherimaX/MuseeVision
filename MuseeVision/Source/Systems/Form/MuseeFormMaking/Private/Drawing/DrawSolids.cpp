#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/Extrude.h"
#include "Kernel/Lathe.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Prism.h"
#include "Kernel/Regions.h"
#include "Kernel/Sweep.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form::Drawing
{
	namespace
	{
		/** The drawing frame's (x, y) plane, facing +z, raised by a height. */
		FPlaneFrame GroundPlane(double HeightMeters, bool bFacingUp = true)
		{
			FPlaneFrame Plane;
			Plane.Origin = FVector(0.0, 0.0, HeightMeters);
			Plane.AxisU = FVector::ForwardVector;
			Plane.AxisV = FVector::RightVector;
			Plane.Normal = bFacingUp ? FVector::UpVector : -FVector::UpVector;
			return Plane;
		}

		bool ResolveHoles(TConstArrayView<FFormProfile> Holes, TArray<FProfileStations>& OutHoles, FString& OutWhy)
		{
			for (const FFormProfile& Hole : Holes)
			{
				if (!CheckProfile(Hole, TEXT("a hole"), true, OutWhy))
				{
					return false;
				}
				OutHoles.Add(Resolve(Hole));
			}
			return true;
		}

		/** The extrusion's parts, as the kernel's worked prisms draw them. */
		const FPrismParts ExtrusionParts{FFormExtrusion::NearPart, FFormExtrusion::FarPart, FFormExtrusion::SidePart, FFormExtrusion::PocketPart};

		/** An extrusion with pockets sunk into its faces or its arrises eased. */
		bool DrawWorkedExtrusion(const FFormExtrusion& Shape, const FProfileStations& Outline, TConstArrayView<FProfileStations> Holes,
			FMeshBuilder& Mesh, FString& OutWhy)
		{
			TArray<FPrismPocket> Pockets;
			for (const FFormPocket& Pocket : Shape.Pockets)
			{
				if (!CheckProfile(Pocket.Outline, TEXT("a pocket"), true, OutWhy))
				{
					return false;
				}
				Pockets.Add({RegionOf(Resolve(Pocket.Outline).Points()), Pocket.DepthMeters, Pocket.bNearFace});
			}
			if (!Shape.NearArris.IsSharp() || !Shape.FarArris.IsSharp())
			{
				TArray<FProfileStations> Loops = {Outline};
				Loops.Append(Holes.GetData(), Holes.Num());
				return ArrisedPrism(Mesh, Loops, Shape.DepthMeters, Shape.NearArris, Shape.FarArris, Pockets, ExtrusionParts, OutWhy);
			}
			TArray<TArray<FVector2D>> HolePoints;
			for (const FProfileStations& Hole : Holes)
			{
				HolePoints.Add(Hole.Points());
			}
			return PocketedPrism(Mesh, RegionOf(Outline.Points(), HolePoints), Shape.DepthMeters, Pockets, ExtrusionParts, OutWhy);
		}

		/** A full turn, within the angle two stations can't tell apart. */
		bool IsFullTurn(double FromDegrees, double ToDegrees)
		{
			return FMath::Abs(ToDegrees - FromDegrees) >= 360.0 - FMath::RadiansToDegrees(TangentJointRadians);
		}

		/**
		 * The plan line a square lathe runs its section along: the polygon of the section's widest apothem, from ToDegrees
		 * back to FromDegrees, so its right (where the section's A lies) looks out from the axis. Its corners are mitred.
		 */
		TArray<FVector2D> SquareLathePath(const FFormLathe& Shape, double Apothem, bool bFull)
		{
			const double Step = 2.0 * UE_DOUBLE_PI / Shape.Sides;
			auto OnPolygon = [Step, Apothem](double Angle)
			{
				const double Middle = Step * FMath::RoundToDouble(Angle / Step);
				return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Apothem / FMath::Cos(Angle - Middle));
			};
			TArray<FVector2D> Path;
			if (bFull)
			{
				for (int32 Corner = Shape.Sides - 1; Corner >= 0; --Corner)
				{
					Path.Add(OnPolygon(Step * (Corner + 0.5)));
				}
				return Path;
			}
			const double From = FMath::DegreesToRadians(FMath::Min(Shape.FromDegrees, Shape.ToDegrees));
			const double To = FMath::DegreesToRadians(FMath::Max(Shape.FromDegrees, Shape.ToDegrees));
			Path.Add(OnPolygon(To));
			for (double Corner = Step * (FMath::CeilToDouble(To / Step - 0.5) - 0.5); Corner > From; Corner -= Step)
			{
				if (Corner < To - TangentJointRadians && Corner > From + TangentJointRadians)
				{
					Path.Add(OnPolygon(Corner));
				}
			}
			Path.Add(OnPolygon(From));
			return Path;
		}

		/** A square lathe: the section, moved in by the apothem, swept round the polygon with mitred corners. */
		bool DrawSquareLathe(const FFormLathe& Shape, const FProfileStations& Section, FMeshBuilder& Mesh, FString& OutWhy)
		{
			const double Apothem = LargestRadius(Section);
			if (Shape.Sides < 3 || Apothem <= WeldToleranceMeters)
			{
				OutWhy = TEXT("a square lathe turns a section that stands off its axis round a polygon of three sides or more");
				return false;
			}
			FProfileStations Moved = Section;
			for (FProfileVertex& Vertex : Moved.Vertices)
			{
				Vertex.Point.X -= Apothem;
			}
			const bool bFull = IsFullTurn(Shape.FromDegrees, Shape.ToDegrees);
			const TArray<FVector2D> Path = SquareLathePath(Shape, Apothem, bFull);
			TArray<bool> Crisp;
			Crisp.Init(true, Path.Num());
			const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, Crisp, bFull, 0.0);
			if (Runs.IsEmpty())
			{
				OutWhy = TEXT("the square lathe's turn is too short to run");
				return false;
			}
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				Sweep(Mesh, Run, Moved);
			}
			if (Moved.bClosed && !bFull && !(Cap(Mesh, Runs[0][0], Moved, false) && Cap(Mesh, Runs.Last().Last(), Moved, true)))
			{
				OutWhy = TEXT("the section's outline crosses itself, so the turn's ends can't be capped");
				return false;
			}
			return true;
		}
	}

	bool CheckProfile(const FFormProfile& Profile, const TCHAR* Role, bool bClosed, FString& OutWhy)
	{
		FString Why;
		if (!Profile.IsWellFormed(&Why))
		{
			OutWhy = FString::Printf(TEXT("%s is not well drawn: %s"), Role, *Why);
			return false;
		}
		if (bClosed && !Profile.Closed)
		{
			OutWhy = FString::Printf(TEXT("%s must be closed"), Role);
			return false;
		}
		return true;
	}

	bool DrawExtrusion(const FFormExtrusion& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		TArray<FProfileStations> Holes;
		if (!CheckProfile(Shape.Outline, TEXT("the outline"), true, OutWhy) || !ResolveHoles(Shape.Holes, Holes, OutWhy))
		{
			return false;
		}
		const FProfileStations Outline = Resolve(Shape.Outline);
		if (!Shape.NearArris.IsSharp() || !Shape.FarArris.IsSharp() || !Shape.Pockets.IsEmpty())
		{
			return DrawWorkedExtrusion(Shape, Outline, Holes, Mesh, OutWhy);
		}
		Mesh.UseSlot(FFormExtrusion::SidePart);
		TArray<TArray<FVector2D>> HolePoints;
		for (const FProfileStations& Hole : Holes)
		{
			HolePoints.Add(Hole.Points());
		}
		const bool bUp = Shape.DepthMeters > 0.0;
		bool bMade = Extrude(Mesh, GroundPlane(0.0), Outline, Holes, Shape.DepthMeters, false);
		Mesh.UseSlot(FFormExtrusion::NearPart);
		bMade = bMade && AddPlanarFace(Mesh, GroundPlane(0.0, !bUp), Outline.Points(), HolePoints);
		Mesh.UseSlot(FFormExtrusion::FarPart);
		bMade = bMade && AddPlanarFace(Mesh, GroundPlane(Shape.DepthMeters, bUp), Outline.Points(), HolePoints);
		if (!bMade)
		{
			OutWhy = TEXT("the outline or a hole crosses itself or another, or the depth is zero");
		}
		return bMade;
	}

	bool DrawLathe(const FFormLathe& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckProfile(Shape.Section, TEXT("the section"), false, OutWhy))
		{
			return false;
		}
		const FProfileStations Section = Resolve(Shape.Section);
		for (const FProfileVertex& Vertex : Section.Vertices)
		{
			if (Vertex.Point.X < 0.0)
			{
				OutWhy = FString::Printf(TEXT("the section crosses the axis (radius %.4f m)"), Vertex.Point.X);
				return false;
			}
		}
		if (Shape.Sides != 0)
		{
			return DrawSquareLathe(Shape, Section, Mesh, OutWhy);
		}
		const FLatheAxis Axis;
		const double From = FMath::DegreesToRadians(Shape.FromDegrees);
		const double To = FMath::DegreesToRadians(Shape.ToDegrees);
		const TArray<double> Angles = LatheStations(Section, From, To);
		Lathe(Mesh, Section, Axis, Angles);
		if (Section.bClosed && !IsFullTurn(Shape.FromDegrees, Shape.ToDegrees))
		{
			const TArray<FSweepFrame> Frames = LatheFrames(Axis, Angles, LargestRadius(Section));
			if (!Cap(Mesh, Frames[0], Section, false) || !Cap(Mesh, Frames.Last(), Section, true))
			{
				OutWhy = TEXT("the section's outline crosses itself, so the turn's ends can't be capped");
				return false;
			}
		}
		return true;
	}

	bool DrawFace(const FFormFace& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		TArray<FProfileStations> Holes;
		if (!CheckProfile(Shape.Outline, TEXT("the outline"), true, OutWhy) || !ResolveHoles(Shape.Holes, Holes, OutWhy))
		{
			return false;
		}
		TArray<TArray<FVector2D>> HolePoints;
		for (const FProfileStations& Hole : Holes)
		{
			HolePoints.Add(Hole.Points());
		}
		if (!AddPlanarFace(Mesh, GroundPlane(0.0), Resolve(Shape.Outline).Points(), HolePoints))
		{
			OutWhy = TEXT("the outline or a hole crosses itself or another");
			return false;
		}
		return true;
	}

	bool DrawCeiling(const FFormCeiling& Shape, FMeshBuilder& Slab, FMeshBuilder& Pane, FString& OutWhy)
	{
		if (!CheckProfile(Shape.Outline, TEXT("the outline"), true, OutWhy) || !CheckProfile(Shape.Opening, TEXT("the opening"), true, OutWhy))
		{
			return false;
		}
		if (Shape.ThicknessMeters <= 0.0 || Shape.PaneMeters < 0.0
			|| (Shape.PaneMeters > 0.0 && Shape.PaneLevelMeters + Shape.PaneMeters > Shape.ThicknessMeters))
		{
			OutWhy = TEXT("the slab has no thickness, or the pane doesn't lie within it");
			return false;
		}
		const FProfileStations Opening = Resolve(Shape.Opening);
		const FProfileStations Holes[1] = {Opening};
		if (!Extrude(Slab, GroundPlane(0.0), Resolve(Shape.Outline), Holes, Shape.ThicknessMeters, true))
		{
			OutWhy = TEXT("the outline or the opening crosses itself or the other");
			return false;
		}
		if (Shape.PaneMeters > 0.0 && !Extrude(Pane, GroundPlane(Shape.PaneLevelMeters), Opening, {}, Shape.PaneMeters, true))
		{
			OutWhy = TEXT("the opening's outline can't be glazed");
			return false;
		}
		return true;
	}

	bool DrawBasin(const FFormBasin& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckProfile(Shape.Outline, TEXT("the outline"), true, OutWhy))
		{
			return false;
		}
		if (Shape.DepthMeters <= 0.0 || Shape.FloorMeters <= 0.0 || Shape.WallMeters <= 0.0)
		{
			OutWhy = TEXT("a basin has a depth, a bed and sides");
			return false;
		}
		const FProfileStations Loop = RunOutward(Resolve(Shape.Outline));
		const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Loop, 0.0);
		if (Runs.IsEmpty())
		{
			OutWhy = TEXT("the outline doubles back too sharply to mitre");
			return false;
		}
		// The sides' section, A out of the basin and B up, drawn with its solid on the left: up the outer face, in
		// across the rim, down the lining to the bed.
		const double Wall = Shape.WallMeters;
		const double Bed = Shape.FloorMeters;
		const double Rim = Bed + Shape.DepthMeters;
		auto Part = [](const FVector2D& From, const FVector2D& To)
		{
			return FProfile(From).LineTo(To).Resolve(DefaultChordToleranceMeters);
		};
		const TPair<const TCHAR*, FProfileStations> Sides[3] = {
			{FFormBasin::OuterPart, Part(FVector2D(Wall, 0.0), FVector2D(Wall, Rim))},
			{FFormBasin::RimPart, Part(FVector2D(Wall, Rim), FVector2D(0.0, Rim))},
			{FFormBasin::LiningPart, Part(FVector2D(0.0, Rim), FVector2D(0.0, Bed))}};
		TArray<FVector2D> Underside;
		for (const TArray<FSweepFrame>& Run : Runs)
		{
			for (const TPair<const TCHAR*, FProfileStations>& Side : Sides)
			{
				Mesh.UseSlot(Side.Key);
				Sweep(Mesh, Run, Side.Value);
			}
			for (int32 Index = 0; Index + 1 < Run.Num(); ++Index)
			{
				const FVector Out = Run[Index].At(FVector2D(Wall, 0.0));
				Underside.Add(FVector2D(Out.X, Out.Y));
			}
		}
		Mesh.UseSlot(FFormBasin::LiningPart);
		const bool bBed = AddPlanarFace(Mesh, GroundPlane(Bed), Loop.Points());
		Mesh.UseSlot(FFormBasin::OuterPart);
		const bool bUnderside = AddPlanarFace(Mesh, GroundPlane(0.0, false), Underside);
		if (!bBed || !bUnderside)
		{
			OutWhy = TEXT("the outline, or its offset by the sides' thickness, crosses itself");
			return false;
		}
		return true;
	}
}
