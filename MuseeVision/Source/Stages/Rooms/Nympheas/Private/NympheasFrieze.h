#pragma once

#include "CoreMinimal.h"

/**
 * The carved lily frieze on the coping's die (§3.5), Art Nouveau after Gallé and Majorelle: the carver's drawing, as
 * a height field over the band. Along each half of the pond from the east keystone (u 0) to the west (u Half): a
 * keystone motif at each apex (a lily seen from the front, rings of 12, 10 and 8 petals round a stamen crown, a pad and
 * a bud either side), and between them repeats of about 1 m (a whiplash rhizome, two overlapping pads and a third lifted
 * on its stalk, an open lily on its own stem, every third half-open, a bud high on the rhizome). The north and south
 * halves mirror. The drawing is the as-built design (windows/Source/MuseeVision/Salon/SalonPond.cpp:168-441), whose
 * dimensions are the carver's: petals, pads and stems in metres on the band.
 */
namespace Musee::Nympheas
{
	class FFriezeDesign
	{
	public:
		/** The design laid out along a half of HalfMeters, on a band BandMeters tall. */
		FFriezeDesign(double HalfMeters, double BandMeters);

		/** How far the carving stands proud at u along the half (0 at the east keystone) and v up from the band's foot, metres. */
		double Height(double U, double V) const;

	private:
		/** One carved shape in the band's plane: a pad, a petal, a stem along a curve, or a round boss. */
		struct FShape
		{
			enum class EKind : uint8 { Pad, Petal, Stem, Bump };
			EKind Kind = EKind::Bump;
			double X = 0.0;
			double Y = 0.0;
			double A = 0.0;
			double B = 0.0;
			double H = 0.0;
			double Angle = 0.0;
			double Rot = 0.0;
			double Fat = 0.7;
			double Base = 0.55;
			bool bGroove = true;
			TArray<FVector2D> Points;
			double W0 = 0.0;
			double W1 = 0.0;
			FBox2D Box = FBox2D(ForceInit);

			double Height(double U, double V) const;
		};

		void AddPad(double X, double Y, double A, double B, double H, double Notch, double Rot);
		void AddPetal(double X, double Y, double Angle, double Length, double Width, double H, bool bGroove = true, double Fat = 0.7,
			double Base = 0.55);
		void AddBump(double X, double Y, double Radius, double H);
		void AddStem(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, double W0, double W1, double H);
		void FlowerSide(double CX, double CY, double Size, double H, double Lean, int32 K, bool bHalfOpen);
		void Bud(double X, double Y, double Angle, double Length, double Width, double H);
		void Keystone(double CX, bool bMirror);
		void Repeat(double X0, int32 K);
		void Index();

		double Half = 0.0;
		double Band = 0.0;
		double Lambda = 1.0;
		TArray<FShape> Shapes;
		/** The shapes reaching into each 50 mm bin along u, so a height looks only at its neighbours. */
		TArray<TArray<int32>> Bins;
	};
}
