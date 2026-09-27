#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The gallery's barrel (§3.3): segmental, R 3.8284 about the line (x 0, z 1.3216), springing from the ledges at x ±3.00
 * and crowning at 5.15 under the roof slab. Its face is lime plaster run on the shell: plain at the springings, at the
 * ends and over the column pairs, and in each bay a field of 0.60 coffers, three steps deep with a rosette on each back,
 * round the bay's lantern. In the lantern the laylight glass hangs flat at 4.94 in its cheeks under a plenum that runs up
 * to the roof slab. Over the plaster the vault's masonry and the roof slab are one travertine body, drawn slice by slice
 * along the gallery, each slice's underside following the plaster's back.
 *
 * Everything here is drawn in the barrel's section (x east, z up), pushed north along the gallery.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		const FVector2D Axis(0.0, Plan::VaultAxisZ);
		constexpr double R = Plan::VaultRadius;
		constexpr double Plastered = Plan::VaultRadius + Plan::VaultPlaster;

		double CofferDepth()
		{
			return Plan::CofferSteps * Plan::CofferRiser;
		}

		/** The back of the coffered plaster, behind the coffers' backs. */
		double CofferedBack()
		{
			return R + CofferDepth() + Plan::VaultPlaster;
		}

		/** The lantern's inner edge on the face, s ±1.14, and its plan x, ±1.1232: the glass's edge and the plenum's walls. */
		double InnerS()
		{
			return Plan::LanternHalf - Plan::LanternFrame;
		}

		double InnerX()
		{
			return Barrel(InnerS(), R).X;
		}

		/** The angle about the axis of the point at a plan x on a radius. */
		double DegAtX(double X, double Radius)
		{
			return FMath::RadiansToDegrees(FMath::Acos(X / Radius));
		}

		/** What lies under the masonry across a stretch of the section: plain plaster, a coffered field, or the lantern's plenum. */
		enum class EZone : uint8
		{
			Plain,
			Coffered,
			Plenum,
		};

		struct FZone
		{
			double From;
			double To;
			EZone Kind;
		};

		/** The zones across the barrel, west to east, of a slice: plain, a full coffered field, or a lantern's (open or at its frame's ends). */
		TArray<FZone> Across(bool bCoffered, bool bLantern, bool bOpen)
		{
			const double S = Plan::VaultSpringingS;
			const double F = Plan::CofferFieldS;
			const double L = Plan::LanternHalf;
			if (!bCoffered)
			{
				return {{-S, S, EZone::Plain}};
			}
			if (!bLantern)
			{
				return {{-S, -F, EZone::Plain}, {-F, F, EZone::Coffered}, {F, S, EZone::Plain}};
			}
			if (!bOpen)
			{
				return {{-S, -F, EZone::Plain}, {-F, -L, EZone::Coffered}, {-L, L, EZone::Plain}, {L, F, EZone::Coffered}, {F, S, EZone::Plain}};
			}
			return {{-S, -F, EZone::Plain}, {-F, -L, EZone::Coffered}, {-L, -InnerS(), EZone::Plain}, {-InnerS(), InnerS(), EZone::Plenum},
				{InnerS(), L, EZone::Plain}, {L, F, EZone::Coffered}, {F, S, EZone::Plain}};
		}

		/**
		 * The masonry over one slice, from the ledges at 3.70 and the plaster's back to the roof's top: its underside steps
		 * out behind each coffered field and, over a lantern, rises in the plenum's walls to the roof slab's soffit.
		 */
		FFormProfile Masonry(const TArray<FZone>& Zones)
		{
			const FVector2D Springing = Barrel(-Plan::VaultSpringingS, Plastered);
			FPen Pen(-Plan::ShellHalfWidth, Plan::Ledge);
			Pen.To(Springing.X, Plan::Ledge).To(Springing.X, Springing.Y);
			for (const FZone& Zone : Zones)
			{
				if (Zone.Kind == EZone::Plenum)
				{
					// Up the plenum's west wall to the roof slab's soffit, across, and down its east wall to the frame's back.
					const FVector2D East = Axis + Polar(Plastered, DegAtX(InnerX(), Plastered));
					Pen.To(-InnerX(), Plan::RoofSoffit).To(InnerX(), Plan::RoofSoffit).To(East.X, East.Y);
					continue;
				}
				// Each arc starts from the last by a radial step where the back changes; at the plenum it meets the wall's plane.
				const double Radius = Zone.Kind == EZone::Coffered ? CofferedBack() : Plastered;
				const double FromDeg = Zone.From == InnerS() ? DegAtX(InnerX(), Radius) : BarrelDeg(Zone.From);
				const double ToDeg = Zone.To == -InnerS() ? DegAtX(-InnerX(), Radius) : BarrelDeg(Zone.To);
				Pen.Arc(Axis, Radius, FromDeg, ToDeg);
			}
			Pen.To(-Springing.X, Plan::Ledge).To(Plan::ShellHalfWidth, Plan::Ledge).To(Plan::ShellHalfWidth, Plan::RoofTop).To(-Plan::ShellHalfWidth, Plan::RoofTop);
			return Pen.Closed();
		}

		/** The run plaster over a stretch of the face, from one s to another. */
		FFormProfile PlasterBand(double From, double To)
		{
			FPen Pen(Barrel(From, R));
			Pen.Arc(Axis, R, BarrelDeg(From), BarrelDeg(To)).Arc(Axis, Plastered, BarrelDeg(To), BarrelDeg(From));
			return Pen.Closed();
		}

		/** A slice of the masonry over the vault between two y (south, north). */
		void Slice(TArray<FFormDrawing>& Out, const FString& Name, double South, double North, const TArray<FZone>& Zones)
		{
			Out.Add(Drawn(TEXT("shell.gallery.vault.") + Name, TEXT("shell"), Prism(Masonry(Zones), South - North), Solid, BarrelFrame(South)));
		}

		/** The height of a coffered field over its back, and a rosette on each back (§3.3's coffer and rosette). */
		double CofferHeight(double S, double Y, double CellOriginS, double CellOriginY)
		{
			const double Cell = Plan::CofferCell;
			const double A = FMath::Fmod(S - CellOriginS + 64.0 * Cell, Cell);
			const double B = FMath::Fmod(Y - CellOriginY + 64.0 * Cell, Cell);
			const double Edge = FMath::Min(FMath::Min(A, Cell - A), FMath::Min(B, Cell - B));
			const double Deep = CofferDepth();
			const double InBack = Edge - Plan::CofferRib - (Plan::CofferSteps - 1) * Plan::CofferSetback;
			if (InBack < 0.0)
			{
				const int32 Step = Edge < Plan::CofferRib ? 0 : 1 + FMath::FloorToInt32((Edge - Plan::CofferRib) / Plan::CofferSetback);
				return Deep - Step * Plan::CofferRiser;
			}
			const double BackSide = Cell - 2.0 * (Plan::CofferRib + (Plan::CofferSteps - 1) * Plan::CofferSetback);
			const FVector2D At(A - 0.5 * Cell, B - 0.5 * Cell);
			return RosetteHeight(At.Size(), FMath::Atan2(At.Y, At.X), Plan::RosetteOfBack * BackSide);
		}

		/**
		 * A coffered field between two s and two y (south, north), carved as a relief on the barrel: its ground the
		 * coffers' backs, 0.18 behind the face, the ribs and steps standing proud of it to the face, and the plaster's
		 * back behind it. The cells are laid from the field of the bay: s from −3.00, y from the bay's south edge.
		 */
		void Field(TArray<FFormDrawing>& Out, const FString& Name, double SWest, double SEast, double South, double North, double BaySouth)
		{
			FFormRelief Relief;
			Relief.Path = FFormProfile::From(Barrel(SEast, R));
			Relief.Path.ArcAbout(Axis, R, R, BarrelDeg(SEast), BarrelDeg(SWest), EFormJoint::Smooth);
			Relief.OffsetMeters = -CofferDepth();
			Relief.BottomMeters = 0.0;
			Relief.TopMeters = South - North;
			Relief.GridMeters = Plan::CofferGrid;
			Relief.BackMeters = Plan::VaultPlaster;
			const double Origin = -Plan::CofferFieldS;
			Relief.HeightMeters = [SEast, South, BaySouth, Origin](double U, double V)
			{
				return CofferHeight(SEast - U, South - V, Origin, BaySouth);
			};
			Out.Add(Drawn(TEXT("gallery.vault.") + Name, TEXT("vault.gallery"), MoveTemp(Relief), nullptr, BarrelFrame(South)));
		}

		/** A plain stretch of plaster between two s and two y. */
		void Plain(TArray<FFormDrawing>& Out, const FString& Name, double From, double To, double South, double North)
		{
			Out.Add(Drawn(TEXT("gallery.vault.") + Name, TEXT("vault.gallery"), Prism(PlasterBand(From, To), South - North), nullptr, BarrelFrame(South)));
		}

		/**
		 * The lantern's frame, a flat band 0.06 on the face inside the opening's edge, cut square at the opening (the
		 * plenum's walls, x ±1.1232); the glass hanging in it at 4.94 and the cheeks that close it up to the frame.
		 */
		void Lantern(TArray<FFormDrawing>& Out, const FString& Bay, double Centre)
		{
			const double L = Plan::LanternHalf;
			const double In = InnerS();
			const double X = InnerX();
			for (const double Side : {-1.0, 1.0})
			{
				FPen Band(Barrel(Side * In, R));
				Band.Arc(Axis, R, BarrelDeg(Side * In), BarrelDeg(Side * L)).Arc(Axis, Plastered, BarrelDeg(Side * L), DegAtX(Side * X, Plastered));
				Out.Add(Drawn(FString::Printf(TEXT("gallery.vault.%s.frame.%s"), *Bay, Side > 0.0 ? TEXT("east") : TEXT("west")), TEXT("vault.gallery"),
					Prism(Band.Closed(), 2.0 * L), nullptr, BarrelFrame(Centre + L)));
			}
			FPen End(Barrel(-In, R));
			End.Arc(Axis, R, BarrelDeg(-In), BarrelDeg(In)).Arc(Axis, Plastered, DegAtX(X, Plastered), DegAtX(-X, Plastered));
			for (const double Side : {1.0, -1.0})
			{
				const double South = Side > 0.0 ? Centre + L : Centre - In;
				Out.Add(Drawn(FString::Printf(TEXT("gallery.vault.%s.frame.%s"), *Bay, Side > 0.0 ? TEXT("south") : TEXT("north")), TEXT("vault.gallery"),
					Prism(End.Closed(), Plan::LanternFrame), nullptr, BarrelFrame(South)));
			}

			const double Hung = R + Plan::Sink;
			const double Cheek = X + Plan::LaylightCheek;
			for (const double Side : {-1.0, 1.0})
			{
				FPen Long(Side * X, Plan::LaylightUnderside);
				Long.To(Side * Cheek, Plan::LaylightUnderside).To(Side * Cheek, Axis.Y + FMath::Sqrt(Hung * Hung - Cheek * Cheek));
				Long.Arc(Axis, Hung, DegAtX(Side * Cheek, Hung), DegAtX(Side * X, Hung));
				Out.Add(Drawn(FString::Printf(TEXT("gallery.laylight.%s.cheek.%s"), *Bay, Side > 0.0 ? TEXT("east") : TEXT("west")), TEXT("laylight.well"),
					Prism(Long.Closed(), 2.0 * In), nullptr, BarrelFrame(Centre + In)));
			}
			FPen Short(-Cheek, Plan::LaylightUnderside);
			Short.To(Cheek, Plan::LaylightUnderside).To(Cheek, Axis.Y + FMath::Sqrt(Hung * Hung - Cheek * Cheek)).Arc(Axis, Hung, DegAtX(Cheek, Hung),
				DegAtX(-Cheek, Hung));
			for (const double Side : {1.0, -1.0})
			{
				const double South = Side > 0.0 ? Centre + In + Plan::LaylightCheek : Centre - In;
				Out.Add(Drawn(FString::Printf(TEXT("gallery.laylight.%s.cheek.%s"), *Bay, Side > 0.0 ? TEXT("south") : TEXT("north")), TEXT("laylight.well"),
					Prism(Short.Closed(), Plan::LaylightCheek), nullptr, BarrelFrame(South)));
			}
			FFormDrawing Glass = PlanPrism(FString::Printf(TEXT("gallery.laylight.%s.glass"), *Bay), TEXT("laylight.glass"),
				Rect(-X, Centre - In, X, Centre + In), {}, Plan::LaylightUnderside, Plan::LaylightUnderside + Plan::LaylightGlass, nullptr);
			Glass.bNanite = false;
			Out.Add(MoveTemp(Glass));
		}

		/**
		 * One bay: its coffered fields (the two ends across the whole field, the two sides of the lantern), its lantern, and
		 * the masonry over it in five slices (the ends, the lantern's frame ends, the lantern).
		 */
		void Bay(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double C = Plan::BayCentres[Index];
			const double L = Plan::LanternHalf;
			const double H = Plan::BayCofferHalf;
			const double In = InnerS();
			const double F = Plan::CofferFieldS;
			const double BaySouth = C + H;
			const FString Bay = FString::Printf(TEXT("bay%d"), Index + 1);
			Field(Out, Bay + TEXT(".field.south"), -F, F, C + H, C + L, BaySouth);
			Field(Out, Bay + TEXT(".field.north"), -F, F, C - L, C - H, BaySouth);
			Field(Out, Bay + TEXT(".field.west"), -F, -L, C + L, C - L, BaySouth);
			Field(Out, Bay + TEXT(".field.east"), L, F, C + L, C - L, BaySouth);
			Lantern(Out, Bay, C);
			Slice(Out, Bay + TEXT(".south"), C + H, C + L, Across(true, false, false));
			Slice(Out, Bay + TEXT(".south-frame"), C + L, C + In, Across(true, true, false));
			Slice(Out, Bay + TEXT(".lantern"), C + In, C - In, Across(true, true, true));
			Slice(Out, Bay + TEXT(".north-frame"), C - In, C - L, Across(true, true, false));
			Slice(Out, Bay + TEXT(".north"), C - L, C - H, Across(true, false, false));
		}
	}

	void DrawGalleryVault(TArray<FFormDrawing>& Out)
	{
		const double S = Plan::VaultSpringingS;
		const double F = Plan::CofferFieldS;
		Plain(Out, TEXT("springing.west"), -S, -F, Plan::SouthFaceY, Plan::ScreenY);
		Plain(Out, TEXT("springing.east"), F, S, Plan::SouthFaceY, Plan::ScreenY);

		// The plain stretches along the gallery: its south end, the three pair bands, its north end.
		TArray<TPair<double, double>> Plains = {{Plan::SouthFaceY, Plan::PlainSouthY}};
		for (int32 Index = 0; Index + 1 < Plan::Bays; ++Index)
		{
			Plains.Add({Plan::BayCentres[Index] - Plan::BayCofferHalf, Plan::BayCentres[Index + 1] + Plan::BayCofferHalf});
		}
		Plains.Add({Plan::PlainNorthY, Plan::ScreenY});
		for (int32 Index = 0; Index < Plains.Num(); ++Index)
		{
			const double South = Plains[Index].Key;
			const double North = Plains[Index].Value;
			Plain(Out, FString::Printf(TEXT("plain.%d"), Index + 1), -F, F, South, North);
			// The south end's masonry reaches over the south wall's finish to its core.
			const double Over = Index == 0 ? South + Plan::LaidFinish : South;
			Slice(Out, FString::Printf(TEXT("plain.%d"), Index + 1), Over, North, Across(false, false, false));
		}
		for (int32 Index = 0; Index < Plan::Bays; ++Index)
		{
			Bay(Out, Index);
		}
	}
}
