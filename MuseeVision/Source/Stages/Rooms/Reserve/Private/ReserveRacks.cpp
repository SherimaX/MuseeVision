#include "ReservePlan.h"
#include "ReserveParts.h"

/**
 * The 36 racks (§3.7), each a moving body drawn at home in its own frame (origin at the rack's floor centre, the stage's
 * axes): two end posts and the top and kick rails in drawn brass, the woven brass mesh clamped in their slots, the two
 * trolleys that hang it from its track, the guide shoes and fins that keep it upright in the floor track, the pull on its
 * nave end, and on the lit faces the picture light's trough.
 *
 * The mesh is real wire, Ø 3.0 at 25 mm pitch: at 11,376 wires Form can make each only as a copy of its own mesh, so the
 * crimp of the weave (which would carry every wire's mesh over a thousand stations) is left out, and the mesh is its two
 * layers of straight wires touching on the rack's centre plane (the README's gaps).
 */
namespace Musee::Reserve
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
		/** The carriage's small parts run in their tracks; nothing else reaches them. */
		const Musee::Collision::FClass* const Carried = nullptr;

		/** A drawing riding a rack's body. */
		FFormDrawing& OnRack(TArray<FFormDrawing>& Out, const FRack& Rack, FFormDrawing Drawing)
		{
			Drawing.Body = Rack.Body();
			return Out.Add_GetRef(MoveTemp(Drawing));
		}

		/**
		 * An end post's plan: the hollow section's outline, 0.160 across × 0.090 along with its arrises rounded r 8, centred
		 * on y 2.455, and the mesh's slot let 12 mm into its inner face; drawn anticlockwise.
		 */
		FFormProfile PostPlan()
		{
			const double A = 0.5 * Plan::PostAcross;
			const double B = 0.5 * Plan::PostAlong;
			const double R = Plan::PostArris;
			const double S = Plan::MeshSlotHalf;
			const double Inner = Plan::PostY - B;
			const double Outer = Plan::PostY + B;
			FFormProfile P = FFormProfile::From(FVector2D(S, Inner));
			P.LineTo(FVector2D(A - R, Inner), EFormJoint::Crisp)
				.ArcAbout(FVector2D(A - R, Inner + R), R, R, -90.0, 0.0, EFormJoint::Smooth)
				.ArcAbout(FVector2D(A - R, Outer - R), R, R, 0.0, 90.0, EFormJoint::Smooth)
				.ArcAbout(FVector2D(-A + R, Outer - R), R, R, 90.0, 180.0, EFormJoint::Smooth)
				.ArcAbout(FVector2D(-A + R, Inner + R), R, R, 180.0, 270.0, EFormJoint::Smooth)
				.LineTo(FVector2D(-S, Inner), EFormJoint::Smooth)
				.LineTo(FVector2D(-S, Inner + Plan::MeshClamp), EFormJoint::Crisp)
				.LineTo(FVector2D(S, Inner + Plan::MeshClamp), EFormJoint::Crisp);
			return P.Close();
		}

		/** A rail's section (across, z) between two heights, the mesh's slot let 12 mm into its face towards the mesh. */
		FFormProfile RailSection(double Bottom, double Top, bool bSlotBelow)
		{
			const double H = Plan::RailHalf;
			const double S = Plan::MeshSlotHalf;
			if (bSlotBelow)
			{
				const double Slot = Bottom + Plan::MeshClamp;
				return FFormProfile::Polygon({FVector2D(-H, Bottom), FVector2D(-S, Bottom), FVector2D(-S, Slot), FVector2D(S, Slot), FVector2D(S, Bottom),
					FVector2D(H, Bottom), FVector2D(H, Top), FVector2D(-H, Top)});
			}
			const double Slot = Top - Plan::MeshClamp;
			return FFormProfile::Polygon({FVector2D(-H, Bottom), FVector2D(H, Bottom), FVector2D(H, Top), FVector2D(S, Top), FVector2D(S, Slot),
				FVector2D(-S, Slot), FVector2D(-S, Top), FVector2D(-H, Top)});
		}

		/** The frame: the two posts (the second turned half round, its slot facing the first), the top and kick rails. */
		void Frame(TArray<FFormDrawing>& Out, const FRack& Rack)
		{
			FFormExtrusion Post;
			Post.Outline = PostPlan();
			Post.DepthMeters = Plan::RackTop - Plan::RackFoot;
			FFormDrawing& Posts = OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("post")), TEXT("screen.frame"), MoveTemp(Post), Solid));
			Posts.Repeats = {FMuseeFrame(FVector(0.0, 0.0, Plan::RackFoot)), FMuseeFrame(FVector(0.0, 0.0, Plan::RackFoot), FQuat(FVector::UpVector, UE_DOUBLE_PI))};

			const FFormProfile Path = FFormProfile::Polyline({FVector2D(0.0, -Plan::RailEnd), FVector2D(0.0, Plan::RailEnd)}, false);
			FFormSweep Top;
			Top.Section = RailSection(Plan::TopRailBottom, Plan::RackTop, true);
			Top.Path = Path;
			OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("rail.top")), TEXT("screen.frame"), MoveTemp(Top), Solid));
			FFormSweep Kick;
			Kick.Section = RailSection(Plan::RackFoot, Plan::KickRailTop, false);
			Kick.Path = Path;
			OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("rail.kick")), TEXT("screen.frame"), MoveTemp(Kick), Solid));
		}

		/**
		 * The mesh: the upright wires on the centre plane's one side and the level wires on its other, touching at every
		 * crossing, every wire's end clamped in a slot.
		 */
		void Mesh(TArray<FFormDrawing>& Out, const FRack& Rack)
		{
			const double R = 0.5 * Plan::WireDiameter;
			const double Bottom = Plan::KickRailTop - Plan::MeshClamp + Plan::WireEndClear;
			const double Top = Plan::TopRailBottom + Plan::MeshClamp - Plan::WireEndClear;
			const int32 Uprights = FMath::FloorToInt32((Plan::RailEnd - R) / Plan::WirePitch);
			FFormDrawing& Warp = OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("mesh.upright")), TEXT("screen.mesh"), Cylinder(Plan::WireDiameter, Bottom, Top), Solid));
			for (int32 K = -Uprights; K <= Uprights; ++K)
			{
				Warp.Repeats.Add(FMuseeFrame(FVector(R, K * Plan::WirePitch, 0.0)));
			}

			const double Middle = 0.5 * (Plan::KickRailTop + Plan::TopRailBottom);
			const int32 Levels = FMath::FloorToInt32((0.5 * (Plan::TopRailBottom - Plan::KickRailTop) - R) / Plan::WirePitch);
			const double Reach = Plan::RailEnd + Plan::MeshClamp - Plan::WireEndClear;
			FFormDrawing& Weft = OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("mesh.level")), TEXT("screen.mesh"), Cylinder(Plan::WireDiameter, 0.0, 2.0 * Reach),
				Solid));
			for (int32 J = -Levels; J <= Levels; ++J)
			{
				Weft.Repeats.Add(AlongAxis(FVector(-R, -Reach, Middle + J * Plan::WirePitch), FVector(0.0, 1.0, 0.0)));
			}
		}

		/** The carriage: two trolleys on the top rail (saddle, nut, stem into the track), two shoes and two fins below. */
		void Carriage(TArray<FFormDrawing>& Out, const FRack& Rack)
		{
			const double SaddleTop = Plan::RackTop + Plan::SaddleHeight;
			const double NutTop = SaddleTop + Plan::NutHeight;
			for (const double End : {-1.0, 1.0})
			{
				const FString Side = End < 0.0 ? TEXT("north") : TEXT("south");
				const double Y = End * Plan::TrolleyY;
				OnRack(Out, Rack, Drawn(Rack.Piece(*(TEXT("trolley.saddle.") + Side)), TEXT("screen.carriage"), Block(FVector(-0.5 * Plan::SaddleAcross,
					Y - 0.5 * Plan::SaddleAlong, Plan::RackTop), FVector(0.5 * Plan::SaddleAcross, Y + 0.5 * Plan::SaddleAlong, SaddleTop)), Carried));
				OnRack(Out, Rack, Drawn(Rack.Piece(*(TEXT("trolley.nut.") + Side)), TEXT("screen.carriage"), Cylinder(Plan::NutDiameter, SaddleTop, NutTop),
					Carried, FMuseeFrame(FVector(0.0, Y, 0.0))));
				OnRack(Out, Rack, Drawn(Rack.Piece(*(TEXT("trolley.stem.") + Side)), TEXT("screen.carriage"), Cylinder(Plan::StemDiameter, NutTop, Plan::StemTop),
					Carried, FMuseeFrame(FVector(0.0, Y, 0.0))));

				const double Shoe = End * Plan::ShoeY;
				OnRack(Out, Rack, Drawn(Rack.Piece(*(TEXT("shoe.") + Side)), TEXT("screen.carriage"), Block(FVector(-0.5 * Plan::ShoeAcross,
					Shoe - 0.5 * Plan::ShoeAlong, Plan::ShoeBottom), FVector(0.5 * Plan::ShoeAcross, Shoe + 0.5 * Plan::ShoeAlong, Plan::RackFoot)), Carried));
				OnRack(Out, Rack, Drawn(Rack.Piece(*(TEXT("fin.") + Side)), TEXT("screen.carriage"), Block(FVector(-0.5 * Plan::FinThickness,
					Shoe - 0.5 * Plan::FinAlong, Plan::FinBottom), FVector(0.5 * Plan::FinThickness, Shoe + 0.5 * Plan::FinAlong, Plan::ShoeBottom)), Carried));
			}
		}

		/** The pull on the nave end post: the bar 58 mm off the post's face on two stand-offs, each meeting the bar's side. */
		void Pull(TArray<FFormDrawing>& Out, const FRack& Rack)
		{
			const double N = Rack.TowardsNave();
			const double Face = Plan::PostY + 0.5 * Plan::PostAlong;
			const double Axis = Face + Plan::PullOff;
			OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("pull.bar")), TEXT("screen.pull"), Cylinder(Plan::PullDiameter, Plan::PullBottom, Plan::PullTop), Solid,
				FMuseeFrame(FVector(0.0, N * Axis, 0.0))));
			const double R = 0.5 * Plan::PullDiameter;
			const double S = 0.5 * Plan::StandOffDiameter;
			const double Reach = Axis - FMath::Sqrt(R * R - S * S) - Face;
			FFormDrawing& StandOffs = OnRack(Out, Rack, Drawn(Rack.Piece(TEXT("pull.stand-off")), TEXT("screen.pull"), Cylinder(Plan::StandOffDiameter, 0.0,
				Reach), Solid));
			for (const double Z : Plan::StandOffZs)
			{
				StandOffs.Repeats.Add(AlongAxis(FVector(0.0, N * Face, Z), FVector(0.0, N, 0.0)));
			}
		}

		/** A lit face's picture light: the trough over its lamp's span, on two arms from plates on the top rail. */
		void Trough(TArray<FFormDrawing>& Out, const FRack& Rack, int32 Face, double Lamp)
		{
			const double F = Face;
			const FString Name = Rack.Piece(Face < 0 ? TEXT("lamp.west") : TEXT("lamp.east"));
			const double Half = 0.5 * Plan::TroughAcross;
			const double Low = Plan::TroughCentreZ - 0.5 * Plan::TroughHeight;
			const double High = Plan::TroughCentreZ + 0.5 * Plan::TroughHeight;
			const double Inner = Plan::TroughOff - Half;
			OnRack(Out, Rack, Drawn(Name + TEXT(".trough"), TEXT("screen.lamp"), Block(FVector(FMath::Min(F * Inner, F * (Inner + Plan::TroughAcross)),
				-0.5 * Lamp, Low), FVector(FMath::Max(F * Inner, F * (Inner + Plan::TroughAcross)), 0.5 * Lamp, High)), Carried));
			const double Rail = Plan::RailHalf;
			const double Plate = Rail + Plan::TroughPlateThickness;
			FFormDrawing Plates = Drawn(Name + TEXT(".plate"), TEXT("screen.lamp"), Block(FVector(0.0, -0.5 * Plan::TroughPlateAlong,
				-0.5 * Plan::TroughPlateHeight), FVector(Plan::TroughPlateThickness, 0.5 * Plan::TroughPlateAlong, 0.5 * Plan::TroughPlateHeight)), Carried);
			FFormDrawing Arms = Drawn(Name + TEXT(".arm"), TEXT("screen.lamp"), Cylinder(Plan::TroughArmDiameter, 0.0, Inner - Plate), Carried);
			for (const double End : {-1.0, 1.0})
			{
				const double Y = End * (0.5 * Lamp - Plan::TroughArmIn);
				// The plate's frame faces out of the rail's face: turned half round on the west face.
				Plates.Repeats.Add(FMuseeFrame(FVector(F * Rail, Y, Plan::TroughCentreZ), Face < 0 ? FQuat(FVector::UpVector, UE_DOUBLE_PI) : FQuat::Identity));
				Arms.Repeats.Add(AlongAxis(FVector(F * Plate, Y, Plan::TroughCentreZ), FVector(F, 0.0, 0.0)));
			}
			OnRack(Out, Rack, MoveTemp(Plates));
			OnRack(Out, Rack, MoveTemp(Arms));
		}
	}

	void DrawRacks(TArray<FFormDrawing>& Out)
	{
		for (const FRack& Rack : Racks())
		{
			Frame(Out, Rack);
			Mesh(Out, Rack);
			Carriage(Out, Rack);
			Pull(Out, Rack);
			for (const Plan::FLitFace& Lit : Plan::LitFaces)
			{
				if (Rack.Name == Lit.Rack)
				{
					Trough(Out, Rack, Lit.Face, Lit.LampMeters);
				}
			}
		}
	}
}
