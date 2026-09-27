#include "SphereParts.h"
#include "SpherePlan.h"

/**
 * The mast's footing (§3.6, Q-1 as ruled): the gilt foot ring (body sphere.mast.foot), drawn at the car's stop mast 2 mm
 * over the dogs, and the four CuSn10 bearing dogs (bodies sphere.mast.dogs.1 to .4), drawn retracted, each in its pearl
 * steel housing through the throat and out under the flange. The column between the foot plane and the car's seat is
 * Motion's variable-length piece, drawn by rule and never at one length, so it is not drawn here; the mast's head
 * (body sphere.mast.head) carries nothing of its own.
 */
namespace Musee::Sphere
{
	namespace
	{
		/** The foot ring, in its body's frame on the foot plane: an annulus round the column's base. */
		void FootRing(TArray<FFormDrawing>& Out)
		{
			FFormDrawing& Made = Out.Add_GetRef(Drawn(TEXT("mast.foot-ring"), TEXT("mast"), Ring(Plan::FootRingInner, Plan::FootRingOuter,
				Plan::FootRingBottom - Plan::Mouth, Plan::FootRingTop - Plan::Mouth), Solid));
			Made.Body = Key(TEXT("mast.foot"));
		}

		/** A dog in its body's frame (at its tip's top face, x inward along its slide): the bar runs back out along -x. */
		void Dog(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double Half = 0.5 * Plan::DogWidth;
			FFormDrawing& Bar = Out.Add_GetRef(Drawn(FString::Printf(TEXT("mast.dog.%d"), Index + 1), TEXT("mast.dogs"),
				Slab(Rectangle(FVector2D(-Plan::DogLength, -Half), FVector2D(0.0, Half)), {}, Plan::DogDepth), Solid,
				FMuseeFrame(FVector(0.0, 0.0, -Plan::DogDepth))));
			Bar.Body = DogBody(Index);
		}

		/**
		 * A dog's housing: a channel from the throat's bore out to its end plate (its floor under the dog, its sides 10 mm
		 * clear of it, its roof the flange's underside), and a cap beyond the flange, the top plate and the end plate.
		 */
		void Housing(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double Bearing = Plan::DogBearings[Index];
			const double InHalf = 0.5 * Plan::HousingInside;
			const double OutHalf = InHalf + Plan::HousingPlate;
			const double Floor = Plan::HousingBottom + Plan::HousingPlate;
			const double EndIn = Plan::HousingOuterEnd - Plan::HousingPlate;

			// The channel's section across the bearing (x the way the bearing rises, y up), pushed out along it.
			const FFormProfile Channel = FFormProfile::Polyline({FVector2D(-OutHalf, Plan::HousingBottom), FVector2D(OutHalf, Plan::HousingBottom),
				FVector2D(OutHalf, Plan::Mouth), FVector2D(InHalf, Plan::Mouth), FVector2D(InHalf, Floor), FVector2D(-InHalf, Floor),
				FVector2D(-InHalf, Plan::Mouth), FVector2D(-OutHalf, Plan::Mouth)}, true);
			const FVector2D Bore = AtBearing(Plan::ThroatBore, Bearing);
			Out.Add(Drawn(Pieces::HousingChannel(Index), TEXT("throat"), Slab(Channel, {}, EndIn - Plan::ThroatBore), Solid,
				FrameOfAxes(FVector(Bore.X, Bore.Y, 0.0), Tangent(Bearing), FVector::UpVector)));

			// The cap's section along the bearing (x out, y up), pushed across it from one outer side to the other.
			const FFormProfile Cap = FFormProfile::Polyline({FVector2D(Plan::FlangeOuter, Plan::Mouth), FVector2D(EndIn, Plan::Mouth),
				FVector2D(EndIn, Plan::HousingBottom), FVector2D(Plan::HousingOuterEnd, Plan::HousingBottom),
				FVector2D(Plan::HousingOuterEnd, Plan::Mouth + Plan::HousingPlate), FVector2D(Plan::FlangeOuter, Plan::Mouth + Plan::HousingPlate)}, true);
			Out.Add(Drawn(Pieces::HousingCap(Index), TEXT("throat"), Slab(Cap, {}, 2.0 * OutHalf), Solid,
				FrameOfAxes(Tangent(Bearing) * OutHalf, Radial(Bearing), FVector::UpVector)));
		}
	}

	void DrawMast(TArray<FFormDrawing>& Out)
	{
		FootRing(Out);
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::DogBearings)); ++Index)
		{
			Dog(Out, Index);
			Housing(Out, Index);
		}
	}
}
