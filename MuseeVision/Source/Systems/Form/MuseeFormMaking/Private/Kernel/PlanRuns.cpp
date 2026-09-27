#include "Kernel/PlanRuns.h"

#include "Kernel/Profile.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * A mitre stretches the section across the cut by 1 / cos(half the turn). A path that doubles back so sharply
		 * that the stretch would pass a hundredfold (a turn above 178.9 degrees) has no buildable corner.
		 */
		constexpr double MaxMitreStretch = 100.0;

		/** Which of a station's two sides a frame is for: the segment arriving, the one leaving, or both (smooth). */
		enum class ESide : uint8
		{
			Arriving,
			Leaving,
			Both,
		};

		/** A plan path's segments and turns. */
		struct FPlanPath
		{
			TConstArrayView<FVector2D> Points;
			bool bClosed = false;
			TArray<FVector2D> Directions;

			FPlanPath(TConstArrayView<FVector2D> InPoints, bool bInClosed)
				: Points(InPoints), bClosed(bInClosed)
			{
				const int32 Segments = bClosed ? Points.Num() : Points.Num() - 1;
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					Directions.Add((Points[(Segment + 1) % Points.Num()] - Points[Segment]).GetSafeNormal(StepSquaredMeters));
				}
			}

			bool IsEnd(int32 Station) const { return !bClosed && (Station == 0 || Station == Points.Num() - 1); }
			FVector2D InDirection(int32 Station) const
			{
				const int32 Arriving = (Station - 1 + Directions.Num()) % Directions.Num();
				return (!bClosed && Station == 0) ? Directions[0] : Directions[Arriving];
			}
			FVector2D OutDirection(int32 Station) const
			{
				const int32 Leaving = Station % Directions.Num();
				return (!bClosed && Station == Points.Num() - 1) ? Directions.Last() : Directions[Leaving];
			}
			static FVector2D PlanNormal(const FVector2D& Direction) { return FVector2D(-Direction.Y, Direction.X); }

			/** The signed turn at a station, positive towards the plan normal (-dy, dx); zero at an open path's ends. */
			double Turn(int32 Station) const
			{
				const FVector2D In = InDirection(Station);
				const FVector2D Out = OutDirection(Station);
				return FMath::Atan2(FVector2D::CrossProduct(In, Out), FVector2D::DotProduct(In, Out));
			}

			/** False, with an ensure naming the station, where the path doubles back too sharply to mitre. */
			bool CanMitre() const
			{
				for (int32 Station = 0; Station < Points.Num(); ++Station)
				{
					if (FMath::Cos(0.5 * Turn(Station)) < 1.0 / MaxMitreStretch)
					{
						ensureMsgf(false, TEXT("A plan path doubles back at station %d: it has no mitre."), Station);
						return false;
					}
				}
				return true;
			}

			FSweepFrame Frame(int32 Station, ESide Side, double ElevationMeters, double U) const
			{
				const FVector2D InNormal = PlanNormal(InDirection(Station));
				const FVector2D OutNormal = PlanNormal(OutDirection(Station));
				const FVector2D Mitre = (InNormal + OutNormal).GetSafeNormal();
				const FVector2D Normal = Side == ESide::Arriving ? InNormal : Side == ESide::Leaving ? OutNormal : Mitre;
				const FVector2D Bisector = (InDirection(Station) + OutDirection(Station)).GetSafeNormal();
				const FVector2D Along = Side == ESide::Arriving ? InDirection(Station)
					: Side == ESide::Leaving ? OutDirection(Station) : Bisector;
				const FVector2D Stretched = Mitre / FVector2D::DotProduct(Mitre, InNormal);
				FSweepFrame F;
				F.Origin = FVector(Points[Station].X, Points[Station].Y, ElevationMeters);
				F.AxisA = FVector(Stretched.X, Stretched.Y, 0.0);
				F.AxisB = FVector::UpVector;
				F.NormalA = FVector(Normal.X, Normal.Y, 0.0);
				F.NormalB = FVector::UpVector;
				F.Along = FVector(Along.X, Along.Y, 0.0);
				F.U = U;
				return F;
			}
		};
	}

	TArray<TArray<FSweepFrame>> PlanRuns(TConstArrayView<FVector2D> Path, TConstArrayView<bool> CrispAt, bool bClosed,
		double ElevationMeters)
	{
		TArray<TArray<FSweepFrame>> Runs;
		if (Path.Num() < 2 || CrispAt.Num() != Path.Num())
		{
			return Runs;
		}
		const FPlanPath Plan(Path, bClosed);
		if (!Plan.CanMitre())
		{
			return Runs;
		}
		auto IsCrisp = [&Plan, &CrispAt](int32 Station) { return !Plan.IsEnd(Station) && CrispAt[Station]; };
		// A closed path starts at its first crisp station, so no run crosses its start; with none it is one ring.
		int32 Start = 0;
		while (bClosed && Start < Path.Num() && !IsCrisp(Start))
		{
			++Start;
		}
		Start %= Path.Num();
		const int32 Steps = bClosed ? Path.Num() : Path.Num() - 1;
		TArray<FSweepFrame> Run;
		double Travelled = 0.0;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const int32 Station = (Start + Step) % Path.Num();
			if (Step > 0)
			{
				const int32 Previous = (Start + Step - 1) % Path.Num();
				Travelled += FVector2D::Distance(Path[Previous], Path[Station]);
			}
			auto Frame = [&](ESide Side) { return Plan.Frame(Station, Side, ElevationMeters, Travelled); };
			const bool bCrisp = IsCrisp(Station);
			if (Step == 0)
			{
				Run.Add(Frame(bCrisp ? ESide::Leaving : ESide::Both));
			}
			else if (Step == Steps)
			{
				Run.Add(Frame(bCrisp ? ESide::Arriving : ESide::Both));
				Runs.Add(MoveTemp(Run));
			}
			else if (bCrisp)
			{
				Run.Add(Frame(ESide::Arriving));
				Runs.Add(MoveTemp(Run));
				Run.Reset();
				Run.Add(Frame(ESide::Leaving));
			}
			else
			{
				Run.Add(Frame(ESide::Both));
			}
		}
		return Runs;
	}

	TArray<TArray<FSweepFrame>> PlanRuns(TConstArrayView<FVector2D> Path, bool bClosed, double ElevationMeters,
		double CreaseRadians)
	{
		if (Path.Num() < 2)
		{
			return {};
		}
		const FPlanPath Plan(Path, bClosed);
		TArray<bool> CrispAt;
		for (int32 Station = 0; Station < Path.Num(); ++Station)
		{
			CrispAt.Add(FMath::Abs(Plan.Turn(Station)) > CreaseRadians);
		}
		return PlanRuns(Path, CrispAt, bClosed, ElevationMeters);
	}

	TArray<TArray<FSweepFrame>> PlanRuns(const FProfileStations& Path, double ElevationMeters)
	{
		TArray<bool> CrispAt;
		for (const FProfileVertex& Vertex : Path.Vertices)
		{
			CrispAt.Add(Vertex.bCrisp);
		}
		return PlanRuns(Path.Points(), CrispAt, Path.bClosed, ElevationMeters);
	}
}
