#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The thirteen niches (§3.1), each cut in its own flat bay of wall: the gallery's eight in the long walls, the tribune's
 * five with their mouths on the plane tangent to the round wall (the wall's hook returns to that plane beside the
 * bay: the sliver reveal). A bay is the wall's finish on its face, the dado below the sill and the field round the
 * arch; its core, below the sill, round the half-round and over the head; the niche's lining of burnished red-ochre
 * lime, the half-round below the springing and the quarter-sphere over it; its floor slab at the sill; and on its face
 * the surround and the keystone.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		/** A band's half-width in a bay's core: the tribune's cores reach into the round wall's notch past the finish's hook. */
		double CoreHalf(const FNiche& Niche)
		{
			return Niche.BayHalf() + (Niche.bTribune ? Plan::LaidFinish : 0.0);
		}

		/** The half-width at the finish's back of a circle of a radius about the niche's axis. */
		double AtBack(double Radius)
		{
			return FMath::Sqrt(Radius * Radius - Plan::LaidFinish * Plan::LaidFinish);
		}

		/** The angle, in the niche's plan (x into the wall, y lateral), of a circle's point at the finish's back. */
		double BackDeg(double Radius)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(AtBack(Radius), Plan::LaidFinish));
		}

		/** The finish on the bay's face: the dado to the sill, the field from the sill round the arch to the ledge. */
		void Finish(TArray<FFormDrawing>& Out, const FNiche& Niche, const FString& Name)
		{
			const double Half = Niche.BayHalf();
			const FMuseeFrame Face = Niche.WallFrame(0.0);
			Out.Add(Drawn(Name + TEXT(".dado"), TEXT("wall.dado"), Prism(Rect(-Half, -Plan::FloorStone, Half, Plan::DadoTop), Plan::LaidFinish), Solid,
				Face));
			FPen Field(-Half, Plan::DadoTop);
			Field.To(-Niche.Radius, Plan::DadoTop).To(-Niche.Radius, Niche.Springing).Arc(FVector2D(0.0, Niche.Springing), Niche.Radius, 180.0, 0.0);
			Field.To(Niche.Radius, Plan::DadoTop).To(Half, Plan::DadoTop).To(Half, Plan::Ledge).To(-Half, Plan::Ledge);
			Out.Add(Drawn(Name + TEXT(".field"), TEXT("wall.field"), Prism(Field.Closed(), Plan::LaidFinish), Solid, Face));
		}

		/**
		 * The bay's core behind the finish: under the sill's slab; from the slab to the springing, round the half-round's
		 * lining; and from the springing to the ledge, round the head's half-cylinder, which the hood fills.
		 */
		void Core(TArray<FFormDrawing>& Out, const FNiche& Niche, const FString& Name)
		{
			const double Half = CoreHalf(Niche);
			const double Back = Plan::NicheBayDepth;
			const double Lined = Niche.Radius + Plan::NicheLining;
			const double SillBed = Plan::DadoTop - Plan::NicheSillSlab;
			const FMuseeFrame Ground = Niche.PlanFrame();
			Out.Add(PrismIn(TEXT("shell.") + Name + TEXT(".foot"), TEXT("shell"), Ground, Rect(Plan::LaidFinish, -Half, Back, Half), -Plan::FloorStone, SillBed,
				Solid));
			FPen Body(Plan::LaidFinish, -Half);
			Body.To(Plan::LaidFinish, -AtBack(Lined)).Arc(FVector2D::ZeroVector, Lined, -BackDeg(Lined), BackDeg(Lined));
			Body.To(Plan::LaidFinish, Half).To(Back, Half).To(Back, -Half);
			Out.Add(PrismIn(TEXT("shell.") + Name + TEXT(".body"), TEXT("shell"), Ground, Body.Closed(), SillBed, Niche.Springing, Solid));
			FPen Head(-Half, Niche.Springing);
			Head.To(-Lined, Niche.Springing).Arc(FVector2D(0.0, Niche.Springing), Lined, 180.0, 0.0).To(Half, Niche.Springing);
			Head.To(Half, Plan::Ledge).To(-Half, Plan::Ledge);
			Out.Add(Drawn(TEXT("shell.") + Name + TEXT(".head"), TEXT("shell"), Prism(Head.Closed(), Back - Plan::LaidFinish), Solid,
				Niche.WallFrame(Plan::LaidFinish)));
		}

		/** The niche's own stone and plaster: the sill's slab, the lining of the half-round, the hood of the head and the core behind it. */
		void Hollow(TArray<FFormDrawing>& Out, const FNiche& Niche, const FString& Name)
		{
			const double Lined = Niche.Radius + Plan::NicheLining;
			const FMuseeFrame Ground = Niche.PlanFrame();
			FPen Sill(Plan::LaidFinish, -AtBack(Lined));
			Sill.Arc(FVector2D::ZeroVector, Lined, -BackDeg(Lined), BackDeg(Lined));
			Out.Add(PrismIn(Name + TEXT(".sill"), TEXT("niche.sill"), Ground, Sill.Closed(), Plan::DadoTop - Plan::NicheSillSlab, Plan::DadoTop, Solid));
			FPen Lining(Plan::LaidFinish, -AtBack(Lined));
			Lining.Arc(FVector2D::ZeroVector, Lined, -BackDeg(Lined), BackDeg(Lined)).To(Plan::LaidFinish, AtBack(Niche.Radius));
			Lining.Arc(FVector2D::ZeroVector, Niche.Radius, BackDeg(Niche.Radius), -BackDeg(Niche.Radius));
			Out.Add(PrismIn(Name + TEXT(".lining"), TEXT("niche.interior"), Ground, Lining.Closed(), Plan::DadoTop, Niche.Springing, Solid));

			// The head turns about the niche's horizontal axis at the springing: A out from the axis, B into the wall.
			const FMuseeFrame Axis = FrameOfAxes(Niche.Origin + FVector::UpVector * Niche.Springing, Niche.Lateral(), FVector::UpVector);
			FPen Hood(AtBack(Niche.Radius), Plan::LaidFinish);
			Hood.To(AtBack(Lined), Plan::LaidFinish).Arc(FVector2D::ZeroVector, Lined, 90.0 - BackDeg(Lined), 90.0).To(0.0, Niche.Radius);
			Hood.Arc(FVector2D::ZeroVector, Niche.Radius, 90.0, 90.0 - BackDeg(Niche.Radius));
			FFormLathe Shell;
			Shell.Section = Hood.Closed();
			Shell.FromDegrees = 0.0;
			Shell.ToDegrees = 180.0;
			Out.Add(Drawn(Name + TEXT(".hood"), TEXT("niche.interior"), MoveTemp(Shell), Solid, Axis));
			FPen Behind(AtBack(Lined), Plan::LaidFinish);
			Behind.To(Lined, Plan::LaidFinish).To(Lined, Plan::NicheBayDepth).To(0.0, Plan::NicheBayDepth).To(0.0, Lined);
			Behind.Arc(FVector2D::ZeroVector, Lined, 90.0, 90.0 - BackDeg(Lined));
			FFormLathe Masonry;
			Masonry.Section = Behind.Closed();
			Masonry.FromDegrees = 0.0;
			Masonry.ToDegrees = 180.0;
			Out.Add(Drawn(TEXT("shell.") + Name + TEXT(".hood"), TEXT("shell"), MoveTemp(Masonry), Solid, Axis));
		}

		/**
		 * On the bay's face: the surround, a 0.10 band round the jambs and the head from the dado rail's top, swept on the
		 * mouth plane (A out from the opening's edge, B out of the wall); and the keystone at the crown, from 0.04 under it
		 * into the architrave's soffit, widening 0.10 to 0.15, 0.055 proud.
		 */
		void Dressings(TArray<FFormDrawing>& Out, const FNiche& Niche, const FString& Name)
		{
			const double RailTop = Plan::DadoTop + Plan::Sink;
			const FMuseeFrame Face = FrameOfAxes(Niche.Origin, -Niche.Lateral(), FVector::UpVector);
			FFormSweep Surround;
			Surround.Section = Written(Plan::NicheSurround);
			Surround.Path = FFormProfile::From(FVector2D(-Niche.Radius, RailTop));
			Surround.Path.LineTo(FVector2D(-Niche.Radius, Niche.Springing), EFormJoint::Crisp)
				.ArcAbout(FVector2D(0.0, Niche.Springing), Niche.Radius, Niche.Radius, 180.0, 0.0, EFormJoint::Smooth)
				.LineTo(FVector2D(Niche.Radius, RailTop), EFormJoint::Smooth);
			Out.Add(Drawn(Name + TEXT(".surround"), TEXT("moulding"), MoveTemp(Surround), nullptr, Face));

			const double Crown = Niche.Springing + Niche.Radius;
			const FVector2D Key[] = {{-0.5 * Plan::KeystoneFootWidth, Crown - Plan::KeystoneUnder}, {0.5 * Plan::KeystoneFootWidth, Crown - Plan::KeystoneUnder},
				{0.5 * Plan::KeystoneTopWidth, Plan::KeystoneTop}, {-0.5 * Plan::KeystoneTopWidth, Plan::KeystoneTop}};
			const FMuseeFrame Sunk(Niche.Origin + Niche.In * Plan::Sink, Face.Rotation);
			Out.Add(Drawn(Name + TEXT(".keystone"), TEXT("moulding"), Prism(FFormProfile::Polygon(Key), Plan::KeystoneProud + Plan::Sink), nullptr, Sunk));
		}
	}

	void DrawNiches(TArray<FFormDrawing>& Out)
	{
		for (const FNiche& Niche : Niches())
		{
			const FString Name = Niche.AnchorLocal();
			Finish(Out, Niche, Name);
			Core(Out, Niche, Name);
			Hollow(Out, Niche, Name);
			Dressings(Out, Niche, Name);
		}
	}
}
