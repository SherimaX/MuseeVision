#include "SiteJoint.h"

#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "SiteJointFaces.h"
#include "SitePlan.h"

/**
 * The joint is drawn in its own frame, J: its origin on A's port frame, x along A's facing towards B, y across the
 * opening, z up. Every face it meets is a vertical plane or a cylinder whose axis lies on the opening's centreline
 * (FSitePort), so each is a curve x(y) in J's plan, and the threshold, strips, linings and masonry are the regions
 * between such curves. An arched lining is laid in voussoirs, each run straight through from face to face with its ends
 * chamfered on the faces' tangent planes at its middle.
 */
namespace Musee::Site
{
	namespace
	{
		/**
		 * A voussoir's chamfered end leaves a curved face by at most this, metres: the museum's chord tolerance, 1 mm, the
		 * one number Form divides every curve by (MuseeFormMaking's Stations.h).
		 */
		constexpr double FaceToleranceMeters = 0.001;

		/** The surround's face line reaches this far past the surround on either side, so its ends lie clear of it, metres. */
		constexpr double FaceMarginMeters = 0.100;

		/** The door's arch is set out at 64 stations per semicircle (the Salon's and the oval's doors, salon.md section 2). */
		constexpr int32 ArchStations = 64;

		/** What a joint draws, in J, and the record and class it draws them from. */
		struct FJointDraft
		{
			const FSitePortClass& Class;
			const FSiteJointRecord& Record;
			FMuseeFrame Frame;
			FString Prefix;
			FJointFace FaceA;
			FJointFace InnerA;
			FJointFace FaceB;
			FJointFace InnerB;
			double Passage = 0.0;
			TArray<FFormDrawing>& Out;

			double Half() const { return 0.5 * Class.ClearWidthMeters; }
			double Rough() const { return Half() + Class.AllowanceMeters; }

			FFormDrawing& Add(const FString& Part, const FSiteJointFinish& Finish, FFormShape Shape, const Musee::Collision::FClass& Collision,
				const FMuseeFrame& Local = FMuseeFrame())
			{
				FFormDrawing& Drawing = Out.AddDefaulted_GetRef();
				Drawing.Name = FName(Prefix + TEXT(".") + Part);
				Drawing.Surface = FName(TEXT("joint"));
				Drawing.Material = Finish.Material;
				Drawing.Working = Finish.Working;
				Drawing.History = Finish.History;
				Drawing.Collision = Collision.Profile();
				Drawing.Frame = Frame.FrameToParent(Local);
				Drawing.Drawer = TEXT("Site");
				Drawing.Shape = MoveTemp(Shape);
				return Drawing;
			}
		};

		/** A slab between two faces over y in [-Across, Across], from Bottom up Depth. */
		FFormShape SlabBetween(const FJointFace& Near, const FJointFace& Far, double Low, double High, double Depth)
		{
			FFormExtrusion Slab;
			Slab.Outline = Between(Near, Far, Low, High);
			Slab.DepthMeters = Depth;
			FFormShape Shape;
			Shape.Set<FFormExtrusion>(MoveTemp(Slab));
			return Shape;
		}

		/** The faces the strips lie on: the mating face where the two meet, or each port face across a passage. */
		TArray<FJointFace> StripFaces(const FJointDraft& Draft)
		{
			if (Draft.Passage > FaceToleranceMeters)
			{
				return {Draft.FaceA, Draft.FaceB};
			}
			return {Draft.FaceA.SameAs(Draft.FaceB) ? Draft.FaceA : FJointFace::Plane(0.0)};
		}

		/** The threshold through both walls and any passage, and the bronze strips set in it flush with the floors. */
		void Threshold(FJointDraft& Draft)
		{
			const FSitePortClass& Class = Draft.Class;
			const double Rough = Draft.Rough();
			TArray<FJointFace> Cuts = {Draft.InnerA};
			int32 Strip = 0;
			for (const FJointFace& Face : Class.StripMeters > 0.0 ? StripFaces(Draft) : TArray<FJointFace>())
			{
				const FJointFace Near = Face.Offset(-0.5 * Class.StripMeters);
				const FJointFace Far = Face.Offset(0.5 * Class.StripMeters);
				Draft.Add(FString::Printf(TEXT("strip.%d"), Strip++), Draft.Record.Strip, SlabBetween(Near, Far, -Rough, Rough, Draft.Record.StripDepthMeters),
					Musee::Collision::Walkable, FMuseeFrame(FVector(0.0, 0.0, -Draft.Record.StripDepthMeters)));
				Cuts.Add(Near);
				Cuts.Add(Far);
			}
			Cuts.Add(Draft.InnerB);
			if (Class.AllowanceMeters <= 0.0)
			{
				return;
			}
			for (int32 Index = 0; Index + 1 < Cuts.Num(); Index += 2)
			{
				Draft.Add(FString::Printf(TEXT("threshold.%d"), Index / 2), Draft.Record.Threshold, SlabBetween(Cuts[Index], Cuts[Index + 1], -Rough, Rough,
					Draft.Record.ThresholdMeters), Musee::Collision::Walkable, FMuseeFrame(FVector(0.0, 0.0, -Draft.Record.ThresholdMeters)));
			}
		}

		/** One voussoir of an arched lining: the annular sector from From to To round the arch, run from face to face. */
		void Voussoir(FJointDraft& Draft, int32 Index, double From, double To)
		{
			const double Radius = Draft.Half();
			const double Outer = Draft.Rough();
			const double Head = Draft.Class.HeadMeters;
			const double Across = 0.5 * (Radius + Outer) * FMath::Cos(0.5 * (From + To));
			const FVector2D Centre(-Across, Head);
			const double FromDeg = FMath::RadiansToDegrees(From);
			const double ToDeg = FMath::RadiansToDegrees(To);
			FFormSweep Shape;
			Shape.Section = FFormProfile::From(Centre + Radius * FVector2D(FMath::Cos(To), FMath::Sin(To)))
				.ArcAbout(Centre, Radius, Radius, ToDeg, FromDeg, EFormJoint::Smooth)
				.LineTo(Centre + Outer * FVector2D(FMath::Cos(From), FMath::Sin(From)), EFormJoint::Crisp)
				.ArcAbout(Centre, Outer, Outer, FromDeg, ToDeg, EFormJoint::Crisp).Close();
			Shape.Path = FFormProfile::Polyline({FVector2D(Draft.InnerA.XAt(Across), Across), FVector2D(Draft.InnerB.XAt(Across), Across)}, false);
			const FVector2D StartNormal = Draft.InnerA.NormalAt(Across);
			const FVector2D EndNormal = Draft.InnerB.NormalAt(Across);
			Shape.StartCut = FVector(StartNormal.X, StartNormal.Y, 0.0);
			Shape.EndCut = FVector(EndNormal.X, EndNormal.Y, 0.0);
			FFormShape Voussoir;
			Voussoir.Set<FFormSweep>(MoveTemp(Shape));
			Draft.Add(FString::Printf(TEXT("lining.voussoir.%02d"), Index), Draft.Record.Lining, MoveTemp(Voussoir), Musee::Collision::Solid);
		}

		/**
		 * The linings in front of both rough openings, through from A's inner face to B's: the jambs standing on the
		 * threshold, and the head, a lintel stone or an arch of voussoirs short enough that each end's chamfer keeps to a
		 * curved face.
		 */
		void Linings(FJointDraft& Draft)
		{
			const FSitePortClass& Class = Draft.Class;
			const double Half = Draft.Half();
			const double Rough = Draft.Rough();
			Draft.Add(TEXT("lining.jamb.north"), Draft.Record.Lining, SlabBetween(Draft.InnerA, Draft.InnerB, -Rough, -Half, Class.HeadMeters), Musee::Collision::Solid);
			Draft.Add(TEXT("lining.jamb.south"), Draft.Record.Lining, SlabBetween(Draft.InnerA, Draft.InnerB, Half, Rough, Class.HeadMeters), Musee::Collision::Solid);
			if (!Class.Arched)
			{
				Draft.Add(TEXT("lining.head"), Draft.Record.Lining, SlabBetween(Draft.InnerA, Draft.InnerB, -Rough, Rough, Class.AllowanceMeters),
					Musee::Collision::Solid, FMuseeFrame(FVector(0.0, 0.0, Class.HeadMeters)));
				return;
			}
			// A chamfered end leaves a face of radius R by w^2 / (8 R) across a voussoir w wide.
			const double Tightest = FMath::Max(Draft.InnerA.Tightness(), Draft.InnerB.Tightness());
			const double Widest = Tightest > 0.0 ? FMath::Sqrt(8.0 * FaceToleranceMeters / Tightest) : TNumericLimits<double>::Max();
			const int32 Stones = FMath::Max(FMath::CeilToInt32(UE_DOUBLE_PI * Rough / Draft.Record.LiningStoneMeters), FMath::CeilToInt32(UE_DOUBLE_PI * Rough / Widest));
			for (int32 Stone = 0; Stone < Stones; ++Stone)
			{
				Voussoir(Draft, Stone, UE_DOUBLE_PI * Stone / Stones, UE_DOUBLE_PI * (Stone + 1) / Stones);
			}
		}

		/**
		 * The masonry closing the gap between the two port faces within the collar envelope, round the rough opening: a wall
		 * between two faces, its face on B's port face and its back on A's, a straight passage through it.
		 */
		bool Masonry(FJointDraft& Draft, FString& OutWhy)
		{
			const FSitePortClass& Class = Draft.Class;
			const double Collar = Class.CollarHalfWidthMeters;
			if (Collar <= 0.0 || (Draft.Passage <= FaceToleranceMeters && Draft.FaceA.SameAs(Draft.FaceB)))
			{
				return true;
			}
			if (!Draft.FaceA.LiesBefore(Draft.FaceB, Collar))
			{
				OutWhy = TEXT("the two port faces cross within the collar envelope");
				return false;
			}
			FFormWallOpening Rough;
			Rough.CenterMeters = Draft.FaceB.AlongFrom(-Collar, 0.0);
			Rough.WidthMeters = 2.0 * Draft.Rough();
			Rough.SillMeters = -Class.AllowanceMeters - Class.CollarBottomMeters;
			Rough.HeadMeters = Class.HeadMeters + (Class.Arched ? 0.0 : Class.AllowanceMeters) - Class.CollarBottomMeters;
			Rough.bArched = Class.Arched;
			Rough.bStraight = true;
			FFormWall Wall;
			Wall.Plan = Draft.FaceB.Line(-Collar, Collar);
			Wall.Back = Draft.FaceA.Line(-Collar - FaceMarginMeters, Collar + FaceMarginMeters);
			Wall.BaseMeters = Class.CollarBottomMeters;
			Wall.HeightMeters = Class.CollarTopMeters - Class.CollarBottomMeters;
			Wall.Openings.Add(Rough);
			FFormShape Shape;
			Shape.Set<FFormWall>(MoveTemp(Wall));
			Draft.Add(TEXT("masonry"), Draft.Record.Masonry, MoveTemp(Shape), Musee::Collision::Solid);
			return true;
		}

		/**
		 * The surround on a stage's inner face round the clear opening, drawn on the face in its development: up one jamb,
		 * over the head and down the other, its section out from the opening. Toward is +1 for B's face, which looks along
		 * +x, and -1 for A's.
		 */
		void Surround(FJointDraft& Draft, const FJointFace& Face, double Toward, const TCHAR* Side)
		{
			const FSitePortClass& Class = Draft.Class;
			const double Half = Draft.Half();
			const double Reach = Half + Class.SurroundMeters + FaceMarginMeters;
			// The face's line runs so the face looks into its room, to the line's left: -y for A, +y for B.
			const double Start = -Toward * Reach;
			auto Along = [&Face, Start](double Y) { return Face.AlongFrom(Start, Y); };
			const double First = Start > 0.0 ? Half : -Half;
			TArray<FVector2D> Points = {FVector2D(Along(First), 0.0)};
			if (Class.Arched)
			{
				for (int32 Station = 0; Station <= ArchStations; ++Station)
				{
					const double Angle = UE_DOUBLE_PI * Station / ArchStations;
					Points.Add(FVector2D(Along(First * FMath::Cos(Angle)), Class.HeadMeters + Half * FMath::Sin(Angle)));
				}
			}
			else
			{
				Points.Append({FVector2D(Along(First), Class.HeadMeters), FVector2D(Along(-First), Class.HeadMeters)});
			}
			Points.Add(FVector2D(Along(-First), 0.0));
			FFormProfile Path = FFormProfile::From(Points[0]);
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				Path.LineTo(Points[Index], Class.Arched ? EFormJoint::Smooth : EFormJoint::Crisp);
			}
			FFormSweep Shape;
			Shape.Section = Draft.Record.SurroundSection;
			Shape.Face = Face.Line(Start, -Start);
			Shape.Path = MoveTemp(Path);
			FFormShape Surround;
			Surround.Set<FFormSweep>(MoveTemp(Shape));
			Draft.Add(FString::Printf(TEXT("surround.%s"), Side), Draft.Record.Surround, MoveTemp(Surround), Musee::Collision::Solid);
		}

		/** J: on A's port frame, x along A's facing, z up. */
		FMuseeFrame JointFrame(const FSitePort& A)
		{
			const FVector X = A.Facing();
			return FMuseeFrame(A.Frame.OriginMeters, FRotationMatrix::MakeFromXZ(X, FVector::UpVector).ToQuat());
		}

		/** Whether two placed ports meet as the class says: facing each other, their frames the passage apart along A's facing. */
		bool Meet(const FSitePort& A, const FSitePort& B, double Passage, FString& OutWhy)
		{
			const FVector Expected = A.Frame.OriginMeters + A.Facing() * Passage;
			if (!B.Frame.OriginMeters.Equals(Expected, LoopToleranceMeters) || FVector::DotProduct(A.Facing(), B.Facing()) > -FMath::Cos(FMath::DegreesToRadians(FacingToleranceDegrees)))
			{
				OutWhy = FString::Printf(TEXT("%s and %s don't meet across a passage of %.3f m"), *A.Key.ToString(), *B.Key.ToString(), Passage);
				return false;
			}
			return true;
		}
	}

	TValueOrError<FSiteJointRecord, FString> ReadJoints(const FString& Path)
	{
		return ReadRecord<FSiteJointRecord>(Path);
	}

	FSitePort Placed(const FSitePort& Port, const FVector& TranslationMeters)
	{
		FSitePort Out = Port;
		Out.Frame.OriginMeters += TranslationMeters;
		Out.Well.MinMeters += TranslationMeters;
		Out.Well.MaxMeters += TranslationMeters;
		return Out;
	}

	bool Joint(const FSitePort& A, const FSitePort& B, double PassageMeters, const FSitePortClass& Class, const FSiteJointRecord& Record,
		FSiteJoint& OutJoint, FString& OutWhy)
	{
		if (!(A.Class == Class.Key) || !(B.Class == Class.Key))
		{
			OutWhy = FString::Printf(TEXT("%s and %s aren't both of class %s"), *A.Key.ToString(), *B.Key.ToString(), *Class.Key.ToString());
			return false;
		}
		if (!Meet(A, B, PassageMeters, OutWhy))
		{
			return false;
		}
		if (FMath::Abs(A.Facing().Z) > 0.5)
		{
			// A vertical port's faces touch, and its iris is the lower stage's: there is nothing between them to draw.
			return true;
		}
		if (Class.Closure != FName(TEXT("open-portal")))
		{
			OutWhy = FString::Printf(TEXT("the joint of a %s closure isn't drawn yet"), *Class.Closure.ToString());
			return false;
		}
		const double Reach = 0.5 * Class.ClearWidthMeters + FMath::Max3(Class.AllowanceMeters, Class.SurroundMeters, Class.CollarHalfWidthMeters)
			+ FaceMarginMeters;
		FJointDraft Draft{Class, Record, JointFrame(A), TEXT("joint.") + A.Key.ToString(), FJointFace::OfPort(A, 0.0, 0.0, false),
			FJointFace::OfPort(A, A.WallDepthMeters, 0.0, false), FJointFace::OfPort(B, 0.0, PassageMeters, true),
			FJointFace::OfPort(B, B.WallDepthMeters, PassageMeters, true), PassageMeters, OutJoint.Drawings};
		for (const FJointFace* Face : {&Draft.FaceA, &Draft.InnerA, &Draft.FaceB, &Draft.InnerB})
		{
			if (!Face->Reaches(Reach))
			{
				OutWhy = FString::Printf(TEXT("a face of %s or %s turns too tight to hold the doorcase"), *A.Key.ToString(), *B.Key.ToString());
				return false;
			}
		}
		Threshold(Draft);
		if (Class.AllowanceMeters > 0.0)
		{
			Linings(Draft);
		}
		if (!Masonry(Draft, OutWhy))
		{
			return false;
		}
		if (!A.bOwnSurround)
		{
			Surround(Draft, Draft.InnerA, -1.0, TEXT("a"));
		}
		if (!B.bOwnSurround)
		{
			Surround(Draft, Draft.InnerB, 1.0, TEXT("b"));
		}
		return true;
	}
}
