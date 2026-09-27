#include "Kernel/Prism.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Profile.h"
#include "Kernel/Sweep.h"
#include "Stations/Stations.h"

namespace Musee::Form
{
	namespace
	{
		using UE::Geometry::FGeneralPolygon2d;

		/**
		 * A vertical strip of wall round a closed loop from Z0 up to Z1, each edge facing its right (the loop's solid lies
		 * on its left) and taking the slot SlotOf gives it. U runs up the wall and V along the loop, in metres.
		 */
		void WallStrip(FMeshBuilder& Mesh, const FProfileStations& Loop, double Z0, double Z1,
			TFunctionRef<FName(const FVector2D&, const FVector2D&)> SlotOf)
		{
			const int32 Count = Loop.Vertices.Num();
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FProfileVertex& A = Loop.Vertices[Index];
				const FProfileVertex& B = Loop.Vertices[(Index + 1) % Count];
				const FVector NormalA(A.NormalOut, 0.0);
				const FVector NormalB(B.NormalIn, 0.0);
				const double VA = A.LengthMeters;
				const double VB = Loop.LengthAtSegmentEnd(Index);
				Mesh.UseSlot(SlotOf(A.Point, B.Point));
				const int32 A0 = Mesh.AddVertex(FVector(A.Point, Z0), NormalA, FVector2D(Z0, VA));
				const int32 B0 = Mesh.AddVertex(FVector(B.Point, Z0), NormalB, FVector2D(Z0, VB));
				const int32 B1 = Mesh.AddVertex(FVector(B.Point, Z1), NormalB, FVector2D(Z1, VB));
				const int32 A1 = Mesh.AddVertex(FVector(A.Point, Z1), NormalA, FVector2D(Z1, VA));
				Mesh.AddQuad(A0, B0, B1, A1);
			}
		}

		/** A flat face at height Z, facing up or down, with UVs of (x, y) metres. */
		bool LevelFace(FMeshBuilder& Mesh, TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes, double Z, bool bUp)
		{
			return AddFlatPolygon(Mesh, Outer, Holes, [Z](const FVector2D& P) { return FVector(P, Z); },
				bUp ? FVector::UpVector : -FVector::UpVector, [](const FVector2D& P) { return P; });
		}

		/** Whether the segment from A to B lies along one of the loops. */
		bool LiesOn(TConstArrayView<TArray<FVector2D>> Loops, const FVector2D& A, const FVector2D& B)
		{
			const FVector2D Middle = 0.5 * (A + B);
			for (const TArray<FVector2D>& Loop : Loops)
			{
				for (int32 Index = 0; Index < Loop.Num(); ++Index)
				{
					const FVector2D& P = Loop[Index];
					const FVector2D& Q = Loop[(Index + 1) % Loop.Num()];
					if (FMath::PointDistToSegment(FVector(Middle, 0.0), FVector(P, 0.0), FVector(Q, 0.0)) <= WeldToleranceMeters)
					{
						return true;
					}
				}
			}
			return false;
		}

		/** The levels of a pocketed prism, bottom to top: its faces and the pockets' floors. */
		bool Levels(double Depth, TConstArrayView<FPrismPocket> Pockets, TArray<double>& OutLevels, FString& OutWhy)
		{
			OutLevels = {0.0, Depth};
			for (const FPrismPocket& Pocket : Pockets)
			{
				if (Pocket.DepthMeters <= WeldToleranceMeters || Pocket.CoverMeters < 0.0
					|| Pocket.CoverMeters + Pocket.DepthMeters >= Depth - WeldToleranceMeters)
				{
					OutWhy = FString::Printf(TEXT("a pocket %.4f m deep under %.4f m of cover in a slab %.4f m thick has no floor"),
						Pocket.DepthMeters, Pocket.CoverMeters, Depth);
					return false;
				}
				OutLevels.Add(Pocket.Low(Depth));
				OutLevels.Add(Pocket.High(Depth));
			}
			OutLevels.Sort();
			for (int32 Index = OutLevels.Num() - 1; Index > 0; --Index)
			{
				if (OutLevels[Index] - OutLevels[Index - 1] <= WeldToleranceMeters)
				{
					OutLevels.RemoveAt(Index);
				}
			}
			return true;
		}

		/** Every point of every loop of the regions. */
		TArray<FVector2D> PointsOf(TConstArrayView<const FRegion*> Regions)
		{
			TArray<FVector2D> Points;
			for (const FRegion* Region : Regions)
			{
				for (const TArray<FVector2D>& Loop : AllLoops(*Region))
				{
					Points.Append(Loop);
				}
			}
			return Points;
		}

		/** The loops of a polygon, each set out on the given points. */
		TArray<TArray<FVector2D>> NodedLoops(const FGeneralPolygon2d& Polygon, TConstArrayView<FVector2D> Nodes)
		{
			TArray<TArray<FVector2D>> Loops = LoopsOf(Polygon);
			for (TArray<FVector2D>& Loop : Loops)
			{
				InsertPointsOnEdges(Loop, Nodes);
			}
			return Loops;
		}

		/** A region's faces at a level, noded. */
		bool RegionFaces(FMeshBuilder& Mesh, const FRegion& Region, TConstArrayView<FVector2D> Nodes, double Z, bool bUp)
		{
			for (const FGeneralPolygon2d& Polygon : Region)
			{
				TArray<TArray<FVector2D>> Loops = NodedLoops(Polygon, Nodes);
				if (Loops.IsEmpty())
				{
					continue;
				}
				const TArray<FVector2D> Outer = Loops[0];
				Loops.RemoveAt(0);
				if (!LevelFace(Mesh, Outer, Loops, Z, bUp))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool PocketedPrism(FMeshBuilder& Mesh, const FRegion& Base, double DepthMeters, TConstArrayView<FPrismPocket> Pockets,
		const FPrismParts& Parts, FString& OutWhy)
	{
		TArray<double> Heights;
		if (DepthMeters <= 0.0 || Base.IsEmpty())
		{
			OutWhy = TEXT("a pocketed slab has a positive depth and an outline");
			return false;
		}
		if (!Levels(DepthMeters, Pockets, Heights, OutWhy))
		{
			return false;
		}
		const int32 Bands = Heights.Num() - 1;
		TArray<FRegion> Layers;
		for (int32 Band = 0; Band < Bands; ++Band)
		{
			const double Middle = 0.5 * (Heights[Band] + Heights[Band + 1]);
			FRegion Taken;
			for (const FPrismPocket& Pocket : Pockets)
			{
				const bool bReaches = Middle > Pocket.Low(DepthMeters) && Middle < Pocket.High(DepthMeters);
				Taken = bReaches ? Union(Taken, Pocket.Region) : Taken;
			}
			Layers.Add(Difference(Base, Taken));
		}
		// Where the layers change, the faces between them: up where material lies below, down where it lies above.
		TArray<FRegion> Up;
		TArray<FRegion> Down;
		for (int32 Level = 0; Level <= Bands; ++Level)
		{
			const FRegion Below = Level > 0 ? Layers[Level - 1] : FRegion();
			const FRegion Above = Level < Bands ? Layers[Level] : FRegion();
			Up.Add(Difference(Below, Above));
			Down.Add(Difference(Above, Below));
		}
		TArray<const FRegion*> Every = {&Base};
		for (int32 Index = 0; Index < Layers.Num(); ++Index)
		{
			Every.Add(&Layers[Index]);
		}
		for (int32 Index = 0; Index < Up.Num(); ++Index)
		{
			Every.Add(&Up[Index]);
			Every.Add(&Down[Index]);
		}
		const TArray<FVector2D> Nodes = PointsOf(Every);
		const TArray<TArray<FVector2D>> BaseLoops = AllLoops(Base);
		auto SlotOf = [&](const FVector2D& A, const FVector2D& B) { return LiesOn(BaseLoops, A, B) ? Parts.Side : Parts.Pocket; };
		for (int32 Band = 0; Band < Bands; ++Band)
		{
			for (const FGeneralPolygon2d& Polygon : Layers[Band])
			{
				for (const TArray<FVector2D>& Loop : NodedLoops(Polygon, Nodes))
				{
					WallStrip(Mesh, LoopStations(Loop), Heights[Band], Heights[Band + 1], SlotOf);
				}
			}
		}
		for (int32 Level = 0; Level <= Bands; ++Level)
		{
			Mesh.UseSlot(Level == 0 ? Parts.Near : (Level == Bands ? Parts.Far : Parts.Pocket));
			if (!RegionFaces(Mesh, Up[Level], Nodes, Heights[Level], true) || !RegionFaces(Mesh, Down[Level], Nodes, Heights[Level], false))
			{
				OutWhy = TEXT("a face of the slab crosses itself where its pockets meet it");
				return false;
			}
		}
		return true;
	}

	namespace
	{
		/** The section of an eased side, A out of the solid and B up, drawn up the side with the solid on its left. */
		FProfileStations EasedSide(double Depth, const FFormArris& Near, const FFormArris& Far)
		{
			const double N = FMath::Max(Near.Meters, 0.0);
			const double F = FMath::Max(Far.Meters, 0.0);
			FProfile Section(FVector2D(-N, 0.0));
			if (N > 0.0)
			{
				Near.bChamfer ? Section.LineTo(FVector2D(0.0, N)) : Section.Arc(FVector2D(-N, N), N, -UE_DOUBLE_HALF_PI, 0.0);
			}
			Section.LineTo(FVector2D(0.0, Depth - F));
			if (F > 0.0)
			{
				Far.bChamfer ? Section.LineTo(FVector2D(-F, Depth)) : Section.Arc(FVector2D(-F, Depth - F), F, 0.0, UE_DOUBLE_HALF_PI);
			}
			return Section.Resolve(DefaultChordToleranceMeters);
		}

		/** The pockets of one face, as holes in it: each must lie within the face's outline, clear of its holes and of the others. */
		bool FacePockets(TConstArrayView<FPrismPocket> Pockets, bool bNear, const TArray<FVector2D>& Outer,
			TConstArrayView<TArray<FVector2D>> Holes, TArray<TArray<FVector2D>>& OutLoops, FString& OutWhy)
		{
			FRegion Taken;
			for (const FPrismPocket& Pocket : Pockets)
			{
				if (Pocket.bNearFace != bNear)
				{
					continue;
				}
				for (const FGeneralPolygon2d& Polygon : Pocket.Region)
				{
					const TArray<TArray<FVector2D>> Loops = LoopsOf(Polygon);
					if (Loops.IsEmpty())
					{
						continue;
					}
					bool bClear = Loops.Num() == 1;
					for (const FVector2D& Point : Loops[0])
					{
						bClear &= IsInside(Outer, Point);
						for (const TArray<FVector2D>& Hole : Holes)
						{
							bClear &= !IsInside(Hole, Point);
						}
					}
					if (!bClear || AreaSquareMeters(Intersection(Taken, {Polygon})) > SliverSquareMeters)
					{
						OutWhy = TEXT("a pocket in an eased face must lie inside the face, clear of its eased arrises, of its holes and of the other pockets, with no island");
						return false;
					}
					Taken.Add(Polygon);
					OutLoops.Add(Loops[0]);
				}
			}
			return true;
		}

		/** A blind pocket's walls, facing into it, and its floor, facing out of the face it is sunk into. */
		bool BlindPocket(FMeshBuilder& Mesh, const TArray<FVector2D>& Loop, double Face, double Floor, FName Slot)
		{
			// The pocket's loop run clockwise, so its walls face into it: the solid lies round it.
			const FProfileStations Stations = LoopStations(Loop);
			WallStrip(Mesh, Stations.SignedAreaSquareMeters() > 0.0 ? Stations.Reversed() : Stations, FMath::Min(Face, Floor),
				FMath::Max(Face, Floor), [Slot](const FVector2D&, const FVector2D&) { return Slot; });
			Mesh.UseSlot(Slot);
			return LevelFace(Mesh, Loop, {}, Floor, Floor < Face);
		}
	}

	bool ArrisedPrism(FMeshBuilder& Mesh, TConstArrayView<FProfileStations> Loops, double DepthMeters, const FFormArris& Near,
		const FFormArris& Far, TConstArrayView<FPrismPocket> Pockets, const FPrismParts& Parts, FString& OutWhy)
	{
		if (DepthMeters <= 0.0 || Loops.IsEmpty() || FMath::Max(Near.Meters, 0.0) + FMath::Max(Far.Meters, 0.0) >= DepthMeters)
		{
			OutWhy = TEXT("an eased slab has a positive depth deeper than its two eases");
			return false;
		}
		const FProfileStations Side = EasedSide(DepthMeters, Near, Far);
		const FVector2D Foot = Side.Vertices[0].Point;
		const FVector2D Head = Side.Vertices.Last().Point;
		TArray<TArray<FVector2D>> NearLoops;
		TArray<TArray<FVector2D>> FarLoops;
		for (int32 Index = 0; Index < Loops.Num(); ++Index)
		{
			// Each loop is run so its right, where the side's A points, looks out of the solid: out of the outline, into a hole.
			const bool bOutward = Index == 0 ? Loops[Index].SignedAreaSquareMeters() > 0.0 : Loops[Index].SignedAreaSquareMeters() < 0.0;
			const TArray<TArray<FSweepFrame>> Runs = PlanRuns(bOutward ? Loops[Index].Reversed() : Loops[Index], 0.0);
			if (Runs.IsEmpty())
			{
				OutWhy = TEXT("an outline doubles back too sharply to ease");
				return false;
			}
			Mesh.UseSlot(Parts.Side);
			TArray<FVector2D>& NearLoop = NearLoops.AddDefaulted_GetRef();
			TArray<FVector2D>& FarLoop = FarLoops.AddDefaulted_GetRef();
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				Sweep(Mesh, Run, Side);
				for (int32 Station = 0; Station + 1 < Run.Num(); ++Station)
				{
					NearLoop.Add(FVector2D(Run[Station].At(Foot)));
					FarLoop.Add(FVector2D(Run[Station].At(Head)));
				}
			}
		}
		const TArray<FVector2D> NearOuter = NearLoops[0];
		const TArray<FVector2D> FarOuter = FarLoops[0];
		NearLoops.RemoveAt(0);
		FarLoops.RemoveAt(0);
		TArray<TArray<FVector2D>> NearPockets;
		TArray<TArray<FVector2D>> FarPockets;
		if (!FacePockets(Pockets, true, NearOuter, NearLoops, NearPockets, OutWhy) || !FacePockets(Pockets, false, FarOuter, FarLoops, FarPockets, OutWhy))
		{
			return false;
		}
		bool bMade = true;
		for (const FPrismPocket& Pocket : Pockets)
		{
			if (Pocket.DepthMeters <= WeldToleranceMeters || Pocket.DepthMeters >= DepthMeters - WeldToleranceMeters || Pocket.CoverMeters != 0.0)
			{
				OutWhy = FString::Printf(TEXT("a pocket in an eased slab opens at its face and has a floor (%.4f m deep in %.4f m)"), Pocket.DepthMeters,
					DepthMeters);
				return false;
			}
			for (const FGeneralPolygon2d& Polygon : Pocket.Region)
			{
				const TArray<TArray<FVector2D>> Loops = LoopsOf(Polygon);
				bMade &= Loops.IsEmpty() || BlindPocket(Mesh, Loops[0], Pocket.bNearFace ? 0.0 : DepthMeters,
					Pocket.bNearFace ? Pocket.DepthMeters : DepthMeters - Pocket.DepthMeters, Parts.Pocket);
			}
		}
		NearLoops.Append(NearPockets);
		FarLoops.Append(FarPockets);
		Mesh.UseSlot(Parts.Near);
		bMade &= LevelFace(Mesh, NearOuter, NearLoops, 0.0, false);
		Mesh.UseSlot(Parts.Far);
		bMade &= LevelFace(Mesh, FarOuter, FarLoops, DepthMeters, true);
		if (!bMade)
		{
			OutWhy = TEXT("a face of the eased slab crosses itself");
		}
		return bMade;
	}
}
