#include "Weather/ClimateNormals.h"

namespace Musee::Time::MonthlyCurveMath
{
	/** The angle of harmonic k at a point of the year: 2 pi k t / 12. */
	double HarmonicAngle(int32 Harmonic, double YearMonths)
	{
		return 2.0 * UE_DOUBLE_PI * Harmonic * YearMonths / NormalsMonthCount;
	}

	/** How much averaging over one month damps harmonic k: sin(pi k / 12) / (pi k / 12). */
	double MonthAveragingDamping(int32 Harmonic)
	{
		const double HalfMonthAngle = UE_DOUBLE_PI * Harmonic / NormalsMonthCount;
		return FMath::Sin(HalfMonthAngle) / HalfMonthAngle;
	}
}

namespace Musee::Time
{
	FMonthlyCurve::FMonthlyCurve(const TStaticArray<double, NormalsMonthCount>& MonthlyMeans)
	{
		using namespace MonthlyCurveMath;
		for (const double Value : MonthlyMeans) { Mean += Value / NormalsMonthCount; }
		for (int32 Harmonic = 1; Harmonic <= HarmonicCount; ++Harmonic)
		{
			// The Nyquist harmonic (k = 6) has half the weight of the others in the discrete transform.
			const double Weight = (Harmonic == HarmonicCount ? 1.0 : 2.0) / NormalsMonthCount;
			double CosineSum = 0.0;
			double SineSum = 0.0;
			for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
			{
				const double Angle = HarmonicAngle(Harmonic, Month + 0.5);
				CosineSum += MonthlyMeans[Month] * FMath::Cos(Angle);
				SineSum += MonthlyMeans[Month] * FMath::Sin(Angle);
			}
			const double Damping = MonthAveragingDamping(Harmonic);
			Cosine[Harmonic - 1] = Weight * CosineSum / Damping;
			Sine[Harmonic - 1] = Weight * SineSum / Damping;
		}
	}

	double FMonthlyCurve::At(double InYearMonths) const
	{
		using namespace MonthlyCurveMath;
		double Value = Mean;
		for (int32 Harmonic = 1; Harmonic <= HarmonicCount; ++Harmonic)
		{
			const double Angle = HarmonicAngle(Harmonic, InYearMonths);
			Value += Cosine[Harmonic - 1] * FMath::Cos(Angle) + Sine[Harmonic - 1] * FMath::Sin(Angle);
		}
		return Value;
	}

	double YearMonths(const FDateTime& Date)
	{
		const int32 DaysInMonth = FDateTime::DaysInMonth(Date.GetYear(), Date.GetMonth());
		const double DayOfMonth = (Date.GetDay() - 1) + Date.GetTimeOfDay().GetTotalDays();
		return (Date.GetMonth() - 1) + DayOfMonth / DaysInMonth;
	}
}
