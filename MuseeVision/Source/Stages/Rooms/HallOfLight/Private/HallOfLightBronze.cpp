#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The bronze (sec. 3.4), CuSn10 brushed: the prints' flat stems, two to a print, standing in their shoes on the plinth
 * top with the mounts' backs bearing on them; the autochromes' glazing bars from the shoe's bed to the transom; and the
 * four stereo readers, a plate on a stem over each stereo stone, tilted back to the viewer on the axis.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A block in plan at an x, x +-Half, between two depths of a wall, from Bottom to Top. */
		FFormDrawing Block(const FString& Name, const TCHAR* LocalSurface, int32 Side, double X, double Half, double FromD, double ToD, double Bottom,
			double Top)
		{
			return PlanRun(Name, Surface(LocalSurface), FRunLimit::OnPlane(X - Half), FRunLimit::OnPlane(X + Half), WallY(Side, FromD), WallY(Side, ToD),
				Bottom, Top, Solid);
		}

		/** The prints' stems at their bays' x +-0.300, bays 1-8 on both walls, each in its shoe on the plinth. */
		void Stems(TArray<FFormDrawing>& Out, int32 Side)
		{
			for (int32 Bay = 1; Bay <= Plan::PrintBays; ++Bay)
			{
				for (const double Hand : {-1.0, 1.0})
				{
					const double X = BayX(Bay) + Hand * Plan::StemOffset;
					const FString Name = FString::Printf(TEXT("print.stem.%s.%d.%s"), SideName(Side), Bay, Hand < 0.0 ? TEXT("west") : TEXT("east"));
					Out.Add(Block(Name, TEXT("print.stem"), Side, X, Plan::StemHalf, Plan::StemFromD, Plan::StemToD, Plan::StemBottom, Plan::StemTop));
					Out.Add(Block(Name + TEXT(".shoe"), TEXT("print.stem"), Side, X, Plan::StemShoeHalf, Plan::StemShoeFromD, Plan::StemShoeToD,
						Plan::PlinthTop, Plan::StemBottom));
				}
			}
		}

		/** The autochromes' glazing bars at their bays' x +-0.560, bays 9 and 10 on both walls, the cassettes between them. */
		void GlazingBars(TArray<FFormDrawing>& Out, int32 Side)
		{
			for (int32 Bay = Plan::PrintBays + 1; Bay <= Plan::PrintBays + 2; ++Bay)
			{
				for (const double Hand : {-1.0, 1.0})
				{
					const FString Name = FString::Printf(TEXT("autochrome.bar.%s.%d.%s"), SideName(Side), Bay, Hand < 0.0 ? TEXT("west") : TEXT("east"));
					Out.Add(Block(Name, TEXT("autochrome.bar"), Side, BayX(Bay) + Hand * Plan::BarOffset, Plan::GlazingBarHalf, Plan::GlazingBarFromD,
						Plan::GlazingBarToD, Plan::WallGlassFoot, Plan::TransomBottom));
				}
			}
		}

		/**
		 * The stereo readers. Each plate is centred 1.320 over its stone and faces the axis (stones 1 and 3, north of it,
		 * face south), tilted back 0.5 rad; the stem stands 0.016 behind the stone's centre and runs from the stone's top
		 * into the plate's back as far as the plate holds it, its farthest line meeting the plate's back face.
		 */
		void Readers(TArray<FFormDrawing>& Out)
		{
			const double Tilt = Plan::ReaderTiltRadians;
			const double HalfThick = 0.5 * Plan::ReaderThickness;
			for (int32 Index = 0; Index < Plan::StereoStones; ++Index)
			{
				const FVector Stone = StereoStone(Index);
				const FMuseeFrame Reader = StereoReader(Index);
				const FVector Centre = Reader.OriginMeters;
				const FVector Right = Reader.DirectionToParent(FVector::ForwardVector);
				const FVector Normal = Reader.DirectionToParent(FVector::RightVector);
				const FVector UpThePlate = Reader.DirectionToParent(FVector::UpVector);
				const double Facing = Normal.Y > 0.0 ? 1.0 : -1.0;
				const FString Name = FString::Printf(TEXT("stereo.reader.%d"), Index + 1);

				FFormExtrusion Plate;
				Plate.Outline = FFormProfile::Rectangle(FVector2D(-0.5 * Plan::ReaderWidth, -0.5 * Plan::ReaderHeight),
					FVector2D(0.5 * Plan::ReaderWidth, 0.5 * Plan::ReaderHeight));
				Plate.DepthMeters = Plan::ReaderThickness;
				Out.Add(Drawn(Name, Surface(TEXT("stereo.reader")), MoveTemp(Plate), Solid, FrameOfAxes(Centre + Normal * HalfThick, Right, UpThePlate)));

				// The stem's farthest line, (behind + radius) back from the centre, meets the back face this far above the centre.
				const double Back = Plan::ReaderStemBehind + Plan::ReaderStemRadius;
				const double Rise = (Back * FMath::Cos(Tilt) - HalfThick) / FMath::Sin(Tilt);
				const double Height = Plan::ReaderCentreZ + Rise - Plan::StereoStoneHeight;
				FFormLathe Stem;
				Stem.Section = FFormProfile::From(FVector2D(0.0, 0.0));
				Stem.Section.LineTo(FVector2D(Plan::ReaderStemRadius, 0.0), EFormJoint::Crisp).LineTo(FVector2D(Plan::ReaderStemRadius, Height), EFormJoint::Crisp)
					.LineTo(FVector2D(0.0, Height), EFormJoint::Crisp);
				const FVector Foot(Stone.X, Stone.Y - Facing * Plan::ReaderStemBehind, Plan::StereoStoneHeight);
				Out.Add(Drawn(Name + TEXT(".stem"), Surface(TEXT("stereo.reader")), MoveTemp(Stem), Solid, FMuseeFrame(Foot)));
			}
		}
	}

	void DrawBronze(TArray<FFormDrawing>& Out)
	{
		for (const int32 Side : Sides)
		{
			Stems(Out, Side);
			GlazingBars(Out, Side);
		}
		Readers(Out);
	}
}
