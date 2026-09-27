#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/Carry.h"
#include "Kernel/MeshBuilder.h"
#include "Stations/Stations.h"

/**
 * A ground's development and its carrying (FFormGround). A ground of revolution follows its meridian on the meridian's
 * own stations, within the chord tolerance as every drawn curve is, its normal turning between the stations as the
 * drawn curve's does; a sphere is carried exactly, each point along its ray from the centre.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** A meridian's stations: (radius, height), the unit normals leaving and arriving at each, and V along it. */
		struct FMeridian
		{
			TArray<FVector2D> Points;
			TArray<FVector2D> NormalsOut;
			TArray<FVector2D> NormalsIn;
			TArray<double> Lengths;

			/** The point and unit normal V metres along the meridian: on its chords, or on its end chords run on beyond them. */
			void At(double V, FVector2D& OutPoint, FVector2D& OutNormal) const
			{
				int32 Chord = 0;
				while (Chord + 2 < Lengths.Num() && V > Lengths[Chord + 1])
				{
					++Chord;
				}
				const double Span = Lengths[Chord + 1] - Lengths[Chord];
				const double Alpha = Span > 0.0 ? (V - Lengths[Chord]) / Span : 0.0;
				OutPoint = FMath::Lerp(Points[Chord], Points[Chord + 1], Alpha);
				OutNormal = FMath::Lerp(NormalsOut[Chord], NormalsIn[Chord + 1], FMath::Clamp(Alpha, 0.0, 1.0)).GetSafeNormal();
			}
		};

		bool ReadMeridian(const FFormGround& Ground, FMeridian& Out, FString& OutWhy)
		{
			if (!CheckProfile(Ground.Meridian, TEXT("the ground's meridian"), false, OutWhy))
			{
				return false;
			}
			if (Ground.Meridian.Closed || Ground.ReferenceRadiusMeters <= 0.0)
			{
				OutWhy = TEXT("a ground of revolution has an open meridian and a reference radius");
				return false;
			}
			const FProfileStations Stations = Resolve(Ground.Meridian);
			for (const FProfileVertex& Vertex : Stations.Vertices)
			{
				if (Vertex.Point.X < 0.0)
				{
					OutWhy = FString::Printf(TEXT("the ground's meridian crosses its axis (radius %.4f m)"), Vertex.Point.X);
					return false;
				}
				Out.Points.Add(Vertex.Point);
				Out.NormalsOut.Add(Vertex.NormalOut);
				Out.NormalsIn.Add(Vertex.NormalIn);
				Out.Lengths.Add(Vertex.LengthMeters);
			}
			if (Out.Points.Num() < 2)
			{
				OutWhy = TEXT("the ground's meridian has fewer than two stations");
				return false;
			}
			return true;
		}
	}

	bool SetOnGround(const FFormGround& Ground, const FMeshBuilder& Developed, FMeshBuilder& Out, FString& OutWhy)
	{
		if (Ground.Kind == EFormGroundKind::Revolution)
		{
			FMeridian Meridian;
			if (!ReadMeridian(Ground, Meridian, OutWhy))
			{
				return false;
			}
			const double Reference = Ground.ReferenceRadiusMeters;
			auto Revolve = [&Meridian, Reference](const FVector& Developed2)
			{
				FVector2D Point, Normal;
				Meridian.At(Developed2.Y, Point, Normal);
				const FVector2D Standing = Point + Normal * Developed2.Z;
				const double Angle = Developed2.X / Reference;
				return FVector(Standing.X * FMath::Cos(Angle), Standing.X * FMath::Sin(Angle), Standing.Y);
			};
			CarryOntoGround(Developed, Revolve, DefaultChordToleranceMeters, Out);
			return true;
		}
		if (Ground.Kind == EFormGroundKind::Sphere)
		{
			if (Ground.RadiusMeters <= 0.0)
			{
				OutWhy = TEXT("a spherical ground has a radius");
				return false;
			}
			const FMuseeFrame& Tangent = Ground.Tangent;
			const double Radius = Ground.RadiusMeters;
			const FVector Centre = Tangent.PointToParent(FVector(0.0, 0.0, -Radius));
			auto Project = [&Tangent, &Centre, Radius](const FVector& Developed2)
			{
				const FVector Ray = (Tangent.PointToParent(FVector(Developed2.X, Developed2.Y, 0.0)) - Centre).GetSafeNormal();
				return Centre + Ray * (Radius + Developed2.Z);
			};
			CarryOntoGround(Developed, Project, DefaultChordToleranceMeters, Out);
			return true;
		}
		OutWhy = TEXT("a flat ground carries nothing");
		return false;
	}
}
