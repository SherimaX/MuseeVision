#pragma once

#include "ArtHang.h"
#include "ArtHangReading.h"
#include "Cast/CastRecord.h"
#include "CoreMinimal.h"
#include "Vessel/VesselRecord.h"
#include "FormMadePiece.h"
#include "Frame/Moulding.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"
#include "Objects/HandscrollRecords.h"
#include "Objects/HangingRecords.h"
#include "Stone/StoneRecord.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	/**
	 * What every object of a stage is made with, read once: its stage and plan, each frame style's moulding, ware's
	 * forms, how works are cast, how scholar's rocks are formed, the scrolls' and handscrolls' fittings, the panels'
	 * construction, the books' bindings and the hooks works hang on a mesh by.
	 */
	struct FObjectMaking
	{
		const FMuseeStageContext& Stage;
		const FArtHang& Hang;
		TMap<FString, TValueOrError<FArtMoulding, FString>> Mouldings;
		TOptional<TValueOrError<FArtWareForms, FString>> WareForms;
		TOptional<TValueOrError<FArtCasts, FString>> CastRecord;
		TOptional<TValueOrError<FArtVessels, FString>> VesselRecord;
		TOptional<TValueOrError<FArtStones, FString>> StoneRecord;
		TOptional<TValueOrError<FArtScrollFittings, FString>> ScrollFittings;
		TOptional<TValueOrError<FArtHandscrollFittings, FString>> HandscrollFittings;
		TOptional<TValueOrError<FArtPanelConstruction, FString>> PanelConstruction;
		TOptional<TValueOrError<TArray<FArtBook>, FString>> Bindings;
		TOptional<TValueOrError<FArtHooks, FString>> HookRecord;

		/** The moulding a frame of this style is made in (FArtMoulding::RecordFor), or why it can't be read. */
		const TValueOrError<FArtMoulding, FString>& Moulding(FName FrameStyle);

		/** How ware is turned (Data/Art/Ware.json), or why it can't be read. */
		const TValueOrError<FArtWareForms, FString>& Forms();

		/** How works in the round are cast (Data/Art/Casts.json), or why it can't be read. */
		const TValueOrError<FArtCasts, FString>& Casts();

		/** How vessels are turned and stood (Data/Art/Vessels.json), or why it can't be read. */
		const TValueOrError<FArtVessels, FString>& Vessels();

		/** How scholar's rocks are formed, and each rock (Data/Art/Stones.json), or why it can't be read. */
		const TValueOrError<FArtStones, FString>& Stones();

		/** A hanging scroll's fittings (Data/Art/Scrolls.json), or why they can't be read. */
		const TValueOrError<FArtScrollFittings, FString>& Scrolls();

		/** A handscroll's fittings (Data/Art/Handscroll.json), or why they can't be read. */
		const TValueOrError<FArtHandscrollFittings, FString>& Handscrolls();

		/** How a panel is made (Data/Art/Panel.json), or why it can't be read. */
		const TValueOrError<FArtPanelConstruction, FString>& Panels();

		/** How each book is bound (Data/Art/Books.json), or why they can't be read. */
		const TValueOrError<TArray<FArtBook>, FString>& Books();

		/** The hardware a framed work hangs on a mesh by (Data/Art/Hooks.json), or why it can't be read. */
		const TValueOrError<FArtHooks, FString>& Hooks();
	};

	/** A piece Art makes: named, finished in its surface or in a material spec, colliding as a solid, drawn by Art. */
	[[nodiscard]] FFormMadePiece ArtPiece(FName Name, FName Surface, const FLayerFinish& Material, const FMuseeFrame& Frame, bool bNanite);

	/** A frame of axes in the stage: its origin, and its x and y (its z is x cross y, as the stage's axes are). */
	[[nodiscard]] FMuseeFrame FrameAlong(const FVector& Origin, const FVector& X, const FVector& Y);

	/**
	 * A sheet as a piece, in its own frame (x to the viewer's right, y out of its face, z up it), its face at Front, its
	 * left edge ImageLeftMeters into the image laid over it and its top edge ImageTopMeters below the image's; glass is
	 * translucent and so not Nanite.
	 */
	[[nodiscard]] FFormMadePiece SheetPiece(FName Name, FName Surface, const FLayerFinish& Material, const FMuseeFrame& Frame, const FVector2D& Size,
		double Thickness, double Front, bool bGlass, double ImageLeftMeters = 0.0, double ImageTopMeters = 0.0);

	/** Whether a work that hangs flat on its wall can be: on a straight wall; a finding when its wall is curved. */
	[[nodiscard]] bool OnStraightWall(const FObjectMaking& Making, const FArtHangEntry& Entry, TArray<FMuseeFinding>& OutFindings);

	/**
	 * The own frame of a work standing in the round on its Mount anchor: at its foot, its `centre`; y its front (the plan's
	 * facing, a bearing from the stage's +x towards +y, levelled on the bed); z the bed's up; x to the viewer's right as the
	 * viewer faces its front.
	 */
	[[nodiscard]] FMuseeFrame RoundFrame(const FArtHangEntry& Entry, const FMuseeFrame& Bed);

	/**
	 * A print on its mount on a Wall anchor: the mount, a board of its size and depth, its face on the anchor's plane; the
	 * print on it, at its place in the plan, a sheet of paper in the work's own layer; the glazing over the mount, lying on
	 * the print; and the frame round the mount, its lip on the glazing and its back flush with the mount's.
	 */
	void MakePrint(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A transparency (an autochrome) on a Wall anchor: its window mat, its face on the anchor's plane with the plate's
	 * window cut through it; the plate behind the window, a pane of its glass carrying the work's own layer; the diffusing
	 * pane behind the plate, through which the daylight behind lights it; the glazing on the mat; and the frame round the
	 * mat, its back flush with the backing's.
	 */
	void MakeTransparency(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/** A card or a daguerreotype's plate lying on its reader (a Mount anchor), centred on it, under its glazing. */
	void MakeCard(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces);

	/**
	 * A piece of turned ware standing at its centre on its Mount anchor, its front towards its facing: its decorated
	 * faces in the work's own layer over the support's lustre, and its ground in the support's plain glaze.
	 */
	void MakeWare(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A work cast from its scan, standing on its carrier's top (its Mount anchor), its front towards its facing: the scan
	 * scaled to the work's true height (the plan's size), its base slice centred on the plan's `centre`, in its medium's
	 * finish (Data/Art/Casts.json), and any part the record casts in another material (a dancer's tulle skirt) a piece of
	 * its own. A scan whose width at that height strays from the plan's is a finding, and the work isn't made.
	 */
	void MakeCast(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A vessel turned on Form's lathe from its section (Data/Art/Vessels.json, the stage's sheet's), scaled to its plan's
	 * size, in its medium's glaze, standing at its centre on its Mount anchor (a case's deck); where its plan says `stand`,
	 * on its carved stand, a drum in the stand's wood (`<name>.stand`).
	 */
	void MakeVessel(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A scholar's rock standing at its centre on its Mount anchor (the garden's ground, or the carved stand its stage sets
	 * on a table), its front towards its facing: formed as its kind of rock is (Data/Art/Stones.json, Stone/StoneMesh.h) to
	 * its plan's height, its foot within the record's radius, in its medium's finish (Data/Art/Media.json).
	 */
	void MakeStone(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A painting on a wooden panel on a Wall anchor, from the wall out: its cradle on the hanging gap (members along the
	 * grain, slides across them flush with its back), the board on the cradle, both in the wood its `wall` surface names;
	 * the ground and paint on the board, in the work's own layer; and, where the plan frames it, the frame round it, its
	 * lip on the paint and its back on the hanging gap.
	 */
	void MakePanel(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/** A tapestry on a Wall anchor: the weave, in the work's own layer, its back offWall off the wall on its batten and lining. */
	void MakeTapestry(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A hanging scroll on a Wall anchor, its back offWall off the wall: the painting at its place, in the work's own layer
	 * over its silk or paper, laid into its mount (the frame surface's silk: borders either side, a band and the earth
	 * below, a band and the heaven above); the top stick behind the mount's top edge, the roller wound in the earth's
	 * foot, both in the mount's silk, and the roller's knobs past its edges; and the cord from two eyes on the stick up to
	 * its hook.
	 */
	void MakeHangingScroll(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A handscroll opened flat on a case's bed (its Wall anchor: x along the bed, y out of its face, z up its slope), read
	 * from right to left, its back offWall off the bed:
	 * - the painting or writing at its `along` and `height`, at its size: its photographs laid across its sheet in its
	 *   reading order, each a panel in its own layer over the ink on its silk or paper (Data/Art/Supports.json), joined
	 *   without a seam, never stretched;
	 * - the mount in the `frame` surface's silk: borders above and below, the bands at the painting's two ends, and to the
	 *   right of its beginning the lead (the frontispiece silk) and the wrapper as far as they lie unrolled;
	 * - at the right, the stave (a half-round in the fittings' wood) with the rest of the wrapper wound round it, as long
	 *   as `wrapperTurns` takes round the scroll rolled whole;
	 * - at the left, the roller (in the fittings' wood) with the unshown length wound on it to `roll_radius`, in the
	 *   mount's silk, and its knobs (`mount.knob`) on its ends past the scroll's edges.
	 * The fittings are Data/Art/Handscroll.json's, the silk's thickness Data/Art/Scrolls.json's.
	 */
	void MakeHandscroll(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A book lying open on its Mount anchor (a case's bed, a lectern's desk), its spine at its centre, its foot towards
	 * its reader: on each side, its board (the cover, standing its square past the leaves), the leaves that side holds at
	 * its opening, and the page on top in the work's own layer. One object is the opening whole; a work's two book
	 * objects are its verso (the first) and its recto.
	 */
	void MakeBook(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/** A loose leaf lying flat at its centre on its Mount anchor, its foot towards its reader, in the work's own layer. */
	void MakeLeaf(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces);
}
