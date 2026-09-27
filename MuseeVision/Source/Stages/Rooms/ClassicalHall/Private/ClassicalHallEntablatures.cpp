#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The Ionic entablature (§3.4), 0.72 over the architrave's soffit at 3.06: the architrave's three fasciae and cyma, the
 * frieze, the bed moulding, the dentil band, the ovolo, the corona and the sima, with the ledge at 3.70 behind the lip
 * where the coves lie and the vaults spring. It runs down both gallery walls, breaking forward in a ressaut over each
 * column pair (filled solid behind), and across the screen; round the tribune's wall, over the throat on the lintel;
 * and round the ring beam on both its faces, back to back. The dentils are carved on each straight run and each ring.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		/** The profile (A out from the architrave's lowest fascia, B up from its soffit), closed back to BackA. */
		FFormProfile EntablatureSection(double BackA)
		{
			return FPen(BackA, 0.0).Strokes(Plan::EntablatureFace).To(BackA, Plan::Ledge - Plan::ArchitraveSoffit).Closed();
		}

		FFormDrawing Entablature(const FString& Name, FFormProfile Path, double BackA)
		{
			FFormSweep Shape;
			Shape.Section = EntablatureSection(BackA);
			Shape.Path = MoveTemp(Path);
			Shape.ElevationMeters = Plan::ArchitraveSoffit;
			return Drawn(Name, TEXT("entablature"), MoveTemp(Shape), nullptr);
		}

		/** Whether a dentil stands at a distance along a run: n dentils of the band's width, evenly pitched from a start. */
		double DentilAt(double U, double Start, int32 Count, double Pitch)
		{
			const double Along = U - Start;
			if (Along < 0.0 || Count < 1)
			{
				return 0.0;
			}
			const int32 Index = FMath::FloorToInt32(Along / Pitch);
			return Index < Count && Along - Index * Pitch <= Plan::DentilWidth ? Plan::DentilProud : 0.0;
		}

		/** The dentils along a run's path, carved on the band 0.035 out from the fascia, from Start at a pitch. */
		FFormDrawing Dentils(const FString& Name, FFormProfile Path, double Start, int32 Count, double Pitch)
		{
			FFormRelief Relief;
			Relief.Path = MoveTemp(Path);
			Relief.OffsetMeters = Plan::DentilBand;
			Relief.BottomMeters = Plan::DentilFoot;
			Relief.TopMeters = Plan::DentilTop;
			Relief.GridMeters = Plan::DentilGrid;
			Relief.BackMeters = Plan::Sink;
			Relief.HeightMeters = [Start, Count, Pitch](double U, double V) { return DentilAt(U, Start, Count, Pitch); };
			return Drawn(Name, TEXT("entablature"), MoveTemp(Relief), nullptr);
		}

		/** A ring's entablature and dentils: the path a circle, run clockwise to face in or anticlockwise to face out. */
		void Ring(TArray<FFormDrawing>& Out, const FString& Name, double Radius, bool bFacingIn, double BackA)
		{
			const FFormProfile Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, bFacingIn);
			Out.Add(Entablature(Name + TEXT(".entablature"), Path, BackA));
			const double Length = 2.0 * UE_DOUBLE_PI * Radius;
			const int32 Count = FMath::FloorToInt32(Length / Plan::DentilPitch);
			const double Pitch = Length / Count;
			Out.Add(Dentils(Name + TEXT(".dentils"), Path, 0.5 * (Pitch - Plan::DentilWidth), Count, Pitch));
		}

		/**
		 * The gallery's run as one path along the architrave's face: from the south wall up the west wall, round each
		 * ressaut, across the screen and down the east wall, crisp at every corner.
		 */
		TArray<FVector2D> GalleryPath()
		{
			const double X = Plan::GalleryHalf - Plan::PilasterProud;
			const double XR = Plan::GalleryHalf - Plan::RessautFace;
			const double South = Plan::SouthFaceY + Plan::Sink;
			TArray<FVector2D> Points = {{-X, South}};
			for (const double Pair : Plan::PairCentres)
			{
				Points.Append({{-X, Pair + Plan::RessautHalf}, {-XR, Pair + Plan::RessautHalf}, {-XR, Pair - Plan::RessautHalf}, {-X, Pair - Plan::RessautHalf}});
			}
			Points.Append({{-X, Plan::ScreenEntablatureY}, {X, Plan::ScreenEntablatureY}});
			for (int32 Index = int32(UE_ARRAY_COUNT(Plan::PairCentres)) - 1; Index >= 0; --Index)
			{
				const double Pair = Plan::PairCentres[Index];
				Points.Append({{X, Pair - Plan::RessautHalf}, {XR, Pair - Plan::RessautHalf}, {XR, Pair + Plan::RessautHalf}, {X, Pair + Plan::RessautHalf}});
			}
			Points.Add({X, South});
			return Points;
		}

		/**
		 * The gallery's dentils, run by run. At an inside corner (the path turning to its right, towards the room) the
		 * neighbouring run's mitre takes the band's depth, so the dentils stop that much further back.
		 */
		void GalleryDentils(TArray<FFormDrawing>& Out, const TArray<FVector2D>& Points)
		{
			const double Band = Plan::DentilBand + Plan::DentilProud;
			auto Inside = [&Points](int32 Corner)
			{
				if (Corner <= 0 || Corner >= Points.Num() - 1)
				{
					return false;
				}
				const FVector2D In = Points[Corner] - Points[Corner - 1];
				const FVector2D Onward = Points[Corner + 1] - Points[Corner];
				return In.X * Onward.Y - In.Y * Onward.X > 0.0;
			};
			for (int32 Run = 0; Run + 1 < Points.Num(); ++Run)
			{
				const double Length = FVector2D::Distance(Points[Run], Points[Run + 1]);
				const double From = Plan::DentilMargin + (Inside(Run) ? Band : 0.0);
				const double To = Length - Plan::DentilMargin - (Inside(Run + 1) ? Band : 0.0);
				const int32 Count = FMath::FloorToInt32((To - From - Plan::DentilWidth) / Plan::DentilPitch) + 1;
				if (To - From < Plan::DentilWidth || Count < 1)
				{
					continue;
				}
				const double Start = From + 0.5 * (To - From - ((Count - 1) * Plan::DentilPitch + Plan::DentilWidth));
				Out.Add(Dentils(FString::Printf(TEXT("gallery.dentils.%02d"), Run + 1), FFormProfile::Polyline({Points[Run], Points[Run + 1]}, false), Start,
					Count, Plan::DentilPitch));
			}
		}

		/** The fillers behind each ressaut, solid from the wall to the ressaut's back, z 3.07 to 3.685, the pair's y ± 0.65. */
		void Fillers(TArray<FFormDrawing>& Out)
		{
			const double Back = Plan::GalleryHalf - Plan::RessautFace - Plan::WallBackA - Plan::Sink;
			for (const double Side : {-1.0, 1.0})
			{
				for (int32 Pair = 0; Pair < int32(UE_ARRAY_COUNT(Plan::PairCentres)); ++Pair)
				{
					const double Y = Plan::PairCentres[Pair];
					const double Wall = Plan::GalleryHalf + Plan::Sink;
					const FFormProfile Box = Rect(FMath::Min(Side * Back, Side * Wall), Y - Plan::RessautFillerHalf, FMath::Max(Side * Back, Side * Wall), Y + Plan::RessautFillerHalf);
					Out.Add(PlanPrism(FString::Printf(TEXT("gallery.ressaut.%s.%d"), Side > 0.0 ? TEXT("east") : TEXT("west"), Pair + 1), TEXT("entablature"), Box,
						{}, Plan::RessautFillerFoot, Plan::RessautFillerTop, nullptr));
				}
			}
		}
	}

	void DrawEntablatures(TArray<FFormDrawing>& Out)
	{
		const TArray<FVector2D> Points = GalleryPath();
		Out.Add(Entablature(TEXT("gallery.entablature"), FFormProfile::Polyline(Points, false), Plan::WallBackA));
		GalleryDentils(Out, Points);
		Fillers(Out);
		Ring(Out, TEXT("tribune"), Plan::TribuneEntablatureR, true, Plan::WallBackA);
		const double Back = -0.5 * (Plan::RingOuterFace - Plan::RingInnerFace);
		Ring(Out, TEXT("tribune.ring.inner"), Plan::RingInnerFace, true, Back);
		Ring(Out, TEXT("tribune.ring.outer"), Plan::RingOuterFace, false, Back);
	}
}
