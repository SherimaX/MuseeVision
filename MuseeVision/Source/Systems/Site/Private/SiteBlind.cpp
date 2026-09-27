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

	namespace
	{
		/**
		 * One iris blade's outline in its pin's frame, open: the inner arc of the ring out along the blade, its far end's
		 * half-circle, the outer arc back, and the half-circle at the pin's end, as the Élan kit's blades are cut
		 * (windows/Source/MuseeVision/Elan/ElanSphereTop.cpp:54-92).
		 */
		FFormProfile BladeOutline(const FSiteIris& Iris)
		{
			const double Mid = Iris.MidRadiusMeters;
			const double Half = 0.5 * Iris.WidthMeters;
			const double Arc = Iris.ArcDegrees;
			const FVector2D Axis(-(Mid + Iris.PinOutsetMeters), 0.0);
			auto OnMid = [&](double Degrees)
			{
				const double Radians = FMath::DegreesToRadians(Degrees);
				return Axis + Mid * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
			};
			return FFormProfile::From(Axis + FVector2D(Mid - Half, 0.0))
				.ArcAbout(Axis, Mid - Half, Mid - Half, 0.0, Arc, EFormJoint::Smooth)
				.ArcAbout(OnMid(Arc), Half, Half, Arc + 180.0, Arc, EFormJoint::Smooth)
				.ArcAbout(Axis, Mid + Half, Mid + Half, Arc, 0.0, EFormJoint::Smooth)
				.ArcAbout(OnMid(0.0), Half, Half, 0.0, -180.0, EFormJoint::Smooth)
				.Close();
		}

		/**
		 * The iris shut on the port's plane: each blade on its pin round the port's axis (pin 0 due east), turned its swing
		 * from open, the top blade's face on the plane and the rest stacked below it at the pitch.
		 */
		void ShutIris(const FSitePort& Port, const FSiteIris& Iris, const FSiteBlindDoors& Doors, TArray<FFormDrawing>& OutDrawings)
		{
			const FString PortName = Port.Key.Segments().Last();
			const double PinRadius = Iris.MidRadiusMeters + Iris.PinOutsetMeters;
			for (int32 Blade = 0; Blade < Iris.Blades; ++Blade)
			{
				const double Around = 360.0 * Blade / Iris.Blades;
				const double Radians = FMath::DegreesToRadians(Around);
				const double Top = -Iris.PitchMeters * Blade;
				const FVector Pin = Port.Frame.OriginMeters + FVector(PinRadius * FMath::Cos(Radians), PinRadius * FMath::Sin(Radians), Top - Iris.BladeMeters);
				FFormExtrusion Shape;
				Shape.Outline = BladeOutline(Iris);
				Shape.DepthMeters = Iris.BladeMeters;
				FFormDrawing& Drawing = OutDrawings.Add_GetRef(FFormDrawing::Of(FName(FString::Printf(TEXT("blind.%s.iris.%d"), *PortName, Blade)),
					TEXT("blind.iris"), MoveTemp(Shape)));
				Drawing.Material = Doors.IrisMaterial;
				Drawing.Working = Doors.IrisWorking;
				Drawing.History = Doors.IrisHistory;
				Drawing.Collision = Musee::Collision::Solid.Profile();
				Drawing.Frame = FMuseeFrame(Pin, FRotator(0.0, Around + Iris.SwingDegrees, 0.0).Quaternion());
				Drawing.Drawer = TEXT("Site");
			}
		}
	}

	TValueOrError<FSiteBlindDoors, FString> ReadBlindDoors(const FString& Path)
	{
		return ReadRecord<FSiteBlindDoors>(Path);
	}

	bool Blind(const FSitePort& Port, const FSitePortClass& Class, const FSiteBlindDoors& Doors, TArray<FFormDrawing>& OutDrawings,
		FString& OutWhy)
	{
		const double Upward = Port.Facing().Z;
		if (Class.IsIris() || FMath::Abs(Upward) > 0.5)
		{
			if (!Class.IsIris() || !Class.Iris.IsStacked() || FMath::Abs(Upward) < 0.5)
			{
				OutWhy = FString::Printf(TEXT("port %s: a vertical port is closed by an iris, stacked in its plane"), *Port.Key.ToString());
				return false;
			}
			// Facing up, the port's own stage lies below its plane and shuts it with its own iris.
			if (Upward < 0.0)
			{
				ShutIris(Port, Class.Iris, Doors, OutDrawings);
			}
			return true;
		}
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
