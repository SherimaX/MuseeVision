#include "Schedule/LightDresser.h"

#include "GameFramework/Actor.h"
#include "LightImage.h"
#include "LightLuminaire.h"
#include "LightLuminaireComponent.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "Schedule/LightingRecords.h"
#include "UObject/Package.h"

namespace Musee::Light
{
	namespace
	{
		const TCHAR* const ScheduleFile = TEXT("Lighting.json");
		const FName MountKind(TEXT("Mount"));
		/** Whose findings these are: Light's, where the fix belongs. */
		const FName LightSystem(TEXT("Light"));

		/** The tags a luminaire set in a piece carries: `musee.host:<surface>`, and `musee.lights:<surface>` for its target. */
		const TCHAR* const HostTagPrefix = TEXT("musee.host:");
		const TCHAR* const LitSurfaceTagPrefix = TEXT("musee.lights:");

		/** A full turn, in degrees of bearing. */
		constexpr double FullTurnDegrees = 360.0;

		/** The places whose weather may fall on an opening (Decision D-005, the stage contract). */
		const TCHAR* const WeatherPlaces[] = { TEXT("museum"), TEXT("giverny"), TEXT("london") };

		/** Where the stage's schedule is: its folder as given, or under Data/ or the project. */
		FString FindSchedule(const FString& Folder)
		{
			const FString Candidates[] = {
				Folder / ScheduleFile,
				Records::DataRoot() / Folder / ScheduleFile,
				FPaths::ProjectDir() / Folder / ScheduleFile,
			};
			for (const FString& Candidate : Candidates)
			{
				if (!FPaths::IsRelative(Candidate) && FPaths::FileExists(Candidate)) { return FPaths::ConvertRelativePathToFull(Candidate); }
			}
			return FString();
		}

		/** One luminaire of the schedule, checked against the stage and the catalogue. */
		struct FPlacement
		{
			FMuseeKey Type;
			/** Where it sits: its Mount anchor's frame, or the stage's axes at its point. */
			FMuseeFrame Seat;
			/** The point it is aimed at, in the stage's frame, if the schedule gives one. */
			TOptional<FVector> Aim;
			/** For a linear luminaire, the direction its run lies in, square to its light. */
			TOptional<FVector> Run;
			/** For an areal luminaire, the face it lights, [width, height] in metres. */
			FVector2D Face = FVector2D::ZeroVector;
			/** For a zone, the direction the face's height runs in: out from the axis, through the zone's middle. */
			TOptional<FVector> FaceHeight;
			/**
			 * The sphere whose inside the face is, [radius, opening], when it encloses the luminaire (a sphere's inside about
			 * its seat), its light converging on the seat; zero otherwise.
			 */
			FVector2D Sphere = FVector2D::ZeroVector;
			bool bEncloses = false;
			ELightOperation Operation = ELightOperation::Always;
			FMuseeKey Follows;
			USceneComponent* Body = nullptr;
		};

		TOptional<ELightOperation> ParseOperation(const FString& Text)
		{
			if (Text == TEXT("always")) { return ELightOperation::Always; }
			if (Text == TEXT("dusk")) { return ELightOperation::Dusk; }
			if (Text == TEXT("presence")) { return ELightOperation::Presence; }
			return {};
		}

		USceneComponent* FindBody(const AActor& StageActor, const FString& Body)
		{
			TArray<USceneComponent*> Components;
			StageActor.GetComponents(Components);
			const FName Tag(*Body);
			USceneComponent* const* Found = Components.FindByPredicate([Tag](const USceneComponent* C) { return C->ComponentHasTag(Tag); });
			return Found ? *Found : nullptr;
		}

		/** Three numbers as a point, or nothing. */
		TOptional<FVector> PointOf(const TArray<double>& Numbers)
		{
			return Numbers.Num() == 3 ? TOptional<FVector>(FVector(Numbers[0], Numbers[1], Numbers[2])) : TOptional<FVector>();
		}

		/**
		 * Where the entry sits and what it is aimed at (the stage contract): on a Mount anchor or at a point in the stage's
		 * frame, never both, aimed at a point. At a point the aim is required, since a point has no axes of its own. Only a
		 * face that encloses its seat (a sphere's inside about it) is aimed at the point it sits on: its light converges
		 * there from all round. The reason it can't be seated, or empty.
		 */
		FString SeatOf(const FMuseeStageContext& Stage, const FLightScheduleLuminaire& Entry, FPlacement& Out)
		{
			const bool bMount = !Entry.Mount.IsEmpty();
			if (bMount == !Entry.Point.IsEmpty()) { return TEXT("it needs either a mount or a point, and not both"); }
			if (bMount)
			{
				const TOptional<FMuseeKey> Mount = FMuseeKey::Parse(Entry.Mount);
				const FMuseeStageAnchor* Anchor = Mount.IsSet() ? Stage.FindAnchor(Mount.GetValue(), MountKind) : nullptr;
				if (!Anchor) { return FString::Printf(TEXT("the stage has no Mount anchor '%s'"), *Entry.Mount); }
				Out.Seat = Anchor->Frame;
			}
			else
			{
				const TOptional<FVector> Point = PointOf(Entry.Point);
				if (!Point.IsSet()) { return TEXT("its point isn't [x, y, z]"); }
				Out.Seat = FMuseeFrame(Point.GetValue());
			}
			if (!Entry.Aim.IsEmpty())
			{
				Out.Aim = PointOf(Entry.Aim);
				if (!Out.Aim.IsSet()) { return TEXT("its aim isn't a point [x, y, z]"); }
				const bool bAtSeat = (Out.Aim.GetValue() - Out.Seat.OriginMeters).IsNearlyZero();
				if (bAtSeat != Entry.Sphere.IsGiven())
				{
					return bAtSeat ? TEXT("it is aimed at the point it sits on") : TEXT("a sphere's face is aimed at its centre, the point it sits on");
				}
			}
			if (!bMount && !Out.Aim.IsSet()) { return TEXT("a luminaire at a point needs its aim"); }
			return FString();
		}

		/** The direction its light leaves in: towards its aim, else along its seat's x axis. */
		FVector LightOf(const FPlacement& Placement)
		{
			const FMuseeFrame& Seat = Placement.Seat;
			return Placement.Aim.IsSet() ? (Placement.Aim.GetValue() - Seat.OriginMeters).GetSafeNormal() : Seat.Rotation.GetAxisX();
		}

		/**
		 * A linear luminaire's run (the stage contract): the schedule's along, else its mount's x axis on a mount, else the
		 * stage's x axis, taken square to its light, since a strip's light leaves its face across its length. Only a linear
		 * type is laid along a direction. The reason it has no run, or empty.
		 */
		FString RunOf(const FLightScheduleLuminaire& Entry, const FLightLuminaire& Luminaire, FPlacement& Out)
		{
			if (Luminaire.Emitter != FLightLuminaire::EEmitter::Linear)
			{
				return Entry.Along.IsEmpty() ? FString() : TEXT("only a linear type is laid along a direction");
			}
			const FVector Default = Entry.Mount.IsEmpty() ? FVector::ForwardVector : Out.Seat.Rotation.GetAxisX();
			const TOptional<FVector> Along = Entry.Along.IsEmpty() ? TOptional<FVector>(Default) : PointOf(Entry.Along);
			if (!Along.IsSet() || Along.GetValue().IsNearlyZero()) { return TEXT("its along isn't a direction [x, y, z]"); }
			const FVector Light = LightOf(Out);
			const FVector Direction = Along.GetValue().GetSafeNormal();
			const FVector Square = Direction - (Direction | Light) * Light;
			if (Square.IsNearlyZero(UE_KINDA_SMALL_NUMBER)) { return TEXT("its run would lie along its own light; its along must cross it"); }
			Out.Run = Square.GetSafeNormal();
			return FString();
		}

		/** A bearing's direction on the plan: clockwise from north, which is -y. */
		FVector BearingDirection(double BearingDegrees)
		{
			const double Radians = FMath::DegreesToRadians(BearingDegrees);
			return FVector(FMath::Sin(Radians), -FMath::Cos(Radians), 0.0);
		}

		/**
		 * The face a zone lights: a rectangle of the zone's own area, as wide as the zone's middle arc (or its outer chord,
		 * for a zone wider than that), laid across the zone's middle bearing with its height running out from the axis. The
		 * reason it has none, or empty.
		 */
		FString ZoneFaceOf(const FLightScheduleZone& Zone, FPlacement& Out)
		{
			if (Zone.R.Num() != 2 || Zone.R[0] < 0.0 || Zone.R[1] <= Zone.R[0]) { return TEXT("its zone's radii aren't [inner, outer], inner less than outer"); }
			if (Zone.Bearing.Num() != 2) { return TEXT("its zone's bearings aren't [from, to]"); }
			const double RawSweep = FMath::Fmod(Zone.Bearing[1] - Zone.Bearing[0], FullTurnDegrees);
			const double SweepDegrees = RawSweep <= 0.0 ? RawSweep + FullTurnDegrees : RawSweep;
			const double Sweep = FMath::DegreesToRadians(SweepDegrees);
			const double Inner = Zone.R[0];
			const double Outer = Zone.R[1];
			const double Area = 0.5 * Sweep * (Outer * Outer - Inner * Inner);
			const double MiddleArc = 0.5 * Sweep * (Inner + Outer);
			const double OuterChord = 2.0 * Outer * FMath::Sin(0.5 * FMath::Min(Sweep, UE_DOUBLE_PI));
			const double Width = FMath::Min(MiddleArc, OuterChord);
			Out.Face = FVector2D(Width, Area / Width);
			Out.FaceHeight = BearingDirection(Zone.Bearing[0] + 0.5 * SweepDegrees);
			const FVector Plan(Out.Seat.OriginMeters.X, Out.Seat.OriginMeters.Y, 0.0);
			const double Radius = Plan.Size();
			double Bearing = FMath::RadiansToDegrees(FMath::Atan2(Plan.X, -Plan.Y));
			Bearing = FMath::Fmod(Bearing - Zone.Bearing[0] + 2.0 * FullTurnDegrees, FullTurnDegrees);
			if (Radius < Inner || Radius > Outer || Bearing > SweepDegrees + UE_KINDA_SMALL_NUMBER)
			{
				return TEXT("it sits outside its own zone");
			}
			return FString();
		}

		/**
		 * The face a sphere's inside is, about the luminaire's seat: the whole sphere less the cap its opening cuts at its
		 * foot, 2 pi R (R + sqrt(R^2 - r^2)) for an opening of radius r, as the rectangle of its own area as wide as the
		 * sphere's equator. Its light converges on the seat from every side. The reason it has none, or empty.
		 */
		FString SphereFaceOf(const FLightScheduleSphere& Sphere, FPlacement& Out)
		{
			const double Radius = Sphere.Radius;
			const double Opening = Sphere.Opening;
			if (Radius <= 0.0 || Opening < 0.0 || Opening >= Radius) { return TEXT("its sphere's radius isn't more than zero, with an opening of zero or more inside it"); }
			const double Equator = 2.0 * UE_DOUBLE_PI * Radius;
			const double Area = Equator * (Radius + FMath::Sqrt(Radius * Radius - Opening * Opening));
			Out.Face = FVector2D(Equator, Area / Equator);
			Out.bEncloses = true;
			Out.Sphere = FVector2D(Radius, Opening);
			return FString();
		}

		/**
		 * The face an areal type lights (the stage contract): its extent, its zone or its sphere, exactly one; only an areal
		 * type is laid over a face. The reason it has none, or empty.
		 */
		FString FaceOf(const FLightScheduleLuminaire& Entry, const FLightLuminaire& Luminaire, FPlacement& Out)
		{
			const bool bExtent = !Entry.Extent.IsEmpty();
			const bool bZone = !Entry.Zone.R.IsEmpty() || !Entry.Zone.Bearing.IsEmpty();
			const bool bSphere = Entry.Sphere.IsGiven();
			if (!Luminaire.IsAreal())
			{
				return bExtent || bZone || bSphere ? TEXT("only an areal type (a luminous ceiling, edge-lit panes, an image panel) is laid over an extent, a zone or a sphere") : FString();
			}
			if (int32(bExtent) + int32(bZone) + int32(bSphere) != 1)
			{
				return TEXT("an areal type needs one of the extent, the zone or the sphere of the face it lights");
			}
			if (bSphere)
			{
				// Only a picture can be shown over a sphere's inside, on the surface Light makes for it.
				return Luminaire.Emitter == FLightLuminaire::EEmitter::Image ? SphereFaceOf(Entry.Sphere, Out)
					: TEXT("only an image panel is laid over a sphere, its picture shown on the sphere's inside");
			}
			if (bZone) { return ZoneFaceOf(Entry.Zone, Out); }
			if (Entry.Extent.Num() != 2 || Entry.Extent[0] <= 0.0 || Entry.Extent[1] <= 0.0) { return TEXT("its extent isn't [width, height] in metres"); }
			Out.Face = FVector2D(Entry.Extent[0], Entry.Extent[1]);
			return FString();
		}

		/** Checks one entry; the reason it can't be placed, or empty. */
		FString Check(const FMuseeStageContext& Stage, const FLightScheduleLuminaire& Entry, FPlacement& Out)
		{
			const TOptional<FMuseeKey> Type = FMuseeKey::Parse(Entry.Type);
			const FLightLuminaire* Luminaire = Type.IsSet() ? FindLuminaire(Type.GetValue()) : nullptr;
			if (!Luminaire) { return FString::Printf(TEXT("its type '%s' isn't in the catalogue"), *Entry.Type); }
			Out.Type = Type.GetValue();
			const FString WhyUnseated = SeatOf(Stage, Entry, Out);
			if (!WhyUnseated.IsEmpty()) { return WhyUnseated; }
			const TOptional<ELightOperation> Operation = ParseOperation(Entry.Operation);
			if (!Operation.IsSet()) { return FString::Printf(TEXT("its operation '%s' is none of always, dusk and presence"), *Entry.Operation); }
			Out.Operation = Operation.GetValue();
			if (!Entry.Follows.IsEmpty())
			{
				const TOptional<FMuseeKey> Follows = FMuseeKey::Parse(Entry.Follows);
				if (!Follows.IsSet()) { return FString::Printf(TEXT("the channel it follows, '%s', isn't a key"), *Entry.Follows); }
				Out.Follows = Follows.GetValue();
			}
			if (!Entry.Body.IsEmpty())
			{
				Out.Body = FindBody(*Stage.StageActor, Entry.Body);
				if (!Out.Body) { return FString::Printf(TEXT("the stage has no moving body '%s'"), *Entry.Body); }
			}
			if (Luminaire->Emitter == FLightLuminaire::EEmitter::Linear && Entry.Length <= 0.0) { return TEXT("a linear type needs the length laid"); }
			const FString WhyNoFace = FaceOf(Entry, *Luminaire, Out);
			if (!WhyNoFace.IsEmpty()) { return WhyNoFace; }
			return RunOf(Entry, *Luminaire, Out);
		}

		/**
		 * The luminaire's frame in the stage: where it sits, its light leaving along x towards its aim. A linear luminaire
		 * lies along the frame's y axis, its run; a zone's face has its height along z, out from the axis; a face that
		 * encloses its seat keeps its seat's frame, its light arriving from every side; any other keeps its seat's z up (its
		 * seat's y, when it is aimed straight along z).
		 */
		FMuseeFrame FrameOf(const FPlacement& Placement)
		{
			const FMuseeFrame& Seat = Placement.Seat;
			if (Placement.bEncloses)
			{
				return Seat;
			}
			const FVector Light = LightOf(Placement);
			if (Placement.Run.IsSet())
			{
				return FMuseeFrame(Seat.OriginMeters, FRotationMatrix::MakeFromXY(Light, Placement.Run.GetValue()).ToQuat());
			}
			if (Placement.FaceHeight.IsSet() && FMath::Abs(Light | Placement.FaceHeight.GetValue()) < 1.0 - UE_KINDA_SMALL_NUMBER)
			{
				return FMuseeFrame(Seat.OriginMeters, FRotationMatrix::MakeFromXZ(Light, Placement.FaceHeight.GetValue()).ToQuat());
			}
			const FVector Up = FMath::Abs(Light | Seat.Rotation.GetAxisZ()) < 1.0 - UE_KINDA_SMALL_NUMBER ? Seat.Rotation.GetAxisZ() : Seat.Rotation.GetAxisY();
			return FMuseeFrame(Seat.OriginMeters, FRotationMatrix::MakeFromXZ(Light, Up).ToQuat());
		}

		void Place(const FMuseeStageContext& Stage, const FLightScheduleLuminaire& Entry, const FPlacement& Placement)
		{
			AActor* Actor = Stage.StageActor;
			USceneComponent* Root = Actor->GetRootComponent();
			USceneComponent* Parent = Placement.Body ? Placement.Body : Root;
			const FName Name = MakeUniqueObjectName(Actor, ULightLuminaireComponent::StaticClass(), FName(*(TEXT("Luminaire_") + Entry.Id.Replace(TEXT("."), TEXT("_")))));
			ULightLuminaireComponent* Component = NewObject<ULightLuminaireComponent>(Actor, Name);
			Component->Configure(Placement.Type, Placement.Operation, Placement.Follows, Meters(Entry.Length), Placement.Face, Placement.Sphere);
			Component->ComponentTags.Add(FName(FString(LuminaireTagPrefix) + Entry.Id));
			if (!Entry.Host.IsNone()) { Component->ComponentTags.Add(FName(FString(HostTagPrefix) + Entry.Host.ToString())); }
			if (!Entry.Surface.IsNone()) { Component->ComponentTags.Add(FName(FString(LitSurfaceTagPrefix) + Entry.Surface.ToString())); }
			const FTransform InStage = ToEngine(FrameOf(Placement)) * Root->GetComponentTransform();
			Component->SetupAttachment(Parent);
			Component->SetRelativeTransform(InStage.GetRelativeTransform(Parent->GetComponentTransform()));
			Actor->AddInstanceComponent(Component);
			Component->RegisterComponent();
		}
	}

	void DressStage(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		const FName Drawer(*Stage.Stage.ToString());
		const auto Find = [&OutFindings, Drawer](const FString& Summary)
		{
			FMuseeFinding& Found = OutFindings.Add_GetRef(FMuseeFinding::Make(Drawer, Summary, FMuseeMeasurement()));
			Found.Owner = LightSystem;
		};
		const FString Path = FindSchedule(Stage.ScheduleDir);
		if (Path.IsEmpty() || !Stage.StageActor || !Stage.StageActor->GetRootComponent())
		{
			if (!Path.IsEmpty()) { Find(TEXT("Light can't dress a stage without its stage actor.")); }
			return;
		}
		TValueOrError<FLightSchedule, FString> Read = ReadRecord<FLightSchedule>(Path);
		if (Read.HasError())
		{
			Find(FString::Printf(TEXT("The lighting schedule can't be read: %s"), *Read.GetError()));
			return;
		}
		const FLightSchedule& Schedule = Read.GetValue();
		if (Schedule.Stage != Stage.Stage.ToString())
		{
			Find(FString::Printf(TEXT("The lighting schedule at %s is stage '%s''s, not '%s''s."), *Path, *Schedule.Stage, *Stage.Stage.ToString()));
		}
		for (const FLightScheduleDaylight& Daylight : Schedule.Daylight)
		{
			if (!TArrayView<const TCHAR* const>(WeatherPlaces).ContainsByPredicate([&Daylight](const TCHAR* Place) { return Daylight.Weather == Place; }))
			{
				Find(FString::Printf(TEXT("Opening '%s' takes the weather of '%s', which isn't one of Time's places."), *Daylight.Opening, *Daylight.Weather));
			}
		}
		for (const FLightScheduleLuminaire& Entry : Schedule.Luminaires)
		{
			FPlacement Placement;
			const FString Why = Check(Stage, Entry, Placement);
			if (!Why.IsEmpty())
			{
				Find(FString::Printf(TEXT("Luminaire '%s' isn't placed: %s."), *Entry.Id, *Why));
				continue;
			}
			const FLightLuminaire* Luminaire = FindLuminaire(Placement.Type);
			if (!Luminaire->bLit)
			{
				Find(FString::Printf(TEXT("Luminaire '%s' stands unlit: %s."), *Entry.Id, *Luminaire->WhyUnlit));
			}
			else if (Luminaire->Emitter == FLightLuminaire::EEmitter::Profile && !FindProfile(*Luminaire))
			{
				Find(FString::Printf(TEXT("Luminaire '%s' stands unlit: its light profile %s hasn't been made from %s (Studio's make makes it)."),
					*Entry.Id, *ProfilePackage(*Luminaire), *FPaths::GetCleanFilename(Luminaire->PhotometricFile)));
			}
			Place(Stage, Entry, Placement);
			if (Placement.bEncloses && Luminaire->bLit && !Image::FindMaterial())
			{
				Find(FString::Printf(TEXT("Luminaire '%s' stands dark: Light's image material %s, which its sphere's surface shows the sky in, hasn't been made (Studio's make makes it)."),
					*Entry.Id, *Image::MaterialPackage()));
			}
		}
	}
}
