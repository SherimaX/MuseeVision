#include "AlbionParts.h"

/**
 * The end screens and the gables (§3.5, §3.6): on the planes y ±18.30, the nave's iron screens above its cills, their
 * mullions, glazing bars and transoms stopping 0.02 under the vault, glazed between them; the south screen carrying the
 * Tristram and Isolde band, twelve leaded panels whose images are Art's (Glazing anchors, D-014), each behind its
 * protective pane; and the aisles' gables above their walls. The iron and the glass stand on the court's side of the
 * plane, the glass on its line, so nothing passes into the parapets outside.
 */
namespace Musee::Albion
{
	namespace
	{
		/** A vertical member of a screen: its axis, half its width, its depth from the plane into the court. */
		struct FMember
		{
			double X;
			double Half;
			double Deep;
		};

		/** A transom: its axis, half its height, its depth. */
		struct FTransom
		{
			double Z;
			double Half;
			double Deep;
		};

		/** The vault half over x in a screen (nave or aisle), whose arc a screen's top follows. */
		const FVaultHalf& HalfOver(double X)
		{
			const TConstArrayView<FVaultHalf> Halves = VaultHalves();
			const double A = FMath::Abs(X);
			if (A <= Plan::ArcadeX)
			{
				return Halves[X < 0.0 ? 0 : 1];
			}
			const bool bWest = X < 0.0;
			const bool bOuter = A > Plan::AisleCrownX;
			return Halves[bWest ? (bOuter ? 2 : 3) : (bOuter ? 5 : 4)];
		}

		/** The screens' tops: 0.02 under the vault's arc. */
		double UnderArc(double X)
		{
			return GlassZ(X) - Plan::UnderArc;
		}

		/** A member's top: under the arc over its lower face. */
		double TopOf(const FMember& M)
		{
			return FMath::Min(UnderArc(M.X - M.Half), UnderArc(M.X + M.Half));
		}

		/** One screen or gable: its plane, which way the court lies, its name, its base, its members and transoms. */
		struct FScreen
		{
			FString Name;
			FString Glazing;
			double Plane = 0.0;
			/** +1 when the court lies north of the plane (the south screen), -1 when south. */
			double Inward = 1.0;
			double Base = 0.0;
			/** Its glass's outer edges, where no member closes it (a gable's springings). */
			double Left = 0.0;
			double Right = 0.0;
			TArray<FMember> Members;
			TArray<FTransom> Transoms;
			/** The south screen's band, x ±4.8 from 9.00 to 12.60, which its own glass fills. */
			bool bBand = false;
		};

		/** A box on the screen's court side, from the plane in by a depth. */
		FFormDrawing OnPlane(const FScreen& S, const FString& Name, double X0, double X1, double Z0, double Z1, double Deep)
		{
			const double Near = S.Plane;
			const double Far = S.Plane - S.Inward * Deep;
			return Box(Name, TEXT("iron.cast"), FVector(X0, FMath::Min(Near, Far), Z0), FVector(X1, FMath::Max(Near, Far), Z1), Overhead);
		}

		/** The frame of the screen's glass: the plane's face on the court side, pushed through the glass's thickness. */
		FMuseeFrame GlassFrame(const FScreen& S, double Thick, double Off = 0.0)
		{
			// AcrossY pushes north (-y): from the glass's south face.
			const double Court = S.Plane - S.Inward * Off;
			const double Other = Court - S.Inward * Thick;
			return AcrossY(FMath::Max(Court, Other));
		}

		/** Whether a height band of a column lies in the band's panels. */
		bool InBand(const FScreen& S, double X0, double X1, double Z0, double Z1)
		{
			return S.bBand && FMath::Max(FMath::Abs(X0), FMath::Abs(X1)) < Plan::BandEastX + 1e-6 && Z0 >= Plan::BandLowerFoot - 1e-6
				&& Z1 <= Plan::BandUpperFoot + Plan::BandPanelHigh + 1e-6;
		}

		/** The run of a transom: between the outer faces of the outermost members that rise above it. */
		bool TransomRun(const FScreen& S, const FTransom& T, double& OutX0, double& OutX1)
		{
			bool bAny = false;
			for (const FMember& M : S.Members)
			{
				if (TopOf(M) > T.Z + T.Half + 1e-6)
				{
					OutX0 = bAny ? FMath::Min(OutX0, M.X - M.Half) : M.X - M.Half;
					OutX1 = bAny ? FMath::Max(OutX1, M.X + M.Half) : M.X + M.Half;
					bAny = true;
				}
			}
			return bAny;
		}

		/** A pane of glass from Z0 up to the arc, between X0 and X1 where the arc stands above Z0. */
		bool UnderArcPane(double X0, double X1, double Z0, FFormProfile& Out)
		{
			const FVaultHalf& H = HalfOver(0.5 * (X0 + X1));
			const double R = H.Radius() - Plan::UnderArc;
			const double CX = H.CentreX();
			const double Up = Z0 - Plan::Springing;
			if (Up >= R)
			{
				return false;
			}
			const double Meets = CX - H.Sign() * FMath::Sqrt(R * R - Up * Up);
			const double Low = UnderArc(X0) < Z0 ? FMath::Max(X0, FMath::Min(Meets, X1)) : X0;
			const double High = UnderArc(X1) < Z0 ? FMath::Min(X1, FMath::Max(Meets, X0)) : X1;
			if (High - Low < 1e-3)
			{
				return false;
			}
			auto Deg = [R, CX](double X) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((X - CX) / R, -1.0, 1.0))); };
			Out = FFormProfile::From(FVector2D(Low, Z0));
			Out.LineTo(FVector2D(High, Z0), EFormJoint::Crisp);
			Out.ArcAbout(FVector2D(CX, Plan::Springing), R, R, Deg(High), Deg(Low), EFormJoint::Crisp);
			Out.Close();
			return true;
		}

		/** The members and transoms of a screen, as boxes. */
		void Iron(const FScreen& S, TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < S.Members.Num(); ++Index)
			{
				const FMember& M = S.Members[Index];
				const FString Name = FString::Printf(TEXT("%s.member.%02d"), *S.Name, Index + 1);
				const double Top = TopOf(M);
				if (InBand(S, M.X - M.Half, M.X + M.Half, Plan::BandLowerFoot, Plan::BandUpperFoot + Plan::BandPanelHigh) && M.Half < Plan::ScreenMullion[0] * 0.5)
				{
					// A glazing bar stops at the band: under it and over it.
					const double Under = Plan::BandLowerFoot - 0.5 * Plan::BandTransom[0];
					const double Over = Plan::BandUpperFoot + Plan::BandPanelHigh + 0.5 * Plan::BandTransom[0];
					Out.Add(OnPlane(S, Name + TEXT(".low"), M.X - M.Half, M.X + M.Half, S.Base, Under, M.Deep));
					Out.Add(OnPlane(S, Name + TEXT(".high"), M.X - M.Half, M.X + M.Half, Over, Top, M.Deep));
					continue;
				}
				Out.Add(OnPlane(S, Name, M.X - M.Half, M.X + M.Half, S.Base, Top, M.Deep));
			}
			for (int32 Index = 0; Index < S.Transoms.Num(); ++Index)
			{
				const FTransom& T = S.Transoms[Index];
				double X0 = 0.0;
				double X1 = 0.0;
				if (TransomRun(S, T, X0, X1))
				{
					Out.Add(OnPlane(S, FString::Printf(TEXT("%s.transom.%d"), *S.Name, Index + 1), X0, X1, T.Z - T.Half, T.Z + T.Half, T.Deep));
				}
			}
		}

		/**
		 * The screen's glass, column by column between its members and row by row between the transoms that cross the
		 * column, the top row up to the arc; the band's panels left to the band.
		 */
		void ScreenPanes(const FScreen& S, TArray<FFormDrawing>& Out)
		{
			const double Thick = Plan::LaminatedMeters;
			TArray<TPair<double, double>> Columns;
			double From = S.Left;
			for (const FMember& M : S.Members)
			{
				if (M.X - M.Half > From + 1e-3)
				{
					Columns.Add({From, M.X - M.Half});
				}
				From = M.X + M.Half;
			}
			if (S.Right > From + 1e-3)
			{
				Columns.Add({From, S.Right});
			}
			TArray<FTransom> Rising = S.Transoms;
			Rising.Sort([](const FTransom& A, const FTransom& B) { return A.Z < B.Z; });
			for (int32 Column = 0; Column < Columns.Num(); ++Column)
			{
				const double X0 = Columns[Column].Key;
				const double X1 = Columns[Column].Value;
				int32 Row = 0;
				double Z = S.Base;
				auto Pane = [&](FFormProfile Outline)
				{
					FFormExtrusion Shape;
					Shape.Outline = MoveTemp(Outline);
					Shape.DepthMeters = Thick;
					Out.Add(Glass(Drawn(FString::Printf(TEXT("%s.glass.%02d.%d"), *S.Name, Column + 1, ++Row), S.Glazing, MoveTemp(Shape), Overhead,
						GlassFrame(S, Thick))));
				};
				for (const FTransom& T : Rising)
				{
					double T0 = 0.0;
					double T1 = 0.0;
					if (!TransomRun(S, T, T0, T1) || T0 > X0 + 1e-6 || T1 < X1 - 1e-6)
					{
						continue;
					}
					if (!InBand(S, X0, X1, Z, T.Z - T.Half))
					{
						Pane(FFormProfile::Rectangle(FVector2D(X0, Z), FVector2D(X1, T.Z - T.Half)));
					}
					Z = T.Z + T.Half;
				}
				FFormProfile Top;
				if (UnderArcPane(X0, X1, Z, Top))
				{
					Pane(MoveTemp(Top));
				}
			}
		}

		/** The members of the nave's screens: the edge mullions against the returns, the others on their lines, the bars. */
		TArray<FMember> NaveMembers()
		{
			TArray<FMember> Members;
			for (const double X : Plan::ScreenMullionX)
			{
				const bool bEdge = FMath::IsNearlyEqual(FMath::Abs(X), Plan::ArcadeX);
				const double Half = 0.5 * (bEdge ? Plan::EdgeMullion[0] : Plan::ScreenMullion[0]);
				Members.Add({bEdge ? X - FMath::Sign(X) * Half : X, Half, bEdge ? Plan::EdgeMullion[1] : Plan::ScreenMullion[1]});
			}
			for (int32 BarIndex = 0; BarIndex < Plan::ScreenBars; ++BarIndex)
			{
				Members.Add({Plan::ScreenBarFirstX + Plan::ScreenBarPitch * BarIndex, 0.5 * Plan::ScreenBar[0], Plan::ScreenBar[1]});
			}
			Members.Sort([](const FMember& A, const FMember& B) { return A.X < B.X; });
			return Members;
		}

		/** The nave's two screens, the south one round the band, each on its iron cill. */
		void NaveScreens(TArray<FFormDrawing>& Out)
		{
			for (const bool bNorth : {true, false})
			{
				FScreen S;
				S.Name = bNorth ? TEXT("screen.north") : TEXT("screen.south");
				S.Glazing = TEXT("screen.glass");
				S.Plane = bNorth ? -Plan::EndGlassY : Plan::EndGlassY;
				S.Inward = bNorth ? -1.0 : 1.0;
				S.Base = Plan::ScreenCillTop;
				S.Members = NaveMembers();
				S.Left = S.Members[0].X + S.Members[0].Half;
				S.Right = S.Members.Last().X - S.Members.Last().Half;
				S.bBand = !bNorth;
				for (const double Z : Plan::ScreenTransomZ)
				{
					const bool bBandTransom = S.bBand && Z < Plan::BandUpperFoot + Plan::BandPanelHigh + 1e-6;
					const double(&Size)[2] = bBandTransom ? Plan::BandTransom : Plan::ScreenTransom;
					S.Transoms.Add({Z, 0.5 * Size[0], Size[1]});
				}
				S.Transoms.Add({Plan::UpperTransomZ, 0.5 * Plan::UpperTransom[0], Plan::UpperTransom[1]});
				Iron(S, Out);
				ScreenPanes(S, Out);
				Out.Add(OnPlane(S, S.Name + TEXT(".cill"), -Plan::ArcadeX, Plan::ArcadeX, Plan::CillFrontTop, Plan::ScreenCillTop, Plan::ScreenCillDeep));
				if (S.bBand)
				{
					// The clear strip between the band's rows, glazed between the mullions (the bars stop at the band).
					const double Mullion = 0.5 * Plan::ScreenMullion[0];
					const double Z0 = Plan::BandLowerFoot + Plan::BandPanelHigh + 0.5 * Plan::BandTransom[0];
					const double Z1 = Plan::BandUpperFoot - 0.5 * Plan::BandTransom[0];
					for (int32 Column = 0; Column < Plan::BandColumns; ++Column)
					{
						const double East = Plan::BandEastX - Plan::BandPanelWidth * Column;
						FFormExtrusion Clear;
						Clear.Outline = FFormProfile::Rectangle(FVector2D(East - Plan::BandPanelWidth + Mullion, Z0), FVector2D(East - Mullion, Z1));
						Clear.DepthMeters = Plan::LaminatedMeters;
						Out.Add(Glass(Drawn(FString::Printf(TEXT("screen.south.strip.%d"), Column + 1), TEXT("screen.glass"), MoveTemp(Clear), Overhead,
							GlassFrame(S, Plan::LaminatedMeters))));
					}
				}
			}
		}

		/** The aisles' four gables over their walls, glazed in the diffusing glass, on a cast cill. */
		void Gables(TArray<FFormDrawing>& Out)
		{
			for (const bool bNorth : {true, false})
			{
				for (const bool bWest : {true, false})
				{
					const double SX = bWest ? -1.0 : 1.0;
					FScreen S;
					S.Name = FString::Printf(TEXT("gable.%s%s"), bNorth ? TEXT("n") : TEXT("s"), bWest ? TEXT("w") : TEXT("e"));
					S.Glazing = TEXT("screen.glass.gable");
					S.Plane = bNorth ? -Plan::EndGlassY : Plan::EndGlassY;
					S.Inward = bNorth ? -1.0 : 1.0;
					S.Base = Plan::Springing + Plan::GableCillHigh;
					S.Left = bWest ? -Plan::SideInnerX : Plan::ArcadeX;
					S.Right = bWest ? -Plan::ArcadeX : Plan::SideInnerX;
					for (const double X : Plan::GableMullionX)
					{
						S.Members.Add({SX * X, 0.5 * Plan::GableMullion[0], Plan::GableMullion[1]});
					}
					S.Members.Sort([](const FMember& A, const FMember& B) { return A.X < B.X; });
					for (const double Z : Plan::GableTransomZ)
					{
						S.Transoms.Add({Z, 0.5 * Plan::GableTransom[0], Plan::GableTransom[1]});
					}
					Iron(S, Out);
					ScreenPanes(S, Out);
					Out.Add(OnPlane(S, S.Name + TEXT(".cill"), S.Left, S.Right, Plan::Springing, S.Base, Plan::GableCillDeep));
				}
			}
		}

		/**
		 * The Tristram and Isolde band: twelve panels 1.60 × 1.70, T1 to T6 in the upper row and T7 to T12 in the lower,
		 * read left to right from inside (east to west). Each is its border of leaded glass round the image's window, which
		 * Art glazes (the Glazing anchor albion.band.t<k>), the lead came round the image, and the protective pane outside.
		 */
		void TristramBand(TArray<FFormDrawing>& Out)
		{
			const double Plane = Plan::EndGlassY;
			const double Leaded = Plan::LeadedGlassMeters;
			const double Came = Plan::CameMeters;
			const double Border = Plan::BandBorder;
			const double Mullion = 0.5 * Plan::ScreenMullion[0];
			const double Transom = 0.5 * Plan::BandTransom[0];
			for (int32 Panel = 0; Panel < 2 * Plan::BandColumns; ++Panel)
			{
				const int32 Column = Panel % Plan::BandColumns;
				const double Foot = Panel < Plan::BandColumns ? Plan::BandUpperFoot : Plan::BandLowerFoot;
				const double East = Plan::BandEastX - Plan::BandPanelWidth * Column;
				const double West = East - Plan::BandPanelWidth;
				const double Head = Foot + Plan::BandPanelHigh;
				const FString Name = FString::Printf(TEXT("band.t%d"), Panel + 1);
				const FVector2D ImageMin(West + Border, Foot + Border);
				const FVector2D ImageMax(East - Border, Head - Border);

				FFormExtrusion Quarries;
				Quarries.Outline = FFormProfile::Rectangle(FVector2D(West + Mullion, Foot + Transom), FVector2D(East - Mullion, Head - Transom));
				Quarries.Holes.Add(FFormProfile::Rectangle(ImageMin, ImageMax));
				Quarries.DepthMeters = Leaded;
				Out.Add(Glass(Drawn(Name + TEXT(".glass"), TEXT("band.glass"), MoveTemp(Quarries), Overhead, AcrossY(Plane))));

				const FVector2D HalfCame(0.5 * Came, 0.5 * Came);
				FFormExtrusion Leading;
				Leading.Outline = FFormProfile::Rectangle(ImageMin - HalfCame, ImageMax + HalfCame);
				Leading.Holes.Add(FFormProfile::Rectangle(ImageMin + HalfCame, ImageMax - HalfCame));
				Leading.DepthMeters = Came;
				Out.Add(Drawn(Name + TEXT(".came"), TEXT("came"), MoveTemp(Leading), Overhead, AcrossY(Plane + 0.5 * (Came - Leaded))));

				const double Guard = Plane + Plan::GuardOffset + Plan::LaminatedMeters;
				FFormExtrusion Pane;
				Pane.Outline = FFormProfile::Rectangle(FVector2D(West, Foot), FVector2D(East, Head));
				Pane.DepthMeters = Plan::LaminatedMeters;
				Out.Add(Glass(Drawn(Name + TEXT(".guard"), TEXT("guard-glazing"), MoveTemp(Pane), Overhead, AcrossY(Guard))));
			}
		}
	}

	void DrawScreens(TArray<FFormDrawing>& Out)
	{
		NaveScreens(Out);
		Gables(Out);
		TristramBand(Out);
	}
}
