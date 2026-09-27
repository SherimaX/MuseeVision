#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The tree form, re-expressed from the as-built museum's tree grower (windows/Source/MuseeVision/Nature/MuseeTree.cpp,
 * after the Swift gardens' tree()): a trunk (or several) with its flare, lean and wander; orders of branches in a
 * phyllotactic spiral at the species' angles, curving, drooping under their weight or rising to the light, the scaffold
 * limbs reaching for the crown's envelope so the tree fills the crown its bed draws; leaves along the twigs, turned out
 * of the crown and up to the sky, darker deep inside it; and, where the species flowers in the museum's season, flowers
 * in clusters on the wood. Every number of its habit is the species record's (FNatureTreeHabit).
 *
 * Species colours: StemColours [the trunk's bark, the twigs']; LeafColours the leaves', one drawn per leaf;
 * FlowerColours [the flowers', a second the habit's share of them take].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** A branch's tip is this share of its foot's radius. */
		constexpr double TwigTipShare = 0.08;
		/** How the radius falls along a branch: r = r0 (1 - (1 - tip) s^0.85), as the as-built grower drew it. */
		constexpr double TaperPower = 0.85;
		/** The trunk starts a little under the soil, so its flare meets the ground. */
		constexpr double TrunkFootDepth = 0.08;
		/** The flare dies away over this height. */
		constexpr double FlareHeightMeters = 1.0;
		/** A collar where a branch leaves its parent, as a share of its radius. */
		constexpr double CollarShare = 1.15;
		/** A child is never thicker than this share of its parent where it leaves it. */
		constexpr double MostChildShare = 0.85;
		/** A branch shorter than this isn't grown. */
		constexpr double ShortestBranchMeters = 0.04;
		/** A twig thinner than this is drawn with three sides. */
		constexpr double ThinnestRoundMeters = 0.0045;
		/**
		 * The occlusion deep in the crown, where little sky reaches (the leaves' and the flowers' floor), carried in their
		 * vertices' alpha, as the as-built grower shaded them.
		 */
		constexpr double LeafShadeFloor = 0.45;
		constexpr double FlowerShadeFloor = 0.6;

		/** How far a point may go along a direction before it leaves an ellipsoid (0 if it is already outside). */
		double RayToEllipsoid(const FVector& P, const FVector& D, const FVector& Centre, const FVector& Radii)
		{
			const FVector O = (P - Centre) / Radii;
			const FVector V = D / Radii;
			const double A = V.SizeSquared();
			const double B = 2.0 * FVector::DotProduct(O, V);
			const double C = O.SizeSquared() - 1.0;
			const double Disc = B * B - 4.0 * A * C;
			if (A <= UE_SMALL_NUMBER || Disc < 0.0)
			{
				return 0.0;
			}
			return FMath::Max(0.0, (-B + FMath::Sqrt(Disc)) / (2.0 * A));
		}

		/** The occlusion inside the crown, where little sky reaches: the floor at its middle, none at its edge. */
		double CrownShade(const FVector& P, const FVector& Centre, const FVector& Radii, double Floor)
		{
			const double E = ((P - Centre) / Radii).Size();
			return FMath::Clamp(Floor + (1.0 - Floor) * E * E, Floor, 1.0);
		}

		struct FBranch
		{
			TArray<FVector> Points;
			TArray<FVector> Dirs;
			TArray<double> Radii;
			int32 Order = 0;
			double Length = 0.0;

			void Sample(double T, FVector& OutP, FVector& OutD, double& OutR) const
			{
				const int32 Segs = Points.Num() - 1;
				const double X = FMath::Clamp(T, 0.0, 1.0) * Segs;
				const int32 I = FMath::Clamp(FMath::FloorToInt(X), 0, Segs - 1);
				const double F = X - I;
				OutP = FMath::Lerp(Points[I], Points[I + 1], F);
				OutD = FMath::Lerp(Dirs[I], Dirs[I + 1], F).GetSafeNormal(UE_SMALL_NUMBER, Dirs[I + 1]);
				OutR = FMath::Lerp(Radii[I], Radii[I + 1], F);
			}
		};

		/** One tree's growth: its size, its crown, and the branches grown so far. */
		class FTreeGrowth
		{
		public:
			FTreeGrowth(const FNatureSpecies& InSpecies, const FPlantSite& Site, FRandomStream& InRandom)
				: Species(InSpecies), Habit(InSpecies.Tree), Random(InRandom), Foot(Site.Crown)
			{
				Height = Site.Height > 0.0 ? Site.Height : Draw(Species.HeightMeters, Random);
				const double Across = Site.CrownRadii.X > 0.0 ? Site.CrownRadii.X : Habit.CrownRadiusShares[0] * Height;
				const double Vertical = Site.CrownRadii.Y > 0.0 ? Site.CrownRadii.Y : Habit.CrownRadiusShares[1] * Height;
				CrownRadii = FVector(Across, Across, Vertical);
				CrownCentre = Foot + FVector(0.0, 0.0, Site.CrownCentre > 0.0 ? Site.CrownCentre : Habit.CrownCentreShare * Height);
				TrunkRadius = Habit.TrunkRadiusMeters * FMath::Sqrt(Height / Middle(Species.HeightMeters));
			}

			void Grow()
			{
				const int32 Stems = FMath::Max(1, Habit.Stems);
				for (int32 S = 0; S < Stems; ++S)
				{
					const double Lean = FMath::DegreesToRadians(Habit.LeanDegrees + (Stems > 1 ? Habit.StemSpreadDegrees : 0.0));
					const double Az = Stems == 1 ? Random.FRandRange(0.0, Tau) : Tau * S / Stems + Random.FRandRange(-0.4, 0.4);
					const FVector Dir(FMath::Sin(Lean) * FMath::Cos(Az), FMath::Sin(Lean) * FMath::Sin(Az), FMath::Cos(Lean));
					const FVector Offset = Stems > 1 ? FVector(FMath::Cos(Az), FMath::Sin(Az), 0.0) * TrunkRadius : FVector::ZeroVector;
					Branch(0, Foot + Offset - FVector(0.0, 0.0, TrunkFootDepth), Dir, Height * Habit.TrunkShare, TrunkRadius / FMath::Sqrt(double(Stems)));
				}
			}

			const FNatureSpecies& Species;
			const FNatureTreeHabit& Habit;
			FRandomStream& Random;
			FVector Foot;
			double Height = 0.0;
			double TrunkRadius = 0.0;
			FVector CrownCentre;
			FVector CrownRadii;
			TArray<FBranch> Branches;

		private:
			bool bCrownPlaced = false;

			/** The path of one branch: its segments curving, wandering, drooping, rising and kinking as the habit says. */
			FBranch Path(int32 Order, const FVector& Start, const FVector& Dir, double Length)
			{
				const bool bTrunk = Order == 0;
				const FNatureBranching* Own = bTrunk ? nullptr : &Habit.Branching[Order - 1];
				const int32 Segs = FMath::Max(1, bTrunk ? Habit.TrunkSegments : Own->Segments);
				const double Curve = bTrunk ? 0.0 : Own->CurveDegrees;
				const double Wander = bTrunk ? Habit.WanderDegrees : Own->WanderDegrees;
				const double Droop = bTrunk ? 0.0 : Own->DroopDegrees;
				const double Rise = bTrunk ? 0.0 : Own->RiseDegrees;
				const double Zig = bTrunk ? Habit.ZigzagDegrees : Own->ZigzagDegrees;
				FBranch B;
				B.Order = Order;
				B.Length = Length;
				B.Points.Add(Start);
				B.Dirs.Add(Dir);
				FVector Cur = Dir;
				double ZigSign = Random.FRand() < 0.5 ? 1.0 : -1.0;
				double ZigAz = Random.FRandRange(0.0, Tau);
				for (int32 I = 1; I <= Segs; ++I)
				{
					const double Along = double(I) / Segs;
					FVector U, W;
					Basis(Cur, U, W);
					FVector Axis = FVector::CrossProduct(Cur, FVector::UpVector);
					Axis = Axis.SizeSquared() < UE_KINDA_SMALL_NUMBER ? U : Axis;
					Cur = RotateAbout(Cur, Axis, FMath::DegreesToRadians(Curve / Segs));
					const double WanderAz = Random.FRandRange(0.0, Tau);
					Cur = RotateAbout(Cur, U * FMath::Cos(WanderAz) + W * FMath::Sin(WanderAz), FMath::DegreesToRadians(Random.FRandRange(-Wander, Wander)));
					Cur = (Cur - FVector::UpVector * (FMath::DegreesToRadians(Droop) / Segs * (0.5 + Along))).GetSafeNormal();
					Cur = (Cur + FVector::UpVector * (FMath::DegreesToRadians(Rise) / Segs)).GetSafeNormal();
					if (Zig != 0.0)
					{
						ZigAz += 2.2;
						Basis(Cur, U, W);
						Cur = RotateAbout(Cur, U * FMath::Cos(ZigAz) + W * FMath::Sin(ZigAz), FMath::DegreesToRadians(Zig) * ZigSign);
						ZigSign = -ZigSign;
					}
					B.Points.Add(B.Points[I - 1] + Cur * (Length / Segs));
					B.Dirs.Add(Cur);
				}
				return B;
			}

			void Branch(int32 Order, const FVector& Start, const FVector& Dir, double Length, double Radius)
			{
				const bool bTrunk = Order == 0;
				FBranch B = Path(Order, Start, Dir, Length);
				const double Tip = bTrunk ? Habit.TrunkTipShare : TwigTipShare;
				for (int32 I = 0; I < B.Points.Num(); ++I)
				{
					const double Along = double(I) / (B.Points.Num() - 1);
					double R = Radius * (1.0 - (1.0 - Tip) * FMath::Pow(Along, TaperPower));
					if (bTrunk)
					{
						const double Rise = FMath::Max(0.0, B.Points[I].Z - Foot.Z) / FlareHeightMeters;
						R *= 1.0 + Habit.FlareShare * FMath::Square(FMath::Max(0.0, 1.0 - Rise));
					}
					B.Radii.Add(R);
				}
				if (bTrunk && !bCrownPlaced)
				{
					// The crown sits over where the trunk has leant to.
					CrownCentre.X = Foot.X + (B.Points.Last().X - Foot.X) * Habit.CrownFollowShare;
					CrownCentre.Y = Foot.Y + (B.Points.Last().Y - Foot.Y) * Habit.CrownFollowShare;
					bCrownPlaced = true;
				}
				Branches.Add(B);
				if (Order < Habit.Branching.Num())
				{
					Children(Order, Branches.Last(), Length);
				}
			}

			void Children(int32 Order, const FBranch Parent, double Length)
			{
				const bool bTrunk = Order == 0;
				const FNatureBranching& Kids = Habit.Branching[Order];
				const int32 Count = bTrunk ? Kids.Count : FMath::Max(0, FMath::RoundToInt(Kids.PerMeter * Length + Random.FRandRange(-0.5, 0.5)));
				const double Az0 = Random.FRandRange(0.0, Tau);
				double Az = Az0;
				for (int32 K = 0; K < Count; ++K)
				{
					const double T = FMath::Clamp(Kids.Start + (Kids.End - Kids.Start) * (K + 0.5 + Random.FRandRange(-0.35, 0.35)) / FMath::Max(1, Count), 0.0, 0.999);
					FVector P, D;
					double PR;
					Parent.Sample(T, P, D, PR);
					const double Rel = (T - Kids.Start) / FMath::Max(1e-3, double(Kids.End - Kids.Start));
					const double Angle = (bTrunk ? FMath::Lerp(double(Kids.AngleLowDegrees), double(Kids.AngleDegrees), Rel) : Kids.AngleDegrees)
						+ Random.FRandRange(-Kids.AngleSpreadDegrees, Kids.AngleSpreadDegrees);
					Az = bTrunk && Habit.bEvenScaffolds ? Az0 + Tau * K / FMath::Max(1, Count) + FMath::DegreesToRadians(Random.FRandRange(-14.0, 14.0))
						: Az + FMath::DegreesToRadians(Kids.PhyllotaxisDegrees + Random.FRandRange(-12.0, 12.0));
					FVector U, W;
					Basis(D, U, W);
					const FVector ChildDir = RotateAbout(D, U * FMath::Cos(Az) + W * FMath::Sin(Az), FMath::DegreesToRadians(Angle)).GetSafeNormal();
					const double Reach = RayToEllipsoid(P, ChildDir, CrownCentre, CrownRadii);
					double ChildLength = (1.0 + Random.FRandRange(-Kids.LengthSpread, Kids.LengthSpread)) * Kids.LengthShare;
					if (bTrunk)
					{
						// A scaffold limb reaches for the crown's edge.
						ChildLength *= Reach > 0.0 ? Reach : TrunkRadius;
					}
					else
					{
						ChildLength *= Length * (1.0 - Kids.LengthTaper * T);
						ChildLength = FMath::Min(ChildLength, FMath::Max(Reach * 1.1 + 0.15 * (4 - Order), 0.08));
					}
					if (ChildLength < ShortestBranchMeters)
					{
						continue;
					}
					double ChildRadius = PR * Kids.RadiusShare * (0.55 + 0.45 * FMath::Sqrt(FMath::Min(1.0, ChildLength / FMath::Max(Length, 1e-3))));
					ChildRadius = FMath::Max(FMath::Min(ChildRadius, PR * MostChildShare), double(Habit.MinRadiusMeters));
					Branch(Order + 1, P, ChildDir, ChildLength, ChildRadius);
				}
			}
		};

		void Wood(const FTreeGrowth& Tree, FPlantParts& Out)
		{
			const FLinearColor Bark = Colour(Tree.Species.StemColours, 0);
			const FLinearColor Twig = Colour(Tree.Species.StemColours, 1);
			const int32 Orders = FMath::Max(1, Tree.Habit.Branching.Num());
			for (const FBranch& B : Tree.Branches)
			{
				const int32 Sides = B.Order == 0 ? Tree.Habit.TrunkSides : Tree.Habit.Branching[B.Order - 1].Sides;
				const FLinearColor Tint = Mix(Bark, Twig, double(B.Order) / Orders);
				TArray<FTubeRing> Rings;
				for (int32 I = 0; I < B.Points.Num(); ++I)
				{
					const FVector Before = B.Dirs[FMath::Max(0, I - 1)];
					const FVector After = B.Dirs[I];
					Rings.Add({ B.Points[I], (Before + After).GetSafeNormal(UE_SMALL_NUMBER, After), B.Radii[I] * (I == 0 && B.Order > 0 ? CollarShare : 1.0), Tint });
				}
				AddTube(Out.FindOrAdd(B.Order == 0 ? Organ::Trunk : Organ::Stem), Rings, B.Radii[0] < ThinnestRoundMeters ? 3 : Sides, true);
			}
		}

		/** The leaves along the leaf-bearing twigs, as many as the crown's volume holds, turned out and up to the light. */
		void Leaves(const FTreeGrowth& Tree, FRandomStream& Random, FPlantParts& Out)
		{
			const FNatureTreeHabit& Habit = Tree.Habit;
			const FNatureSpecies& Species = Tree.Species;
			const double MiddleHeight = Middle(Species.HeightMeters);
			const double ReferenceVolume = Habit.CrownRadiusShares[0] * Habit.CrownRadiusShares[0] * Habit.CrownRadiusShares[1] * FMath::Pow(MiddleHeight, 3.0);
			const double Volume = Tree.CrownRadii.X * Tree.CrownRadii.Y * Tree.CrownRadii.Z;
			double Twigs = 0.0;
			for (const FBranch& B : Tree.Branches)
			{
				Twigs += B.Order >= Habit.LeafOrder ? B.Length * (1.0 - Habit.LeafStartShare) : 0.0;
			}
			const double PerMeter = Habit.LeavesPerPlant * Volume / FMath::Max(ReferenceVolume, UE_SMALL_NUMBER) / FMath::Max(Twigs, 1e-3);
			FNatureSurface& Surface = Out.FindOrAdd(Organ::Leaf);
			for (const FBranch& B : Tree.Branches)
			{
				if (B.Order < Habit.LeafOrder)
				{
					continue;
				}
				const int32 N = FMath::FloorToInt(PerMeter * B.Length * (1.0 - Habit.LeafStartShare) + Random.FRand());
				for (int32 K = 0; K < N; ++K)
				{
					FVector P, D;
					double PR;
					B.Sample(Habit.LeafStartShare + (1.0 - Habit.LeafStartShare) * (K + Random.FRand()) / N, P, D, PR);
					FVector U, W;
					Basis(D, U, W);
					const double A = K * 2.39996 + Random.FRandRange(-0.3, 0.3);
					const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
					FVector Dir = RotateAbout(D, FVector::CrossProduct(D, Radial), FMath::DegreesToRadians(Habit.LeafAngleDegrees + Random.FRandRange(-15.0, 15.0)));
					Dir = (Dir - FVector::UpVector * (Habit.LeafDroop * Random.FRandRange(0.6, 1.4))).GetSafeNormal(UE_SMALL_NUMBER, Radial);
					const FVector Outward = ((P - Tree.CrownCentre) / Tree.CrownRadii).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					const FVector Want = (Outward * 0.55 + FVector::UpVector * 0.75 + Random.GetUnitVector() * 0.45).GetSafeNormal();
					const FVector Normal = (Want - Dir * FVector::DotProduct(Want, Dir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::CrossProduct(Dir, U));
					const double Length = Draw(Species.LeafLengthMeters, Random);
					const double Width = FMath::Min(Draw(Species.LeafWidthMeters, Random), Length);
					const FLinearColor Tint = Vary(AnyColour(Species.LeafColours, Random), Random, 5.0, 0.1, 0.14);
					AddCard(Surface, P + Radial * PR, Dir, Normal, Length, Width, Habit.LeafFold, Habit.LeafCurl, 1, Tint,
						CrownShade(P, Tree.CrownCentre, Tree.CrownRadii, LeafShadeFloor));
				}
			}
		}

		/** The flowers on the wood, in clusters on their stalks, facing out of the crown. */
		void Flowers(const FTreeGrowth& Tree, FRandomStream& Random, FPlantParts& Out)
		{
			const FNatureTreeHabit& Habit = Tree.Habit;
			const FNatureSpecies& Species = Tree.Species;
			double Wood = 0.0;
			for (const FBranch& B : Tree.Branches)
			{
				Wood += B.Order >= Habit.FlowerOrder ? B.Length : 0.0;
			}
			const double Clusters = Draw(Species.FlowersPerPlant, Random) / FMath::Max(1.0, 0.5 * (Habit.FlowersPerCluster[0] + Habit.FlowersPerCluster[1]));
			const double PerMeter = Clusters / FMath::Max(Wood, 1e-3);
			FNatureSurface& Surface = Out.FindOrAdd(Organ::Flower);
			for (const FBranch& B : Tree.Branches)
			{
				if (B.Order < Habit.FlowerOrder)
				{
					continue;
				}
				const int32 N = FMath::FloorToInt(PerMeter * B.Length + Random.FRand());
				for (int32 K = 0; K < N; ++K)
				{
					FVector P, D;
					double PR;
					B.Sample(0.08 + 0.92 * (K + Random.FRand()) / N, P, D, PR);
					FVector U, W;
					Basis(D, U, W);
					const FVector Outward = ((P - Tree.CrownCentre) / Tree.CrownRadii).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					const int32 Cluster = Draw(Habit.FlowersPerCluster, Random);
					for (int32 J = 0; J < Cluster; ++J)
					{
						const double A = Random.FRandRange(0.0, Tau);
						const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
						const double Stalk = Habit.PedicelMeters * Random.FRandRange(0.5, 1.3);
						const FVector Centre = P + Radial * (PR + Stalk) + D * Random.FRandRange(-0.01, 0.01);
						const FVector Facing = (Radial * 0.8 + FVector::UpVector * 0.3 + Outward * 0.4 + Random.GetUnitVector() * 0.35).GetSafeNormal(UE_SMALL_NUMBER, Radial);
						const FLinearColor Tint = Vary(Colour(Species.FlowerColours, Random.FRand() < Habit.SecondColourShare ? 1 : 0), Random, 3.0, 0.08, 0.04);
						AddFlowerCard(Surface, Centre, Facing, Random.FRandRange(0.0, Tau), Draw(Species.FlowerSizeMeters, Random), 0.22, Tint,
							CrownShade(P, Tree.CrownCentre, Tree.CrownRadii, FlowerShadeFloor));
					}
				}
			}
		}

		void GrowTree(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			FTreeGrowth Tree(Species, Site, Random);
			Tree.Grow();
			Wood(Tree, Out);
			Leaves(Tree, Random, Out);
			if (Random.FRand() < Species.FloweringShare)
			{
				Flowers(Tree, Random, Out);
			}
		}

		TArray<FString> TreeFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			const FNatureTreeHabit& Habit = Species.Tree;
			NeedRange(Species.HeightMeters, TEXT("heightMeters"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.StemColours, 2, TEXT("stemColours (the bark, the twigs)"), Out);
			if (Habit.CrownRadiusShares.Num() != 2 || Habit.CrownRadiusShares[0] <= 0.0f || Habit.CrownRadiusShares[1] <= 0.0f || Habit.CrownCentreShare <= 0.0f)
			{
				Out.Add(TEXT("its tree habit needs its crown: crownCentreShare, and crownRadiusShares [across, vertical]"));
			}
			if (Habit.Branching.IsEmpty() || Habit.Branching[0].Count <= 0 || Habit.TrunkRadiusMeters <= 0.0f || Habit.TrunkShare <= 0.0f || Habit.LeavesPerPlant <= 0
				|| Habit.LeafOrder < 1 || Habit.LeafOrder > Habit.Branching.Num())
			{
				Out.Add(TEXT("its tree habit needs a trunk, scaffold limbs, and the order of branching its leaves grow on"));
			}
			if (Species.FloweringShare > 0.0f)
			{
				NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant"), Out);
				NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
				NeedRange(Habit.FlowersPerCluster, TEXT("tree.flowersPerCluster"), Out);
				NeedColours(Species.FlowerColours, 2, TEXT("flowerColours"), Out);
			}
			return Out;
		}
	}

	const FForm& TreeForm()
	{
		static const FForm Form{ TEXT("tree"), { Organ::Trunk, Organ::Stem, Organ::Leaf, Organ::Flower }, false, &GrowTree, &TreeFaults };
		return Form;
	}
}
