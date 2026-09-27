#include "AtriumParts.h"

/**
 * The canopy grid (§3.5, after the Great Court): straight pearl steel members 60 wide by 120 deep between nodes on twenty
 * rings, set out at the as-built polar angles and projected onto the canopy's room face. Rings 1 to 14 carry 72 nodes
 * 5° apart, rings 15 to 19 carry 36 nodes 10° apart, alternate rings offset half a step; ring 20 is the compression
 * ring's face. The members are the rings' members, and from each node to the two nearest nodes of the ring outside (at
 * the change from 72 to 36, each inner node fans to three). Members stop 5 mm short of the ribs' side faces where a rib
 * runs under the canopy, and 5 mm short of the ring.
 *
 * A straight member under curved glass bears on it along its middle: each member's top face touches the room face at
 * mid-span, and hangs below it at the nodes by the chord's sagitta. A ring's members, all at one height, are one swept
 * polygon (a closed ring where no rib meets it, one run per bay where the ribs do); every other member is its own bar.
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A member shorter than this after its stops is left out: a stub the node itself fills. */
		constexpr double ShortestMember = 0.050;

		int32 Rings() { return int32(UE_ARRAY_COUNT(Plan::GridRings)); }
		int32 NodesOn(int32 Ring) { return Ring < Plan::DenseRings ? Plan::DenseNodes : Plan::SparseNodes; }
		double StepOn(int32 Ring) { return 360.0 / NodesOn(Ring); }

		/** The ring's first node's bearing past ring 1's: half a step on alternate rings; the first sparse ring stands over ring 14's nodes. */
		double OffsetOn(int32 Ring)
		{
			if (Ring < Plan::DenseRings)
			{
				return (Ring % 2) * 0.5 * StepOn(Ring);
			}
			const double Over = ((Plan::DenseRings - 1) % 2) * 0.5 * StepOn(Plan::DenseRings - 1);
			return Over + ((Ring - Plan::DenseRings) % 2) * 0.5 * StepOn(Ring);
		}

		double NodeBearing(int32 Ring, int32 Node)
		{
			return Plan::FirstNodeBearing + OffsetOn(Ring) + StepOn(Ring) * Node;
		}

		FVector NodeAt(int32 Ring, int32 Node)
		{
			const FVector2D Plan2 = Polar(Plan::GridRings[Ring][0], NodeBearing(Ring, Node));
			return FVector(Plan2.X, Plan2.Y, Plan::GridRings[Ring][1]);
		}

		/** The node of a ring nearest a bearing. */
		int32 NodeNear(int32 Ring, double Bearing)
		{
			const int32 N = NodesOn(Ring);
			const int32 Index = FMath::RoundToInt32((Bearing - Plan::FirstNodeBearing - OffsetOn(Ring)) / StepOn(Ring));
			return ((Index % N) + N) % N;
		}

		const FVector& SphereCentre()
		{
			static const FVector Centre(0.0, 0.0, Plan::SphereCentreZ);
			return Centre;
		}

		/** The height of a rib's back line at a radius: on the canopy's room face under the Sphere, round the turn outside it. */
		double RibBackZ(double Radius)
		{
			const double Turn = Plan::RibTurnRadius;
			const double TangentR = Plan::RibTurnCentreR + Turn * FMath::Cos(FMath::DegreesToRadians(Plan::RibTurnEndDeg));
			if (Radius <= TangentR)
			{
				return Plan::SphereCentreZ - FMath::Sqrt(Plan::CanopyRadius * Plan::CanopyRadius - Radius * Radius);
			}
			const double Out = Radius - Plan::RibTurnCentreR;
			return FMath::Abs(Out) < Turn ? Plan::RibTurnZ + FMath::Sqrt(Turn * Turn - Out * Out) : -UE_DOUBLE_BIG_NUMBER;
		}

		/** A straight member's placing: its top face at mid-span on the room face, its depth along the Sphere's radius there. */
		struct FMember
		{
			FVector From;
			FVector To;
			FVector Normal;

			FMember(const FVector& P, const FVector& Q)
			{
				const FVector Middle = 0.5 * (P + Q) - SphereCentre();
				Normal = Middle.GetSafeNormal();
				const FVector Lift = (Plan::CanopyRadius - Middle.Size()) * Normal;
				From = P + Lift;
				To = Q + Lift;
			}

			FVector At(double S) const { return From + S * (To - From); }

			/** The member's width axis: square to its length and its depth. */
			FVector Across() const { return FVector::CrossProduct(Normal, (To - From).GetSafeNormal()); }

			/** Whether its underside at S passes under a rib's back line (5 mm kept), so the rib is in the way there. */
			bool MeetsRib(double S) const
			{
				const FVector Top = At(S);
				const double Bottom = Top.Z + Plan::MemberDepth * Normal.Z - 0.5 * Plan::MemberWidth * FMath::Abs(Across().Z);
				return Bottom < RibBackZ(FVector2D(Top.X, Top.Y).Size()) + Plan::SteelClearance;
			}
		};

		/** The stretch of a member, as fractions of its length, within the given distance either side of rib k's plane, if the rib is in the way there. */
		bool RibStretch(const FMember& Member, int32 Rib, double Reach, double& OutFrom, double& OutTo)
		{
			const FVector2D Normal2 = Along(RibBearing(Rib));
			const FVector Normal(Normal2.X, Normal2.Y, 0.0);
			const double Allow = Reach + 0.5 * Plan::MemberWidth * FMath::Abs(FVector::DotProduct(Member.Across(), Normal));
			const double D0 = FVector::DotProduct(Member.From, Normal);
			const double D1 = FVector::DotProduct(Member.To, Normal);
			if (FMath::IsNearlyEqual(D0, D1))
			{
				if (FMath::Abs(D0) >= Allow) { return false; }
				OutFrom = 0.0;
				OutTo = 1.0;
			}
			else
			{
				const double SA = (-Allow - D0) / (D1 - D0);
				const double SB = (Allow - D0) / (D1 - D0);
				OutFrom = FMath::Clamp(FMath::Min(SA, SB), 0.0, 1.0);
				OutTo = FMath::Clamp(FMath::Max(SA, SB), 0.0, 1.0);
				if (OutFrom >= OutTo) { return false; }
			}
			const double Middle = 0.5 * (OutFrom + OutTo);
			const FVector2D Where(Member.At(Middle).X, Member.At(Middle).Y);
			return FVector2D::DotProduct(Where, Outward(RibBearing(Rib))) > 0.0 && Member.MeetsRib(Middle);
		}

		/** The fraction of a member's length, running inwards, at which it comes to the ring's reach (its outer face and the 5 mm joint). */
		double RingStop(const FMember& Member)
		{
			const FVector2D P(Member.From.X, Member.From.Y);
			const FVector2D D = FVector2D(Member.To.X, Member.To.Y) - P;
			const double Reach = Plan::RingOuter + Plan::SteelClearance;
			// |P + s D| = Reach, the root nearest the far end (the member runs inwards to the ring).
			const double A = D.SizeSquared();
			const double B = 2.0 * FVector2D::DotProduct(P, D);
			const double C = P.SizeSquared() - Reach * Reach;
			const double Disc = B * B - 4.0 * A * C;
			return Disc < 0.0 ? 1.0 : FMath::Clamp((-B - FMath::Sqrt(Disc)) / (2.0 * A), 0.0, 1.0);
		}

		/** A bar from S0 to S1 of a member, as an extrusion of its section along its length. */
		void Bar(TArray<FFormDrawing>& Out, const FString& Name, const FMember& Member, double S0, double S1)
		{
			const FVector Along3 = Member.To - Member.From;
			const double Length = (S1 - S0) * Along3.Size();
			if (Length < ShortestMember)
			{
				return;
			}
			const FMuseeFrame Frame(Member.At(S0), FRotationMatrix::MakeFromZY(Along3, Member.Normal).ToQuat());
			const double Half = 0.5 * Plan::MemberWidth;
			Out.Add(Drawn(Name, TEXT("canopy.grid"), Prism(Box(-Half, 0.0, Half, Plan::MemberDepth), 0.0, Length), Solid, Frame));
		}

		/** A member between two nodes, cut where ribs are in the way and stopped short of the ring when it reaches it. */
		void CutMember(TArray<FFormDrawing>& Out, const FString& Name, const FVector& P, const FVector& Q, bool bToRing)
		{
			const FMember Member(P, Q);
			TArray<TPair<double, double>> Kept = {{0.0, bToRing ? RingStop(Member) : 1.0}};
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				double From = 0.0;
				double To = 0.0;
				if (!RibStretch(Member, Rib, Plan::RibHalf + Plan::SteelClearance, From, To))
				{
					continue;
				}
				TArray<TPair<double, double>> Next;
				for (const TPair<double, double>& Part : Kept)
				{
					if (Part.Key < From) { Next.Add({Part.Key, FMath::Min(Part.Value, From)}); }
					if (Part.Value > To) { Next.Add({FMath::Max(Part.Key, To), Part.Value}); }
				}
				Kept = MoveTemp(Next);
			}
			for (int32 Index = 0; Index < Kept.Num(); ++Index)
			{
				Bar(Out, Kept.Num() > 1 ? FString::Printf(TEXT("%s.%d"), *Name, Index + 1) : Name, Member, Kept[Index].Key, Kept[Index].Value);
			}
		}

		/** A ring's members' section, (A in towards the axis from the chord, B up), at the chord's middle: square to the Sphere's radius there. */
		FFormProfile RingSection(int32 Ring)
		{
			const FMember Chord(NodeAt(Ring, 0), NodeAt(Ring, 1));
			const FVector Middle = 0.5 * (NodeAt(Ring, 0) + NodeAt(Ring, 1));
			const FVector2D Out2(Middle.X, Middle.Y);
			const double Chord2 = Out2.Size();
			const FVector2D N(FVector2D(Chord.Normal.X, Chord.Normal.Y).Size(), Chord.Normal.Z);
			const FVector2D T(-N.Y, N.X);
			const FVector2D Top(Plan::CanopyRadius * N.X, Plan::SphereCentreZ + Plan::CanopyRadius * N.Y);
			const double Half = 0.5 * Plan::MemberWidth;
			const FVector2D Corners[] = {Top - Half * T, Top + Half * T, Top + Half * T + Plan::MemberDepth * N, Top - Half * T + Plan::MemberDepth * N};
			TArray<FVector2D> Points;
			for (const FVector2D& Corner : Corners)
			{
				Points.Add(FVector2D(Chord2 - Corner.X, Corner.Y - Middle.Z));
			}
			return FFormProfile::Polyline(Points, true);
		}

		/** The plan point a fraction along a ring's chord from one node to the next. */
		FVector2D OnChord(int32 Ring, int32 Node, double S)
		{
			const FVector P = NodeAt(Ring, Node);
			const FVector Q = NodeAt(Ring, Node + 1);
			const FVector X = P + S * (Q - P);
			return FVector2D(X.X, X.Y);
		}

		/**
		 * Where a ring's members clear a rib's plane by the stop: the chord and fraction, walking towards increasing
		 * bearing, where the distance first reaches +Reach (leaving the rib) or last is -Reach (coming to it).
		 */
		void ClearOf(int32 Ring, int32 Rib, bool bLeaving, int32& OutNode, double& OutS)
		{
			const FVector2D Normal = Along(RibBearing(Rib));
			const double Reach = (bLeaving ? 1.0 : -1.0) * (Plan::RibHalf + Plan::SteelClearance);
			const int32 Near = NodeNear(Ring, RibBearing(Rib));
			for (int32 Offset = -2; Offset <= 1; ++Offset)
			{
				const int32 Node = Near + Offset;
				const double D0 = FVector2D::DotProduct(OnChord(Ring, Node, 0.0), Normal);
				const double D1 = FVector2D::DotProduct(OnChord(Ring, Node, 1.0), Normal);
				if ((D0 - Reach) * (D1 - Reach) <= 0.0 && !FMath::IsNearlyEqual(D0, D1))
				{
					OutNode = Node;
					OutS = (Reach - D0) / (D1 - D0);
					return;
				}
			}
			OutNode = Near;
			OutS = 0.0;
		}

		/** Whether a ring's members pass under the ribs' backs (5 mm kept) anywhere a rib crosses the ring. */
		bool RibsMeetRing(int32 Ring)
		{
			const FMember Chord(NodeAt(Ring, 0), NodeAt(Ring, 1));
			return Chord.MeetsRib(0.5);
		}

		/** A ring's members: one closed ring, or where the ribs meet it, one run from rib to rib in each bay. */
		void RingMembers(TArray<FFormDrawing>& Out, int32 Ring)
		{
			FFormSweep Shape;
			Shape.Section = RingSection(Ring);
			Shape.ElevationMeters = Plan::GridRings[Ring][1];
			const int32 N = NodesOn(Ring);
			if (!RibsMeetRing(Ring))
			{
				TArray<FVector2D> Points;
				for (int32 Node = 0; Node < N; ++Node)
				{
					Points.Add(OnChord(Ring, Node, 0.0));
				}
				Shape.Path = FFormProfile::Polyline(Points, true);
				Out.Add(Drawn(FString::Printf(TEXT("canopy.grid.ring-%02d"), Ring + 1), TEXT("canopy.grid"), MoveTemp(Shape), Solid));
				return;
			}
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				int32 FirstNode = 0;
				int32 LastNode = 0;
				double FirstS = 0.0;
				double LastS = 0.0;
				ClearOf(Ring, Rib, true, FirstNode, FirstS);
				ClearOf(Ring, Rib + 1, false, LastNode, LastS);
				LastNode += LastNode < FirstNode ? N : 0;
				TArray<FVector2D> Points = {OnChord(Ring, FirstNode, FirstS)};
				for (int32 Node = FirstNode + 1; Node <= LastNode; ++Node)
				{
					Points.Add(OnChord(Ring, Node, 0.0));
				}
				Points.Add(OnChord(Ring, LastNode, LastS));
				FFormSweep Run = Shape;
				Run.Path = FFormProfile::Polyline(Points, false);
				Out.Add(Drawn(FString::Printf(TEXT("canopy.grid.ring-%02d.bay-%02d"), Ring + 1, Rib + 1), TEXT("canopy.grid"), MoveTemp(Run), Solid));
			}
		}

		/** The members from each node of a ring to the nearest nodes of the ring outside it: two, or three where 72 become 36. */
		void Diagonals(TArray<FFormDrawing>& Out, int32 Inner)
		{
			const int32 Outer = Inner - 1;
			const bool bFan = NodesOn(Inner) != NodesOn(Outer);
			const bool bToRing = Inner + 1 == Rings();
			for (int32 Node = 0; Node < NodesOn(Inner); ++Node)
			{
				const double Bearing = NodeBearing(Inner, Node);
				TArray<double> Toward = {Bearing - 0.5 * StepOn(Outer), Bearing + 0.5 * StepOn(Outer)};
				if (bFan)
				{
					Toward = {Bearing - StepOn(Outer), Bearing, Bearing + StepOn(Outer)};
				}
				for (int32 Index = 0; Index < Toward.Num(); ++Index)
				{
					// From the outer node inwards, so a member that reaches the ring stops at its far end.
					CutMember(Out, FString::Printf(TEXT("canopy.grid.diagonal-%02d.%02d.%d"), Inner + 1, Node + 1, Index + 1),
						NodeAt(Outer, NodeNear(Outer, Toward[Index])), NodeAt(Inner, Node), bToRing);
				}
			}
		}
	}

	void DrawCanopyGrid(TArray<FFormDrawing>& Out)
	{
		// Ring 20 is the compression ring's face: its nodes take the members from ring 19 and carry no ring members of their own.
		for (int32 Ring = 0; Ring + 1 < Rings(); ++Ring)
		{
			RingMembers(Out, Ring);
		}
		for (int32 Inner = 1; Inner < Rings(); ++Inner)
		{
			Diagonals(Out, Inner);
		}
	}
}
