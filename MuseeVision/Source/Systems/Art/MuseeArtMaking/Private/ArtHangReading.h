#pragma once

#include "ArtHang.h"
#include "ArtWork.h"
#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"
#include "Scroll/ScrollLayout.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	/** The kind of stage anchor a canvas hangs on (the stage contract): a hanging wall. */
	inline const FName WallAnchorKind(TEXT("Wall"));

	/**
	 * The kind of stage anchor a stained-glass panel is set in (the stage contract, D-014): a glazed light, its origin at
	 * the panel's lower-left seen from inside, x along the light, y out of the glass towards the viewer, z up.
	 */
	inline const FName GlazingAnchorKind(TEXT("Glazing"));

	/**
	 * The kind of stage anchor an object is set on (the stage contract): a reader a card or a plate lies on, its origin at
	 * the centre of the face it lies on, x to the viewer's right, y out of the face, z up it; a case's bed, its origin on
	 * the bed, z up, where ware stands at its `centre`.
	 */
	inline const FName MountAnchorKind(TEXT("Mount"));

	/** The kind of stage anchor a moving body is (the stage contract): the body's frame, which a work on it rides. */
	inline const FName BodyAnchorKind(TEXT("Body"));

	/** The supports Art makes, as the hanging plans write them. */
	namespace Supports
	{
		inline const FName Canvas(TEXT("canvas"));
		inline const FName Glass(TEXT("glass"));
		inline const FName Paper(TEXT("paper"));
		inline const FName Plate(TEXT("silvered copper plate"));
		inline const FName Earthenware(TEXT("earthenware"));
		inline const FName Silk(TEXT("silk"));
		inline const FName Panel(TEXT("panel"));
		inline const FName Tapestry(TEXT("tapestry"));
		/** A work in the round made from its scan (the Classical Hall's plaster casts, Degas's bronze dancer). */
		inline const FName Cast(TEXT("cast"));
		/** Chenghuai's ceramics: cast from a scan where the plan gives one. */
		inline const FName Ceramic(TEXT("ceramic"));
		/** A scholar's rock (Chenghuai's 石兄 and its Lingbi rock): formed as its kind of rock is. */
		inline const FName Stone(TEXT("stone"));
	}

	/** Whether an object is cast from its scan: a cast, or ceramic whose plan gives a scan. */
	[[nodiscard]] inline bool IsCast(const FArtHangEntry& Entry)
	{
		return Entry.Support == Supports::Cast || (Entry.Support == Supports::Ceramic && !Entry.Scan.Mesh.IsEmpty());
	}

	/** Whether an object is a vessel turned on Form's lathe from its section: ceramic without a scan. */
	[[nodiscard]] inline bool IsTurned(const FArtHangEntry& Entry)
	{
		return Entry.Support == Supports::Ceramic && Entry.Scan.Mesh.IsEmpty();
	}

	/** Whether an object is a scholar's rock, formed as its kind of rock is. */
	[[nodiscard]] inline bool IsStone(const FArtHangEntry& Entry)
	{
		return Entry.Support == Supports::Stone;
	}

	/** Whether an object stands in the round in its medium's finish (Data/Art/Media.json): cast, turned or a rock. */
	[[nodiscard]] inline bool IsInMedium(const FArtHangEntry& Entry)
	{
		return IsCast(Entry) || IsTurned(Entry) || IsStone(Entry);
	}

	/** A scan's name: its mesh's file name without its extension (`augustus-prima-porta`). */
	[[nodiscard]] FString ScanName(const FArtHangEntry& Entry);

	/** The kinds of object (a plan's `kind`) Art makes of a support that isn't made as a sheet on a wall or a reader. */
	namespace Kinds
	{
		/** A Chinese painting on silk or paper in its mount, hanging by its cord. */
		inline const FName HangingScroll(TEXT("hanging-scroll"));
		/** A Chinese painting or calligraphy on silk or paper, mounted as a handscroll and opened flat on a case's bed. */
		inline const FName Handscroll(TEXT("handscroll"));
		/** A bound book lying open on a case's bed or a lectern's desk. */
		inline const FName Book(TEXT("book"));
		/** A loose sheet (a newspaper's page) lying flat on a case's bed. */
		inline const FName Leaf(TEXT("leaf"));
	}

	/** An open book's sides: its verso on the reader's left, its recto on the right. */
	inline constexpr int32 BookSides = 2;

	/** A plan's frame when the work hangs unframed. */
	inline const FName NoFrame(TEXT("none"));

	/** A finding of Art's about a stage's hanging plan. */
	[[nodiscard]] FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary);

	/**
	 * The stage's hanging plan, if it has one: false when the stage keeps no Hang.json, or when it doesn't read or
	 * names another stage (a finding).
	 */
	[[nodiscard]] bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings);

	/** What the plan's entries are checked against, read once: Care.json's classes and reproductions. */
	struct FCareKeys
	{
		TArray<FName> Classes;
		TArray<FName> Reproductions;
	};

	/** Care.json's classes and reproductions, or why it can't be read. */
	[[nodiscard]] TValueOrError<FCareKeys, FString> ReadCareKeys();

	/**
	 * Whether an entry resolves: to a work its catalogue holds (read with its reproduction's credit), whether Art makes
	 * it or the stage builds it (`"built": "form"`, the one value `built` takes); with a care class and a reproduction
	 * Care.json knows, and a well-formed original's size where it is made after one. The work, or why not.
	 */
	[[nodiscard]] TValueOrError<FArtWork, FString> ResolveEntry(const FArtHangEntry& Entry, const FCareKeys& Care);

	/** How a layer is finished: the palette entry it derives from, and the working and history it is named with. */
	struct FLayerFinish
	{
		FName Palette;
		FName Working;
		FName History;
	};

	/**
	 * One photograph of a handscroll as it lies across the scroll's sheet: the layer it is shown in (`paint.<id>` for a
	 * work shown from its one photograph, `paint.<id>.<n>` for its photograph `<id>-<n>`), what Studio names that layer's
	 * texture for (`<id>` or `<id>.<n>`), the photograph, and where it lies.
	 */
	struct FScrollSection
	{
		FString Work;
		FString ImagePath;
		FName Layer;
		FScrollPanel Panel;
		/** The photograph's number, <id>-<n>; 1 for a work shown from its one photograph. */
		int32 Number = 1;
	};

	/**
	 * One object of a work the stage makes: its entry in the plan; where its left edge falls in the work's one image, or
	 * its own image; the palette key of the layer it shows, and how that layer is finished; how its faces without the
	 * image are finished (a dish's underside), the glass it lies under (its thickness, from the glazing's working), and,
	 * for turned ware, its form; for a painted vessel, the size of its decoration; for a handscroll, its photographs laid
	 * across its sheet, left to right, and the length of its mount's separator and frontispiece before the work at its
	 * right end (m), from Data/Art/Handscrolls.json.
	 */
	struct FCanvasPlan
	{
		const FArtHangEntry* Entry = nullptr;
		double ImageOffsetMeters = 0.0;
		FString OwnImagePath;
		FName Layer;
		FLayerFinish Finish;
		FLayerFinish Ground;
		double GlazingMeters = 0.0;
		/** A transparency's plate and the diffusing pane behind it: their thickness, from their glass's working (m). */
		double PlateMeters = 0.0;
		double BackingMeters = 0.0;
		FWareSpec Ware;
		TArray<FScrollSection> Sections;
		double FrontMeters = 0.0;
		/** A painted vessel's decoration: the size of its turned surface laid out flat (metres round, metres along). */
		FVector2D DecorationMeters = FVector2D::ZeroVector;
	};

	/**
	 * A work the stage makes: the work (with its image), its objects in order, and the one image's size. A work whose
	 * objects each show their own image (a series of canvases, a band of panels) has no one image.
	 */
	struct FWorkPlan
	{
		FArtWork Work;
		TArray<FCanvasPlan> Canvases;
		FVector2D ImageMeters = FVector2D::ZeroVector;
		bool bOwnImages = false;

		[[nodiscard]] bool IsSplit() const { return Canvases.Num() > 1; }
	};

	/**
	 * Every work of a hanging plan that Art can make, checked whole: each object's entry (it resolves; its anchor, size,
	 * support, and the surfaces it names, its frame's, its mount's, its glass's and its glazing's, in the stage's finish
	 * schedule), the work's objects (each index once; sharing one image at one height, or each showing its own) and its
	 * images; and each support's finishes in the palette (Data/Art/Supports.json). Works the stage builds are left to
	 * it. What can't be made is a finding and is left out; the works point into Hang, which must outlive them. False,
	 * with a finding, when the supports' record or their finishes can't be read.
	 */
	[[nodiscard]] bool PlanWorks(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FWorkPlan>& OutWorks, TArray<FMuseeFinding>& OutFindings);
}
