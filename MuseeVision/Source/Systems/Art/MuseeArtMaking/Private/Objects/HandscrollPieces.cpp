#include "ArtPieces.h"
#include "FormExtrusion.h"
#include "FormMake.h"
#include "Objects/ObjectMaking.h"
#include "Objects/ScrollDrawing.h"
#include "Plan/EntryContext.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The surfaces a handscroll's wooden fittings are named by; their materials are the fittings record's. */
		const FName StaveSurface(TEXT("handscroll.stave"));
		const FName RollerSurface(TEXT("handscroll.roller"));

		/** A drawing in a finish a record names, overriding the finish schedule. */
		FFormDrawing InFinish(FFormDrawing&& Drawing, const FArtFinishSpec& Finish)
		{
			Drawing.Material = Finish.Palette;
			Drawing.Working = Finish.Working;
			Drawing.History = Finish.History;
			return MoveTemp(Drawing);
		}

		/** The stave's section, in its drawing's (along, out): the half disc below its flat, which runs through the axis. */
		FFormProfile StaveSection(double Radius)
		{
			return FFormProfile::From(FVector2D(-Radius, 0.0)).ArcAbout(FVector2D::ZeroVector, Radius, Radius, 180.0, 360.0).Close();
		}

		/**
		 * The scroll as it lies on its bed, in the bed's (along, up) and out of its face: the mount's foot and height, the
		 * silk's back and face, and where its flat run meets its two rolls (the lines on which each roll rests on the silk).
		 */
		struct FHandscrollLay
		{
			double Foot = 0.0;
			double Height = 0.0;
			double Back = 0.0;
			double Face = 0.0;
			double Left = 0.0;
			double Right = 0.0;

			FHandscrollLay(const FArtHangEntry& Entry, double SilkMeters)
			{
				const FArtScrollMount& Mount = Entry.Mount;
				Foot = Entry.Height - Mount.Border;
				Height = Entry.SizeMeters().Y + 2.0 * Mount.Border;
				Back = Entry.OffWall;
				Face = Back + SilkMeters;
				Left = Entry.Along - Mount.Band;
				Right = Entry.Along + Entry.SizeMeters().X + Mount.Band + Mount.Lead + Mount.Wrapper;
			}
		};

		/**
		 * A roll lying across the scroll at a line where its flat run ends: a tube of the wound silk round its core, resting
		 * on the silk's face, from the mount's foot to its head.
		 */
		FFormDrawing Roll(FName Name, FName Surface, const FScrollWall& Wall, const FHandscrollLay& Lay, double At, double Radius, FFormProfile&& Core)
		{
			FFormExtrusion Tube;
			Tube.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, false);
			Tube.Holes.Add(MoveTemp(Core));
			Tube.DepthMeters = Lay.Height;
			// The drawing's (x, y) is the bed's (along, out), so its z is the bed's up.
			return ArtDrawing(Name, Surface, MoveTemp(Tube), FrameAlong(Wall.At(At, Lay.Face + Radius, Lay.Foot), Wall.Along, Wall.Out));
		}

		/**
		 * The length of the wrapper wound on the stave: as long as its turns take round the scroll rolled whole (its unshown
		 * length on the roller, the flat run over it and what of its front lies wound at the stave, each turn the silk's
		 * thickness), less what lies unrolled.
		 */
		double WoundWrapper(const FArtHangEntry& Entry, const FHandscrollLay& Lay, const FArtHandscrollFittings& Fit, double FrontRolled, double SilkMeters)
		{
			const double Rolled = FMath::Sqrt(FMath::Square(Entry.Mount.Roll_Radius) + SilkMeters * (Lay.Right - Lay.Left + FrontRolled) / UE_DOUBLE_PI);
			return Fit.WrapperTurns * UE_DOUBLE_TWO_PI * Rolled - Entry.Mount.Wrapper;
		}

		/** The roll's radius when Length of silk SilkMeters thick is wound on a core of CoreRadius: its section's area grows by each turn's. */
		double WoundRadius(double CoreRadius, double Length, double SilkMeters)
		{
			return FMath::Sqrt(FMath::Square(CoreRadius) + SilkMeters * Length / UE_DOUBLE_PI);
		}

		/** The painting or writing: a panel of the sheet for each photograph laid across it, each in its own layer. */
		void MakeSheet(const FCanvasPlan& Canvas, const FScrollWall& Wall, const FHandscrollLay& Lay, double SilkMeters, TArray<FFormMadePiece>& OutPieces)
		{
			const FArtHangEntry& Entry = *Canvas.Entry;
			const FName Surface(ObjectWord(Entry.Support));
			for (const FScrollSection& Section : Canvas.Sections)
			{
				const FString Name = Canvas.Sections.Num() > 1 ? FString::Printf(TEXT("%s.section.%d"), *ObjectName(Entry, false), Section.Number)
					: ObjectName(Entry, false);
				const FMuseeFrame Corner(Wall.At(Entry.Along + Section.Panel.AlongMeters, Lay.Back, Entry.Height), Wall.Wall.Rotation);
				const FVector2D Size(Section.Panel.LengthMeters, Entry.SizeMeters().Y);
				OutPieces.Add(SheetPiece(FName(Name), Surface, { Section.Layer, Canvas.Finish.Working, Canvas.Finish.History }, Corner, Size, SilkMeters,
					SilkMeters, false, Section.Panel.ImageLeftMeters, Section.Panel.ImageTopMeters));
			}
		}
	}

	void MakeHandscroll(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtScrollFittings, FString>& Silk = Making.Scrolls();
		const TValueOrError<FArtHandscrollFittings, FString>& Fittings = Making.Handscrolls();
		if (Silk.HasError() || Fittings.HasError() || !OnStraightWall(Making, Entry, OutFindings))
		{
			if (Silk.HasError() || Fittings.HasError())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its handscroll can't be mounted: %s."), *Entry.Work,
					Silk.HasError() ? *Silk.GetError() : *Fittings.GetError())));
			}
			return;
		}
		const double T = Silk.GetValue().MountMeters;
		const FArtHandscrollFittings& Fit = Fittings.GetValue();
		const FScrollWall Wall(Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame);
		const FHandscrollLay Lay(Entry, T);
		// The separator and the frontispiece beyond the work: what of them the stage doesn't lay open past the work's band
		// and lead lies wound on the stave, inside the wrapper, which is unrolled before them.
		const double FrontRolled = FMath::Max(0.0, Canvas.FrontMeters - Entry.Mount.Band - Entry.Mount.Lead);
		if (FrontRolled > 0.0 && Entry.Mount.Wrapper > 0.0)
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its wrapper lies %.3f m unrolled, but %.3f m of its separator and "
				"frontispiece (Data/%s) lie wound at the stave, which a scroll unrolls before its wrapper: lay them open, a lead of %.3f m or more."),
				*Entry.Work, Entry.Mount.Wrapper, FrontRolled, FArtHandscroll::Record, Canvas.FrontMeters - Entry.Mount.Band)));
			return;
		}
		const double Wound = WoundWrapper(Entry, Lay, Fit, FrontRolled, T);
		if (Wound <= 0.0)
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its wrapper lies %.3f m unrolled, more than the %.3f m its %.2f turns "
				"round the rolled scroll make (Data/%s)."), *Entry.Work, Entry.Mount.Wrapper, Wound + Entry.Mount.Wrapper, Fit.WrapperTurns,
				FArtHandscrollFittings::Record)));
			return;
		}
		const FArtScrollMount& Mount = Entry.Mount;
		const FVector2D Painting = Entry.SizeMeters();
		TArray<FFormDrawing> Drawings;
		// The mount's silk from the left roll to the right: the band, the painting's window, the band, the lead, the wrapper.
		Drawings.Add(MountSheet(PartName(Entry, false, TEXT("mount")), Entry.Frame, Wall, FVector2D(Lay.Left, Lay.Foot), FVector2D(Lay.Right - Lay.Left,
			Lay.Height), FVector2D(Mount.Band, Mount.Border), Painting, Lay.Face, T));
		// At the left, the unshown length wound on the roller, and the knobs on the roller's ends past the scroll's foot and head.
		const FVector Axis = Wall.At(Lay.Left, Lay.Face + Mount.Roll_Radius, Lay.Foot);
		Drawings.Add(Roll(PartName(Entry, false, TEXT("roll")), Entry.Frame, Wall, Lay, Lay.Left, Mount.Roll_Radius,
			FFormProfile::Ellipse(FVector2D::ZeroVector, Fit.RollerRadiusMeters, Fit.RollerRadiusMeters, 0.0, false)));
		Drawings.Add(InFinish(Rod(PartName(Entry, false, TEXT("roller")), RollerSurface, Axis, Wall.Up, Wall.Along, Lay.Height, Fit.RollerRadiusMeters),
			Fit.Roller));
		Drawings.Add(Rod(PartName(Entry, false, TEXT("knob.foot")), Mount.Knob, Axis - Wall.Up * Fit.KnobLengthMeters, Wall.Up, Wall.Along,
			Fit.KnobLengthMeters, Fit.KnobRadiusMeters));
		Drawings.Add(Rod(PartName(Entry, false, TEXT("knob.head")), Mount.Knob, Axis + Wall.Up * Lay.Height, Wall.Up, Wall.Along, Fit.KnobLengthMeters,
			Fit.KnobRadiusMeters));
		// At the right, the stave with the rest of its wrapper wound round it, and over that the frontispiece's unshown rest.
		const double Wrapped = WoundRadius(Fit.StaveRadiusMeters, Wound + FrontRolled, T);
		Drawings.Add(Roll(PartName(Entry, false, TEXT("wrapper")), Entry.Frame, Wall, Lay, Lay.Right, Wrapped, StaveSection(Fit.StaveRadiusMeters)));
		FFormExtrusion Stave;
		Stave.Outline = StaveSection(Fit.StaveRadiusMeters);
		Stave.DepthMeters = Lay.Height;
		Drawings.Add(InFinish(ArtDrawing(PartName(Entry, false, TEXT("stave")), StaveSurface, MoveTemp(Stave), FrameAlong(Wall.At(Lay.Right,
			Lay.Face + Wrapped, Lay.Foot), Wall.Along, Wall.Out)), Fit.Stave));
		Musee::FormMaking::Make(Drawings, OutPieces, OutFindings);
		MakeSheet(Canvas, Wall, Lay, T, OutPieces);
	}
}
