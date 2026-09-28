#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The hedge form, re-expressed from the as-built museum's clipped hedges (windows/Source/MuseeVision/Nature/
 * MuseeHedge.cpp): a clipped hedge is a shell, grown once along its bed's line. The shears keep a dense skin of small
 * leaves (a yew's needles) on the clipped surface, the section its bed gives (the width at the foot, the height, the
 * batter narrowing the top, the shoulders rounded), over a dark core of bare twigs lying the core inset inside it, which
 * the visitor can't pass through. Each needle stands out of the skin, turned out to the light, its tip at the clipped
 * surface where the shears left it. An open end is clipped square across. The hedge grows as a run of identical modules,
 * one to each plant at its spacing along the line, the two at its ends clipped across where the hedge's ends are open;
 * where the line turns a corner, the modules either side of it are mitred on the plane that halves the corner, so the
 * clipped surface runs round it unbroken. A single plant clipped to a ball (a box ball) is the same skin over a sphere
 * sunk into the soil, round a core of bare twigs.
 *
 * Species: leavesPerSquareMeter of clipped surface; leafLengthMeters, leafWidthMeters. Colours: LeafColours;
 * LeafAgeColours [the season's new growth at the skin]; StemColours [the core's bare twigs].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The share of needles that are the season's new growth, paler, at the skin in early summer. */
		constexpr double NewGrowthShare = 0.25;
		/** Stations round a shoulder's quarter circle. */
		constexpr int32 ShoulderStations = 4;
		/** The clipped surface's stations along the line, a metre apart at most. */
		constexpr double StationMeters = 0.5;
		/** The segments round and the rings up a ball's core, a sphere of bare twigs. */
		constexpr int32 BallCoreSegments = 16;
		constexpr int32 BallCoreRings = 8;

		/** How densely the shears keep the skin: the clip's own leaves to the square metre, else its species'. */
		double SkinDensity(const FNatureSpecies& Species, const FNatureClip& Clip, FRandomStream& Random)
		{
			return Clip.LeavesPerM2 > 0.0f ? double(Clip.LeavesPerM2) : Draw(Species.LeavesPerSquareMeter, Random);
		}

		/** Where a run starts along its line at an offset across it: at 0, or on its mitre. */
		double StartAt(const FPlantSite& Site, double Across)
		{
			return Site.StartMitre * Across;
		}

		/** Where a run of this length ends along its line at an offset across it: at its length, or on its mitre. */
		double EndAt(const FPlantSite& Site, double Length, double Across)
		{
			return Length + Site.EndMitre * Across;
		}

		/**
		 * The section of a surface Inset inside the clipped one, as points (across, up) from the foot's left to its right
		 * over the top, and each point's outward normal.
		 */
		void Section(const FNatureClip& Clip, double Inset, TArray<FVector2D>& OutPoints, TArray<FVector2D>& OutNormals)
		{
			const double Half = 0.5 * Clip.Width - Inset;
			const double Top = Clip.Height - Inset;
			const double TopHalf = Half - Clip.Batter;
			const double Round = FMath::Max(0.0, FMath::Min(double(Clip.Shoulder), TopHalf));
			const FVector2D LeftNormal = FVector2D(-Clip.Height, Clip.Batter).GetSafeNormal();
			OutPoints.Add(FVector2D(-Half, 0.0));
			OutNormals.Add(LeftNormal);
			for (int32 K = 0; K <= ShoulderStations; ++K)
			{
				const double A = UE_DOUBLE_PI - UE_DOUBLE_HALF_PI * K / ShoulderStations;
				OutPoints.Add(FVector2D(-TopHalf + Round, Top - Round) + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Round);
				OutNormals.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)));
			}
			for (int32 K = 0; K <= ShoulderStations; ++K)
			{
				const double A = UE_DOUBLE_HALF_PI - UE_DOUBLE_HALF_PI * K / ShoulderStations;
				OutPoints.Add(FVector2D(TopHalf - Round, Top - Round) + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Round);
				OutNormals.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)));
			}
			OutPoints.Add(FVector2D(Half, 0.0));
			OutNormals.Add(FVector2D(Clip.Height, Clip.Batter).GetSafeNormal());
		}

		/** The core: the inset section swept along the line and closed at its ends (square, or on their mitres), its twigs' colour. */
		void Core(const FNatureSpecies& Species, const FPlantSite& Site, const FVector& Along, const FVector& Across, double Length, FPlantParts& Out)
		{
			TArray<FVector2D> Points, Normals;
			Section(Site.Clip, Site.Clip.CoreInset, Points, Normals);
			const FLinearColor Twigs = Colour(Species.StemColours, 0);
			FNatureSurface& Surface = Out.FindOrAdd(Organ::Trunk);
			const FVector From = Site.Line[0];
			const auto At = [&](const FVector2D& P, double X) { return From + Along * X + Across * P.X + FVector::UpVector * P.Y; };
			for (int32 I = 0; I + 1 < Points.Num(); ++I)
			{
				const FVector NA = Across * Normals[I].X + FVector::UpVector * Normals[I].Y;
				const FVector NB = Across * Normals[I + 1].X + FVector::UpVector * Normals[I + 1].Y;
				const double A0X = StartAt(Site, Points[I].X);
				const double A1X = EndAt(Site, Length, Points[I].X);
				const double B0X = StartAt(Site, Points[I + 1].X);
				const double B1X = EndAt(Site, Length, Points[I + 1].X);
				const int32 A0 = Surface.Vertex(At(Points[I], A0X), NA, FVector2D(A0X, I), Twigs, 0.3);
				const int32 A1 = Surface.Vertex(At(Points[I], A1X), NA, FVector2D(A1X, I), Twigs, 0.3);
				const int32 B1 = Surface.Vertex(At(Points[I + 1], B1X), NB, FVector2D(B1X, I + 1), Twigs, 0.3);
				const int32 B0 = Surface.Vertex(At(Points[I + 1], B0X), NB, FVector2D(B0X, I + 1), Twigs, 0.3);
				Surface.Quad(A0, A1, B1, B0);
			}
			for (const bool bEnd : { false, true })
			{
				// The cut's face lies on its plane: along = the mitre times the offset across (plus the length, at the end).
				const FVector Normal = bEnd ? (Along - Across * Site.EndMitre).GetSafeNormal() : (Across * Site.StartMitre - Along).GetSafeNormal();
				const auto On = [&](const FVector2D& P) { return At(P, bEnd ? EndAt(Site, Length, P.X) : StartAt(Site, P.X)); };
				const int32 Middle = Surface.Vertex(On(FVector2D(0.0, 0.5 * Site.Clip.Height)), Normal, FVector2D::ZeroVector, Twigs, 0.3);
				for (int32 I = 0; I + 1 < Points.Num(); ++I)
				{
					Surface.Triangle(Middle, Surface.Vertex(On(Points[I]), Normal, Points[I], Twigs, 0.3), Surface.Vertex(On(Points[I + 1]), Normal, Points[I + 1], Twigs, 0.3));
				}
				Surface.Triangle(Middle, Surface.Vertex(On(Points.Last()), Normal, Points.Last(), Twigs, 0.3), Surface.Vertex(On(Points[0]), Normal, Points[0], Twigs, 0.3));
			}
		}

		/** One needle, standing out of the skin at a point with its outward normal, its tip at the clipped surface. */
		void Needle(const FNatureSpecies& Species, const FVector& Skin, const FVector& Normal, FRandomStream& Random, FNatureSurface& Out)
		{
			const double Length = Draw(Species.LeafLengthMeters, Random);
			const FVector Dir = (Normal + Random.GetUnitVector() * 0.9).GetSafeNormal(UE_SMALL_NUMBER, Normal);
			const FVector Base = Skin - Dir * Length;
			const FVector Face = (Normal - Dir * FVector::DotProduct(Normal, Dir) + FVector::UpVector * 0.3).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			const FLinearColor Tint = Random.FRand() < NewGrowthShare ? Vary(Colour(Species.LeafAgeColours, 0), Random, 4.0, 0.08, 0.1)
				: Vary(AnyColour(Species.LeafColours, Random), Random, 4.0, 0.08, 0.12);
			AddCard(Out, Base, Dir, (Face - Dir * FVector::DotProduct(Face, Dir)).GetSafeNormal(UE_SMALL_NUMBER, Normal), Length, Draw(Species.LeafWidthMeters, Random), 0.2, 0.05, 1,
				Tint, 0.85);
		}

		/**
		 * A clipped ball: a sphere of the clip's diameter, sunk its depth into the soil at the plant's crown, its skin of
		 * needles over the part above the soil (a zone of the sphere, 2 pi R times its height, its points spread evenly by
		 * Archimedes' rule: evenly in height and round the axis), round a core of bare twigs the core inset inside it.
		 */
		void GrowBall(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const double Radius = 0.5 * Site.Clip.Ball;
			const FVector Centre = Site.Crown + FVector(0.0, 0.0, Radius - Site.Clip.Sink);
			const double CoreRadius = Radius - Site.Clip.CoreInset;
			if (CoreRadius > 0.0)
			{
				AddEllipsoid(Out.FindOrAdd(Organ::Trunk), Centre, FVector::UpVector, CoreRadius, CoreRadius, BallCoreSegments, BallCoreRings, Colour(Species.StemColours, 0), 0.3);
			}
			// The soil cuts the sphere this far below its centre (a negative height).
			const double Lowest = Site.Clip.Sink - Radius;
			const double Density = SkinDensity(Species, Site.Clip, Random);
			const int32 Needles = FMath::RoundToInt(Density * 2.0 * UE_DOUBLE_PI * Radius * (Radius - Lowest));
			FNatureSurface& Surface = Out.FindOrAdd(Organ::Leaf);
			for (int32 N = 0; N < Needles; ++N)
			{
				const double Up = Random.FRandRange(Lowest, Radius);
				const double Azimuth = Random.FRandRange(0.0, Tau);
				const double Ring = FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Up * Up));
				const FVector Normal = FVector(Ring * FMath::Cos(Azimuth), Ring * FMath::Sin(Azimuth), Up) / Radius;
				Needle(Species, Centre + Normal * Radius, Normal, Random, Surface);
			}
		}

		void GrowHedge(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			if (Site.Clip.IsBall())
			{
				GrowBall(Species, Site, Random, Out);
				return;
			}
			if (Site.Line.Num() != 2 || !Site.Clip.IsSet())
			{
				return;
			}
			const FVector Span = Site.Line[1] - Site.Line[0];
			const double Length = FVector2D(Span.X, Span.Y).Size();
			const FVector Along = FVector(Span.X, Span.Y, 0.0).GetSafeNormal();
			const FVector Across = FVector::CrossProduct(FVector::UpVector, Along);
			Core(Species, Site, Along, Across, Length, Out);
			TArray<FVector2D> Points, Normals;
			Section(Site.Clip, 0.0, Points, Normals);
			double Perimeter = 0.0;
			TArray<double> Runs = { 0.0 };
			for (int32 I = 0; I + 1 < Points.Num(); ++I)
			{
				Perimeter += FVector2D::Distance(Points[I], Points[I + 1]);
				Runs.Add(Perimeter);
			}
			const double Density = SkinDensity(Species, Site.Clip, Random);
			const int32 Needles = FMath::RoundToInt(Density * Perimeter * Length);
			FNatureSurface& Surface = Out.FindOrAdd(Organ::Leaf);
			for (int32 N = 0; N < Needles; ++N)
			{
				const double S = Random.FRandRange(0.0, Perimeter);
				int32 I = 0;
				while (I + 2 < Runs.Num() && Runs[I + 1] < S)
				{
					++I;
				}
				const double T = (S - Runs[I]) / FMath::Max(UE_SMALL_NUMBER, Runs[I + 1] - Runs[I]);
				const FVector2D P = FMath::Lerp(Points[I], Points[I + 1], T);
				const FVector2D Q = FMath::Lerp(Normals[I], Normals[I + 1], T).GetSafeNormal();
				const double X = Random.FRandRange(StartAt(Site, P.X), EndAt(Site, Length, P.X));
				Needle(Species, Site.Line[0] + Along * X + Across * P.X + FVector::UpVector * P.Y, Across * Q.X + FVector::UpVector * Q.Y, Random, Surface);
			}
			const double EndArea = Site.Clip.Height * (Site.Clip.Width - Site.Clip.Batter);
			for (const double X : { 0.0, Length })
			{
				if ((X == 0.0 && Site.bOpenStart) || (X > 0.0 && Site.bOpenEnd))
				{
					for (int32 N = 0, Count = FMath::RoundToInt(Density * EndArea); N < Count; ++N)
					{
						const double Up = Random.FRandRange(0.0, Site.Clip.Height);
						const double Half = 0.5 * Site.Clip.Width - Site.Clip.Batter * Up / Site.Clip.Height;
						Needle(Species, Site.Line[0] + Along * X + Across * Random.FRandRange(-Half, Half) + FVector::UpVector * Up, X == 0.0 ? -Along : Along, Random, Surface);
					}
				}
			}
		}

		TArray<FString> HedgeFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeavesPerSquareMeter, TEXT("leavesPerSquareMeter (of clipped surface)"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours (the new growth)"), Out);
			NeedColours(Species.StemColours, 1, TEXT("stemColours (the core's twigs)"), Out);
			return Out;
		}
	}

	const FForm& HedgeForm()
	{
		static const FForm Form{ TEXT("hedge"), { Organ::Trunk, Organ::Leaf }, false, &GrowHedge, &HedgeFaults, EPlanting::AlongLine };
		return Form;
	}
}
