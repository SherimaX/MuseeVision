#include "SiteBlind.h"

#include "MuseeCollision.h"
#include "MuseeRecords.h"

namespace Musee::Site
{
	namespace
	{
		/** The surface both leaves are named by; their bronze comes from the blind's record, not a finish schedule. */
		const TCHAR* const LeafSurface = TEXT("blind.doors");

		/**
		 * One leaf's outline in the opening's own plane (across the port, up), metres: from the jamb to the meeting
		 * stile, under the head. Side is -1 for the leaf on the port's -y side, +1 for the other.
		 */
		FFormProfile LeafOutline(const FSitePortClass& Class, const FSiteBlindDoors& Doors, double Side)
		{
			const double Gap = Doors.ClearanceMeters;
			const double Jamb = 0.5 * Class.ClearWidthMeters - Gap;
			const double Meeting = 0.5 * Gap;
			FFormProfile Outline = FFormProfile::From(FVector2D(Side * Meeting, Gap));
			Outline.LineTo(FVector2D(Side * Jamb, Gap), EFormJoint::Crisp);
			if (!Class.Arched)
			{
				Outline.LineTo(FVector2D(Side * Jamb, Class.HeadMeters - Gap), EFormJoint::Crisp);
				Outline.LineTo(FVector2D(Side * Meeting, Class.HeadMeters - Gap), EFormJoint::Crisp);
				return Outline.Close();
			}
			// Up the jamb to the springing, then round the arch (its radius less the gap) to the meeting stile.
			const double Radius = Jamb;
			const double AtJamb = Side < 0.0 ? 180.0 : 0.0;
			const double AtMeeting = FMath::RadiansToDegrees(FMath::Acos(Side * Meeting / Radius));
			Outline.LineTo(FVector2D(Side * Jamb, Class.HeadMeters), EFormJoint::Crisp);
			Outline.ArcAbout(FVector2D(0.0, Class.HeadMeters), Radius, Radius, AtJamb, AtMeeting, EFormJoint::Smooth);
			return Outline.Close();
		}

		/**
		 * The leaves' frame in the port's: its x across the opening (the port's y), its y up and its z out of the stage,
		 * with the origin on the port's centreline at the leaves' inner face, so each leaf is drawn in the opening's plane
		 * and pushed out towards the port's face.
		 */
		FMuseeFrame LeafFrame(const FSitePort& Port, const FSiteBlindDoors& Doors)
		{
			const FQuat Turn = FRotationMatrix::MakeFromXY(FVector::RightVector, FVector::UpVector).ToQuat();
			const FMuseeFrame InPort(FVector(-(Doors.SetBackMeters + Doors.LeafMeters), 0.0, 0.0), Turn);
			return Port.Frame.FrameToParent(InPort);
		}
	}

	TValueOrError<FSiteBlindDoors, FString> ReadBlindDoors(const FString& Path)
	{
		return ReadRecord<FSiteBlindDoors>(Path);
	}

	bool Blind(const FSitePort& Port, const FSitePortClass& Class, const FSiteBlindDoors& Doors, TArray<FFormDrawing>& OutDrawings,
		FString& OutWhy)
	{
		const double Gap = Doors.ClearanceMeters;
		const bool bFits = Class.ClearWidthMeters > 4.0 * Gap && Class.HeadMeters > 2.0 * Gap && Doors.LeafMeters > 0.0
			&& Doors.SetBackMeters + Doors.LeafMeters <= Port.WallDepthMeters;
		if (!bFits)
		{
			OutWhy = FString::Printf(TEXT("the blind's leaves don't fit port %s: its opening is too small or its wall too thin"), *Port.Key.ToString());
			return false;
		}
		const FString PortName = Port.Key.Segments().Last();
		for (const double Side : {-1.0, 1.0})
		{
			FFormExtrusion Leaf;
			Leaf.Outline = LeafOutline(Class, Doors, Side);
			Leaf.DepthMeters = Doors.LeafMeters;
			FFormDrawing& Drawing = OutDrawings.Add_GetRef(FFormDrawing::Of(
				FName(FString::Printf(TEXT("blind.%s.%s"), *PortName, Side < 0.0 ? TEXT("left") : TEXT("right"))), LeafSurface, MoveTemp(Leaf)));
			Drawing.Material = Doors.Material;
			Drawing.Working = Doors.Working;
			Drawing.History = Doors.History;
			Drawing.Collision = Musee::Collision::Solid.Profile();
			Drawing.Frame = LeafFrame(Port, Doors);
			Drawing.Drawer = TEXT("Site");
		}
		return true;
	}
}
