#include "AlbionParts.h"

/**
 * The vaults' iron under the glass (§3.5), set out on each half's arc by its depth under the glass: the main ribs on the
 * column lines, built-up I sections on their shoes, each with its inner arch blending up into it and the chain of rings
 * between them; the light ribs every 1.50 between; the purlins along the court and the ridge bars at the crowns; the
 * glazing bars under the panes' joints; the side bearers and the brackets under the light ribs' feet at the side walls.
 */
namespace Musee::Albion
{
	namespace
	{
		FVector InSection(const FVector2D& P, double Y) { return FVector(P.X, Y, P.Y); }

		/** A rectangle in (depth under the glass, across), from one depth to another and a half-width either side. */
		FFormProfile Strip(double From, double To, double Half)
		{
			return FFormProfile::Rectangle(FVector2D(From, -Half), FVector2D(To, Half));
		}

		/** The main rib: outer flange, web and inner flange, (depth, across), 0.15 to 0.41 under the glass. */
		FFormProfile MainRibSection(bool bEnd)
		{
			const double Top = Plan::RibTop;
			const double Low = Top + Plan::MainRibDepth;
			const double F = Plan::MainFlangeMeters;
			const double Outer = bEnd ? Plan::MainOuterHalfEnd : Plan::MainOuterHalf;
			const double Inner = bEnd ? Plan::MainInnerHalfEnd : Plan::MainInnerHalf;
			const double Web = Plan::MainWebHalf;
			return Section({FVector2D(Top, -Outer), FVector2D(Top + F, -Outer), FVector2D(Top + F, -Web), FVector2D(Low - F, -Web), FVector2D(Low - F, -Inner),
				FVector2D(Low, -Inner), FVector2D(Low, Inner), FVector2D(Low - F, Inner), FVector2D(Low - F, Web), FVector2D(Top + F, Web), FVector2D(Top + F, Outer),
				FVector2D(Top, Outer)});
		}

		/** The light rib: a tee, its table under the purlins, its stem and the bead on its edge, (depth, across). */
		FFormProfile LightRibSection()
		{
			const double Top = Plan::RibTop;
			const double Table = Top + Plan::LightTableMeters;
			const double Low = Top + Plan::LightRibDepth;
			const double Bead = Low - Plan::LightBeadMeters;
			const double T = Plan::LightTableHalf;
			const double S = Plan::LightStemHalf;
			const double B = Plan::LightBeadHalf;
			return Section({FVector2D(Top, -T), FVector2D(Table, -T), FVector2D(Table, -S), FVector2D(Bead, -S), FVector2D(Bead, -B), FVector2D(Low, -B),
				FVector2D(Low, B), FVector2D(Bead, B), FVector2D(Bead, S), FVector2D(Table, S), FVector2D(Table, T), FVector2D(Top, T)});
		}

		/** A glazing bar: its web from 3 mm under the glass, its foot, (depth, across). */
		FFormProfile GlazingBarSection()
		{
			const double Top = Plan::BarTop;
			const double Low = Plan::BarFoot;
			const double Foot = Low - Plan::BarFootMeters;
			const double W = Plan::BarWebHalf;
			const double F = Plan::BarFootHalf;
			return Section({FVector2D(Top, -W), FVector2D(Foot, -W), FVector2D(Foot, -F), FVector2D(Low, -F), FVector2D(Low, F), FVector2D(Foot, F),
				FVector2D(Foot, W), FVector2D(Top, W)});
		}

		/** The inner arch's top under the glass s along the arc: 0.62 at the springing, blending as (s/L)^6 into the rib. */
		double InnerArchTop(const FVaultHalf& H, double S)
		{
			const double Reach = Plan::InnerArchReach * H.Length();
			const double Rib = Plan::RibTop + Plan::MainRibDepth;
			return Plan::InnerArchDepth - (Plan::InnerArchDepth - Rib) * FMath::Pow(FMath::Min(S / Reach, 1.0), Plan::InnerArchExponent);
		}

		/** The stations the inner arch is set out at, about every 0.10 of arc. */
		constexpr double InnerArchStep = 0.100;

		/**
		 * One half of a main rib on its line: the rib from its shoe to the crown, its shoe, its inner arch and its chain of
		 * rings, each ring midway between the rib's foot and the inner arch.
		 */
		void MainRib(const FVaultHalf& H, int32 Line, TArray<FFormDrawing>& Out)
		{
			const double Y = Plan::LineY(Line);
			const bool bEnd = Line == 1 || Line == Plan::Lines;
			const FString Name = FString::Printf(TEXT("vault.rib.%d.%s"), Line, H.Name);
			const double From = H.SAtHeight(Plan::Springing + Plan::RibShoeMeters);
			const double Length = H.Length();
			FFormSweep Rib;
			Rib.Section = MainRibSection(bEnd);
			Rib.Path = H.Arc(0.0, From, Length);
			Out.Add(Drawn(Name, TEXT("iron.cast"), MoveTemp(Rib), Overhead, AcrossY(Y)));

			const double ShoeX = H.SpringX + H.Sign() * (Plan::RibTop + 0.5 * Plan::MainRibDepth);
			Out.Add(Box(Name + TEXT(".shoe"), TEXT("iron.cast"), FVector(ShoeX - Plan::RibShoeHalfX, Y - Plan::RibShoeHalfY, Plan::Springing),
				FVector(ShoeX + Plan::RibShoeHalfX, Y + Plan::RibShoeHalfY, Plan::Springing + Plan::RibShoeMeters), Overhead));

			const double Reach = Plan::InnerArchReach * Length;
			const int32 Steps = FMath::CeilToInt32((Reach - From) / InnerArchStep);
			TArray<FVector2D> Points;
			for (int32 Index = 0; Index <= Steps; ++Index)
			{
				const double S = FMath::Lerp(From, Reach, double(Index) / Steps);
				Points.Add(H.At(S, InnerArchTop(H, S)));
			}
			FFormSweep Arch;
			Arch.Section = Strip(0.0, Plan::InnerArchBar, Plan::InnerArchHalf);
			Arch.Path = H.Path(MoveTemp(Points), true);
			Out.Add(Drawn(Name + TEXT(".arch"), TEXT("iron.wrought"), MoveTemp(Arch), Overhead, AcrossY(Y)));

			const double RibFoot = Plan::RibTop + Plan::MainRibDepth;
			int32 RingIndex = 0;
			for (double S = Plan::ChainPitch; S <= Plan::ChainReach * Length; S += Plan::ChainPitch)
			{
				const FVector Centre = InSection(H.At(S, 0.5 * (RibFoot + InnerArchTop(H, S))), Y);
				Out.Add(Ring(FString::Printf(TEXT("%s.ring.%02d"), *Name, ++RingIndex), TEXT("iron.wrought"),
					FrameOfAxes(Centre, FVector(1.0, 0.0, 0.0), FVector::UpVector), Plan::ChainRing, Plan::ChainTube, Overhead));
			}
		}

		/** Whether a line along the court is one of the column lines. */
		bool IsColumnLine(double Y)
		{
			for (int32 Line = 1; Line <= Plan::Lines; ++Line)
			{
				if (FMath::IsNearlyEqual(Y, Plan::LineY(Line), 1e-6))
				{
					return true;
				}
			}
			return false;
		}

		/** The light ribs' lines, every 1.50 from -16.5 to 16.5, the column lines left to the main ribs: eighteen. */
		TArray<double> LightRibLines()
		{
			TArray<double> Lines;
			const double First = Plan::FirstLineY + Plan::LightRibPitch;
			for (double Y = First; Y < -First + 1e-6; Y += Plan::LightRibPitch)
			{
				if (!IsColumnLine(Y))
				{
					Lines.Add(Y);
				}
			}
			return Lines;
		}

		void Ribs(TArray<FFormDrawing>& Out)
		{
			for (const FVaultHalf& H : VaultHalves())
			{
				for (int32 Line = 1; Line <= Plan::Lines; ++Line)
				{
					MainRib(H, Line, Out);
				}
				const TArray<double> Lines = LightRibLines();
				for (int32 Index = 0; Index < Lines.Num(); ++Index)
				{
					FFormSweep Rib;
					Rib.Section = LightRibSection();
					Rib.Path = H.Arc(0.0, 0.0, H.Length());
					Out.Add(Drawn(FString::Printf(TEXT("vault.light-rib.%02d.%s"), Index + 1, H.Name), TEXT("iron.cast"), MoveTemp(Rib), Overhead,
						AcrossY(Lines[Index])));
				}
			}
		}

		/**
		 * The purlins along the court on each half, evenly up the arc (about 1.8 m apart), a tee turned to the arc's normal:
		 * its flange 0.05 under the glass, its web down to the ribs' tops. And a ridge bar at each crown.
		 */
		void Purlins(TArray<FFormDrawing>& Out)
		{
			const double Run = 2.0 * Plan::PurlinEndY;
			for (const FVaultHalf& H : VaultHalves())
			{
				const int32 Count = H.bNave ? Plan::NavePurlins : Plan::AislePurlins;
				for (int32 Index = 1; Index <= Count; ++Index)
				{
					const double S = H.Length() * Index / (Count + 1);
					const FVector2D N = H.Normal(S);
					const FVector2D T(-N.Y, N.X);
					const FVector2D Top = H.At(S, Plan::PurlinTop);
					const FVector2D Flange = H.At(S, Plan::PurlinTop + Plan::PurlinFlangeMeters);
					const FVector2D Foot = H.At(S, Plan::RibTop);
					const double F = Plan::PurlinFlangeHalf;
					const double W = Plan::PurlinWebHalf;
					FFormExtrusion Shape;
					Shape.Outline = Section({Top - T * F, Top + T * F, Flange + T * F, Flange + T * W, Foot + T * W, Foot - T * W, Flange - T * W, Flange - T * F});
					Shape.DepthMeters = Run;
					Out.Add(Drawn(FString::Printf(TEXT("vault.purlin.%s.%d"), H.Name, Index), TEXT("iron.cast"), MoveTemp(Shape), Overhead,
						AcrossY(Plan::PurlinEndY)));
				}
			}
			const TPair<const TCHAR*, FVector2D> Crowns[] = {{TEXT("nave"), FVector2D(0.0, Plan::NaveCrown)},
				{TEXT("west"), FVector2D(-Plan::AisleCrownX, Plan::AisleCrown)}, {TEXT("east"), FVector2D(Plan::AisleCrownX, Plan::AisleCrown)}};
			for (const TPair<const TCHAR*, FVector2D>& Crown : Crowns)
			{
				FFormExtrusion Shape;
				Shape.Outline = FFormProfile::Rectangle(FVector2D(Crown.Value.X - Plan::RidgeBarHalf, Crown.Value.Y - Plan::RibTop),
					FVector2D(Crown.Value.X + Plan::RidgeBarHalf, Crown.Value.Y - Plan::RidgeBarTop));
				Shape.DepthMeters = Run;
				Out.Add(Drawn(FString::Printf(TEXT("vault.ridge-bar.%s"), Crown.Key), TEXT("iron.cast"), MoveTemp(Shape), Overhead, AcrossY(Plan::PurlinEndY)));
			}
		}

		/**
		 * The glazing bars, 48 a half at 36.6/49 across the court, under the panes' chords from 10.10 to the ridge bar: each
		 * straight under its pane, turning at the pane's joint.
		 */
		void GlazingBars(TArray<FFormDrawing>& Out)
		{
			const double Pitch = 2.0 * Plan::GlassEndY / Plan::GlazingBays;
			for (const FVaultHalf& H : VaultHalves())
			{
				TArray<FVector2D> Chords;
				for (const double S : H.PaneStations())
				{
					Chords.Add(H.At(S));
				}
				const FVector2D Crown = Chords.Pop();
				Chords.Add(Crown + (Chords.Last() - Crown).GetSafeNormal() * Plan::RidgeBarHalf);
				for (int32 BarIndex = 1; BarIndex < Plan::GlazingBays; ++BarIndex)
				{
					FFormSweep Shape;
					Shape.Section = GlazingBarSection();
					Shape.Path = H.Path(Chords);
					Out.Add(Drawn(FString::Printf(TEXT("vault.bar.%s.%02d"), H.Name, BarIndex), TEXT("iron.cast"), MoveTemp(Shape), Overhead,
						AcrossY(-Plan::GlassEndY + Pitch * BarIndex)));
				}
			}
		}

		/**
		 * The side bearers along each side wall at the springing, and under each light rib's foot a cast bracket: its plate
		 * up the wall from 9.65 and its strut out to 0.33 under the bearer, cast as one web.
		 */
		void Bearers(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Face = S * Plan::SideInnerX;
				const double Reach = Face - S * Plan::BearerReach;
				const TCHAR* Side = bWest ? TEXT("west") : TEXT("east");
				Out.Add(Box(FString::Printf(TEXT("vault.bearer.%s"), Side), TEXT("iron.cast"), FVector(FMath::Min(Face, Reach), -Plan::EndInnerY, Plan::BearerFoot),
					FVector(FMath::Max(Face, Reach), Plan::EndInnerY, Plan::Springing), Overhead));
				const double Plate = Face - S * Plan::BracketMeters;
				const double Strut = Face - S * Plan::BracketReach;
				const TArray<double> Lines = LightRibLines();
				for (int32 Index = 0; Index < Lines.Num(); ++Index)
				{
					FFormExtrusion Web;
					Web.Outline = Section({FVector2D(Face, Plan::BracketFoot), FVector2D(Plate, Plan::BracketFoot), FVector2D(Strut, Plan::BracketStrutTop),
						FVector2D(Strut, Plan::BearerFoot), FVector2D(Face, Plan::BearerFoot)});
					Web.DepthMeters = Plan::BracketMeters;
					Out.Add(Drawn(FString::Printf(TEXT("vault.bracket.%s.%02d"), Side, Index + 1), TEXT("iron.cast"), MoveTemp(Web), Overhead,
						AcrossY(Lines[Index] + 0.5 * Plan::BracketMeters)));
				}
			}
		}
	}

	void DrawVaults(TArray<FFormDrawing>& Out)
	{
		Ribs(Out);
		Purlins(Out);
		GlazingBars(Out);
		Bearers(Out);
	}
}
