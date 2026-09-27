#include "ReservePlan.h"
#include "ReserveParts.h"

/**
 * The overhead tracks (§3.6), one fixed over each rack's plane from the side wall to |y| 0.900: a bronze channel whose
 * lips leave the slot the trolleys' stems run in, its nave-end stop, the wall plate with its four bolts, and three hanger
 * rods, each clamped to the channel with a collar and seated in a canopy let into the brick above.
 */
namespace Musee::Reserve
{
	namespace
	{
		/** Out of reach overhead: the tracks stop nothing but the racks' trolleys, which run inside them. */
		const Musee::Collision::FClass* const Overhead = nullptr;

		/** The channel in section (A across, z), anticlockwise: lips, sides and top plate round the hollow the trolleys run in. */
		FFormProfile ChannelSection()
		{
			const double W = Plan::TrackHalf;
			const double S = Plan::TrackSlotHalf;
			const double In = W - Plan::TrackSide;
			const double Lip = Plan::TrackUnderside + Plan::TrackLip;
			const double Soffit = Plan::TrackTop - Plan::TrackPlate;
			const double Bottom = Plan::TrackUnderside;
			return FFormProfile::Polygon({FVector2D(-W, Bottom), FVector2D(-S, Bottom), FVector2D(-S, Lip), FVector2D(-In, Lip), FVector2D(-In, Soffit),
				FVector2D(In, Soffit), FVector2D(In, Lip), FVector2D(S, Lip), FVector2D(S, Bottom), FVector2D(W, Bottom), FVector2D(W, Plan::TrackTop),
				FVector2D(-W, Plan::TrackTop)});
		}

		/** A box spanning two y in either order. */
		FBlock BoxAcross(double X, double HalfX, double YA, double YB, double Z0, double Z1)
		{
			return Block(FVector(X - HalfX, FMath::Min(YA, YB), Z0), FVector(X + HalfX, FMath::Max(YA, YB), Z1));
		}

		void Hanger(TArray<FFormDrawing>& Out, const FString& Track, int32 Index, double X, double Y)
		{
			const FString Name = FString::Printf(TEXT("%s.hanger.%d"), *Track, Index + 1);
			const FVector At(X, Y, 0.0);
			const double ClampTop = Plan::TrackTop + Plan::ClampHeight;
			const double CollarTop = ClampTop + Plan::CollarHeight;
			const double CanopyTop = SeatOver(X, Y, 0.5 * Plan::RodCanopyDiameter);
			const double CanopyBottom = CanopyTop - Plan::RodCanopyHeight;
			Out.Add(Drawn(Name + TEXT(".clamp"), TEXT("track"), Block(FVector(X - 0.5 * Plan::ClampAcross, Y - 0.5 * Plan::ClampAlong, Plan::TrackTop),
				FVector(X + 0.5 * Plan::ClampAcross, Y + 0.5 * Plan::ClampAlong, ClampTop)), Overhead));
			Out.Add(Drawn(Name + TEXT(".collar"), TEXT("track"), Cylinder(Plan::CollarDiameter, ClampTop, CollarTop), Overhead, FMuseeFrame(At)));
			Out.Add(Drawn(Name + TEXT(".rod"), TEXT("track"), Cylinder(Plan::RodDiameter, CollarTop, CanopyBottom), Overhead, FMuseeFrame(At)));
			Out.Add(Drawn(Name + TEXT(".canopy"), TEXT("track"), Cylinder(Plan::RodCanopyDiameter, CanopyBottom, CanopyTop), Overhead, FMuseeFrame(At)));
		}

		void Track(TArray<FFormDrawing>& Out, const FRack& Rack)
		{
			const FString Name = FString::Printf(TEXT("track.%s"), *Rack.Name);
			const double S = Rack.Side;
			const double Face = S * (Plan::InnerY - Plan::WallPlateProud);
			const double End = S * Plan::TrackEnd;

			FFormSweep Channel;
			Channel.Section = ChannelSection();
			Channel.Path = FFormProfile::Polyline({FVector2D(Rack.X, FMath::Min(Face, End)), FVector2D(Rack.X, FMath::Max(Face, End))}, false);
			Out.Add(Drawn(Name + TEXT(".channel"), TEXT("track"), MoveTemp(Channel), Overhead));
			Out.Add(Drawn(Name + TEXT(".stop"), TEXT("track"), BoxAcross(Rack.X, Plan::StopHalf, End, End - S * Plan::StopThickness, Plan::StopBottom,
				Plan::StopTop), Overhead));
			Out.Add(Drawn(Name + TEXT(".plate"), TEXT("track"), BoxAcross(Rack.X, Plan::WallPlateHalf, Face, S * Plan::InnerY, Plan::WallPlateBottom,
				Plan::WallPlateTop), Overhead));
			// The bolts' hex heads on the plate's face, standing out towards the nave.
			const double Apothem = 0.5 * Plan::BoltAcrossFlats;
			FFormDrawing Bolt = Drawn(Name + TEXT(".bolt"), TEXT("track"), Turned({FVector2D(0.0, 0.0), FVector2D(Apothem, 0.0), FVector2D(Apothem,
				Plan::BoltHead), FVector2D(0.0, Plan::BoltHead)}, 6), Overhead);
			for (const double Z : Plan::BoltZs)
			{
				for (const double Across : {-Plan::BoltX, Plan::BoltX})
				{
					Bolt.Repeats.Add(AlongAxis(FVector(Rack.X + Across, Face, Z), FVector(0.0, -S, 0.0)));
				}
			}
			Out.Add(MoveTemp(Bolt));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::RodYs)); ++Index)
			{
				Hanger(Out, Name, Index, Rack.X, S * Plan::RodYs[Index]);
			}
		}
	}

	void DrawTracks(TArray<FFormDrawing>& Out)
	{
		for (const FRack& Rack : Racks())
		{
			Track(Out, Rack);
		}
	}
}
