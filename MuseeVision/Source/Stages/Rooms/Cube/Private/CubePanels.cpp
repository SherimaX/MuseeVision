#include "CubeParts.h"

/**
 * The panel wall (§3.2), Light's image luminaire: six faces of light-field modules on a 0.700 grid, 40 by 40 to a face
 * with a module corner at each face's centre. Each module is a 0.698 square face (2 mm seams) and 0.100 deep behind it;
 * the faces meet on the room's edges without overlapping, leaving a 0.100 square void behind each edge. The ceiling's
 * rough opening, the circle r 4.130 round the soffit ring, omits the 88 modules wholly inside it and shapes the 44 it
 * crosses. Every module is its own solid; Light draws their housings behind them from its record.
 */
namespace Musee::Cube
{
	namespace
	{
		/** A face of the room: its name, its centre on its plane, and its axes, x and y across it, z away from the room. */
		struct FPanelFace
		{
			const TCHAR* Name;
			FVector Centre;
			FVector X;
			FVector Y;
			/** Whether the ceiling's rough opening, about the face's centre, cuts it. */
			bool bOpening = false;
		};

		TArray<FPanelFace> Faces()
		{
			using namespace Plan;
			const double Mid = 0.5 * (Ceiling + Floor);
			return {
				{TEXT("ceiling"), FVector(0.0, 0.0, Ceiling), FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), true},
				{TEXT("floor"), FVector(0.0, 0.0, Floor), FVector(1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0)},
				{TEXT("north"), FVector(0.0, -FaceHalf, Mid), FVector(1.0, 0.0, 0.0), FVector(0.0, 0.0, 1.0)},
				{TEXT("south"), FVector(0.0, FaceHalf, Mid), FVector(-1.0, 0.0, 0.0), FVector(0.0, 0.0, 1.0)},
				{TEXT("east"), FVector(FaceHalf, 0.0, Mid), FVector(0.0, 1.0, 0.0), FVector(0.0, 0.0, 1.0)},
				{TEXT("west"), FVector(-FaceHalf, 0.0, Mid), FVector(0.0, -1.0, 0.0), FVector(0.0, 0.0, 1.0)},
			};
		}

		/** Where a module's face stands on its face, in the face's axes: the centre of cell (I, J). */
		FVector2D CellCentre(int32 I, int32 J)
		{
			const double First = -Plan::FaceHalf + 0.5 * Plan::Pitch;
			return FVector2D(First + Plan::Pitch * I, First + Plan::Pitch * J);
		}

		/** The corners of a module's face about its centre, anticlockwise in its face's axes. */
		TArray<FVector2D> ModuleCorners()
		{
			const double Half = 0.5 * (Plan::Pitch - Plan::Seam);
			return {FVector2D(-Half, -Half), FVector2D(Half, -Half), FVector2D(Half, Half), FVector2D(-Half, Half)};
		}

		/** The parameters (0 to 1) where the segment P → Q crosses the circle about C of radius R, in order along it. */
		TArray<double> Crossings(const FVector2D& P, const FVector2D& Q, const FVector2D& C, double R)
		{
			const FVector2D D = Q - P;
			const FVector2D F = P - C;
			const double A = FVector2D::DotProduct(D, D);
			const double B = 2.0 * FVector2D::DotProduct(F, D);
			const double Disc = B * B - 4.0 * A * (FVector2D::DotProduct(F, F) - R * R);
			TArray<double> Out;
			if (Disc > 0.0)
			{
				for (const double Sign : {-1.0, 1.0})
				{
					const double T = (-B + Sign * FMath::Sqrt(Disc)) / (2.0 * A);
					if (T > 0.0 && T < 1.0)
					{
						Out.Add(T);
					}
				}
			}
			return Out;
		}

		double DegreesAbout(const FVector2D& C, const FVector2D& P)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(P.Y - C.Y, P.X - C.X));
		}

		/**
		 * A module's outline less the disc about C of radius R, which crosses it: the square's corners outside the circle
		 * and, where the square's edge runs inside it, the circle's short arc from where the edge enters to where it leaves.
		 */
		FFormProfile ShapedOutline(TConstArrayView<FVector2D> Corners, const FVector2D& C, double R)
		{
			int32 Start = 0;
			while (FVector2D::Distance(Corners[Start], C) <= R)
			{
				++Start;
			}
			FFormProfile Outline = FFormProfile::From(Corners[Start]);
			// Walking round from a corner outside, the crossings alternate: into the disc, then out of it.
			bool bInside = false;
			FVector2D Entered = FVector2D::ZeroVector;
			for (int32 Step = 0; Step < Corners.Num(); ++Step)
			{
				const FVector2D P = Corners[(Start + Step) % Corners.Num()];
				const FVector2D Q = Corners[(Start + Step + 1) % Corners.Num()];
				for (const double T : Crossings(P, Q, C, R))
				{
					const FVector2D X = P + T * (Q - P);
					bInside = !bInside;
					if (bInside)
					{
						Entered = X;
						Outline.LineTo(X);
						continue;
					}
					const double From = DegreesAbout(C, Entered);
					const double Turn = FMath::UnwindDegrees(DegreesAbout(C, X) - From);
					Outline.ArcAbout(C, R, R, From, From + Turn);
				}
				if (Step + 1 < Corners.Num() && FVector2D::Distance(Q, C) > R)
				{
					Outline.LineTo(Q);
				}
			}
			return Outline.Close();
		}
	}

	void DrawPanels(TArray<FFormDrawing>& Out)
	{
		const TArray<FVector2D> Corners = ModuleCorners();
		const FFormProfile Whole = FFormProfile::Polyline(Corners, true);
		for (const FPanelFace& Face : Faces())
		{
			for (int32 I = 0; I < Plan::ModulesPerEdge; ++I)
			{
				for (int32 J = 0; J < Plan::ModulesPerEdge; ++J)
				{
					const FVector2D At = CellCentre(I, J);
					FFormProfile Outline = Whole;
					if (Face.bOpening)
					{
						// The axis lies at the face's centre: in the module's own axes, at minus its place.
						const FVector2D Axis = -At;
						double Far = 0.0;
						for (const FVector2D& Corner : Corners)
						{
							Far = FMath::Max(Far, FVector2D::Distance(Corner, Axis));
						}
						const double Half = Corners[2].X;
						const double Near = FVector2D::Distance(FVector2D(FMath::Clamp(Axis.X, -Half, Half), FMath::Clamp(Axis.Y, -Half, Half)), Axis);
						if (Far <= Plan::RoughOpening)
						{
							continue;
						}
						if (Near < Plan::RoughOpening)
						{
							Outline = ShapedOutline(Corners, Axis, Plan::RoughOpening);
						}
					}
					const FVector Origin = Face.Centre + At.X * Face.X + At.Y * Face.Y;
					Out.Add(Drawn(FString::Printf(TEXT("%s%s.%d.%d"), Pieces::Panel, Face.Name, I + 1, J + 1), TEXT("panels"),
						Prism(MoveTemp(Outline), 0.0, Plan::ModuleDepth), Solid, FrameOfAxes(Origin, Face.X, Face.Y)));
				}
			}
		}
	}
}
