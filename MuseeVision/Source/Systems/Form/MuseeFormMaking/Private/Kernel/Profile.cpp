#include "Kernel/Profile.h"

#include "Kernel/Tolerances.h"
#include "Stations/Stations.h"

namespace Musee::Form
{
	namespace
	{
		/** The unit normal on the right of a step: the side a profile drawn with its solid on the left faces. */
		FVector2D RightNormal(const FVector2D& Step)
		{
			return FVector2D(Step.Y, -Step.X).GetSafeNormal();
		}

		FVector2D EllipsePoint(const FVector2D& Center, double RadiusA, double RadiusB, double T)
		{
			return Center + FVector2D(RadiusA * FMath::Cos(T), RadiusB * FMath::Sin(T));
		}

		/** The arc's own surface normal at t: the right of its exact tangent, in the direction it is drawn. */
		FVector2D EllipseNormal(double RadiusA, double RadiusB, double T, bool bIncreasing)
		{
			const FVector2D Tangent(-RadiusA * FMath::Sin(T), RadiusB * FMath::Cos(T));
			return RightNormal(bIncreasing ? Tangent : -Tangent);
		}

		/** A profile being resolved: its stations so far, with the joint at each still to be settled. */
		struct FDraft
		{
			TArray<FProfileVertex> Vertices;
			TArray<EProfileJoint> Joints;

			explicit FDraft(const FVector2D& Start)
			{
				FProfileVertex First;
				First.Point = Start;
				Vertices.Add(First);
				Joints.Add(EProfileJoint::Auto);
			}

			const FVector2D& Current() const { return Vertices.Last().Point; }

			/** A segment from the current station to End, with its normals at both ends, meeting the last at Joint. */
			void Segment(const FVector2D& End, const FVector2D& NormalAtStart, const FVector2D& NormalAtEnd,
				EProfileJoint Joint)
			{
				FProfileVertex& From = Vertices.Last();
				From.NormalOut = NormalAtStart;
				Joints.Last() = Joint;
				FProfileVertex To;
				To.Point = End;
				To.NormalIn = NormalAtEnd;
				To.LengthMeters = From.LengthMeters + FVector2D::Distance(From.Point, End);
				Vertices.Add(To);
				Joints.Add(EProfileJoint::Auto);
			}

			/** A straight segment to End. A line of no length draws nothing. */
			void Line(const FVector2D& End, EProfileJoint Joint)
			{
				const FVector2D Step = End - Current();
				if (Step.Size() > WeldToleranceMeters)
				{
					const FVector2D Normal = RightNormal(Step);
					Segment(End, Normal, Normal, Joint);
				}
			}
		};

		/** Settles one station's joint: one mean normal where it is smooth, the two sides' own where it is crisp. */
		void Settle(FProfileVertex& Vertex, EProfileJoint Joint)
		{
			const double Turn = FMath::Atan2(FMath::Abs(FVector2D::CrossProduct(Vertex.NormalIn, Vertex.NormalOut)),
				FVector2D::DotProduct(Vertex.NormalIn, Vertex.NormalOut));
			const bool bTangent = Turn < TangentJointRadians;
			const bool bSmooth = Joint == EProfileJoint::Smooth || (Joint == EProfileJoint::Auto && bTangent);
			const FVector2D Mean = (Vertex.NormalIn + Vertex.NormalOut).GetSafeNormal();
			Vertex.bCrisp = !bSmooth || Mean.IsZero();
			if (!Vertex.bCrisp)
			{
				Vertex.NormalIn = Mean;
				Vertex.NormalOut = Mean;
			}
		}
	}

	double FProfileStations::LengthAtSegmentEnd(int32 Segment) const
	{
		return (bClosed && Segment == Vertices.Num() - 1) ? ClosedLengthMeters : Vertices[Segment + 1].LengthMeters;
	}

	double FProfileStations::SignedAreaSquareMeters() const
	{
		double Twice = 0.0;
		for (int32 Index = 0; Index < Vertices.Num(); ++Index)
		{
			Twice += FVector2D::CrossProduct(Vertices[Index].Point, Vertices[(Index + 1) % Vertices.Num()].Point);
		}
		return 0.5 * Twice;
	}

	FProfileStations FProfileStations::Reversed() const
	{
		FProfileStations Result;
		Result.bClosed = bClosed;
		const int32 Count = Vertices.Num();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// A closed profile keeps its first station first; an open one starts from its end.
			const FProfileVertex& Source = Vertices[bClosed ? (Count - Index) % Count : Count - 1 - Index];
			FProfileVertex Vertex = Source;
			Vertex.NormalIn = -Source.NormalOut;
			Vertex.NormalOut = -Source.NormalIn;
			Vertex.LengthMeters = Index == 0 ? 0.0
				: Result.Vertices.Last().LengthMeters + FVector2D::Distance(Result.Vertices.Last().Point, Source.Point);
			Result.Vertices.Add(Vertex);
		}
		if (bClosed && Count > 0)
		{
			Result.ClosedLengthMeters = Result.Vertices.Last().LengthMeters
				+ FVector2D::Distance(Result.Vertices.Last().Point, Result.Vertices[0].Point);
		}
		return Result;
	}

	TArray<FVector2D> FProfileStations::Points() const
	{
		TArray<FVector2D> Result;
		Result.Reserve(Vertices.Num());
		for (const FProfileVertex& Vertex : Vertices)
		{
			Result.Add(Vertex.Point);
		}
		return Result;
	}

	FProfile::FProfile(const FVector2D& StartMeters)
		: Start(StartMeters)
	{
	}

	FProfile& FProfile::LineTo(const FVector2D& EndMeters, EProfileJoint Joint)
	{
		FElement Element;
		Element.Kind = EElement::Line;
		Element.Joint = Joint;
		Element.End = EndMeters;
		Elements.Add(Element);
		return *this;
	}

	FProfile& FProfile::Arc(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters, double FromRadians,
		double ToRadians, EProfileJoint Joint)
	{
		FElement Element;
		Element.Kind = EElement::Arc;
		Element.Joint = Joint;
		Element.Center = CenterMeters;
		Element.RadiusA = RadiusAMeters;
		Element.RadiusB = RadiusBMeters;
		Element.FromRadians = FromRadians;
		Element.ToRadians = ToRadians;
		Element.End = EllipsePoint(CenterMeters, RadiusAMeters, RadiusBMeters, ToRadians);
		Elements.Add(Element);
		return *this;
	}

	FProfile& FProfile::Arc(const FVector2D& CenterMeters, double RadiusMeters, double FromRadians, double ToRadians,
		EProfileJoint Joint)
	{
		return Arc(CenterMeters, RadiusMeters, RadiusMeters, FromRadians, ToRadians, Joint);
	}

	FProfile& FProfile::Close(EProfileJoint AtLast, EProfileJoint AtFirst)
	{
		bClosed = true;
		JointAtLast = AtLast;
		JointAtFirst = AtFirst;
		return *this;
	}

	FProfileStations FProfile::Resolve(double ChordToleranceMeters) const
	{
		FDraft Draft(Start);
		for (const FElement& Element : Elements)
		{
			if (Element.Kind == EElement::Line)
			{
				Draft.Line(Element.End, Element.Joint);
				continue;
			}
			const FVector2D ArcStart = EllipsePoint(Element.Center, Element.RadiusA, Element.RadiusB, Element.FromRadians);
			EProfileJoint JointAtArc = Element.Joint;
			if (FVector2D::Distance(Draft.Current(), ArcStart) > WeldToleranceMeters)
			{
				Draft.Line(ArcStart, Element.Joint);
				JointAtArc = EProfileJoint::Auto;
			}
			const bool bIncreasing = Element.ToRadians > Element.FromRadians;
			const TArray<double> T = EllipseStations(Element.RadiusA, Element.RadiusB, Element.FromRadians,
				Element.ToRadians, ChordToleranceMeters);
			for (int32 Index = 1; Index < T.Num(); ++Index)
			{
				Draft.Segment(EllipsePoint(Element.Center, Element.RadiusA, Element.RadiusB, T[Index]),
					EllipseNormal(Element.RadiusA, Element.RadiusB, T[Index - 1], bIncreasing),
					EllipseNormal(Element.RadiusA, Element.RadiusB, T[Index], bIncreasing),
					Index == 1 ? JointAtArc : EProfileJoint::Smooth);
			}
		}

		FProfileStations Result;
		TArray<FProfileVertex>& Vertices = Draft.Vertices;
		if (bClosed && Vertices.Num() > 2)
		{
			const FVector2D First = Vertices[0].Point;
			Draft.Line(First, JointAtLast);
			// The last station now stands on the first: fold it in.
			const FProfileVertex Last = Vertices.Pop();
			Draft.Joints.Pop();
			Vertices[0].NormalIn = Last.NormalIn;
			Draft.Joints[0] = JointAtFirst;
			Result.bClosed = true;
			Result.ClosedLengthMeters = Last.LengthMeters;
			for (int32 Index = 0; Index < Vertices.Num(); ++Index)
			{
				Settle(Vertices[Index], Draft.Joints[Index]);
			}
		}
		else
		{
			Vertices[0].NormalIn = Vertices[0].NormalOut;
			Vertices.Last().NormalOut = Vertices.Last().NormalIn;
			Vertices[0].bCrisp = true;
			Vertices.Last().bCrisp = true;
			for (int32 Index = 1; Index + 1 < Vertices.Num(); ++Index)
			{
				Settle(Vertices[Index], Draft.Joints[Index]);
			}
		}
		Result.Vertices = MoveTemp(Vertices);
		return Result;
	}
}
