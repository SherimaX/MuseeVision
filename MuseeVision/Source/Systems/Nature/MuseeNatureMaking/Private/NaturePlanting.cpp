#include "Growth/Organs.h"
#include "Growth/Plant.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "NatureAnchors.h"
#include "NaturePieces.h"
#include "NaturePlanting.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/**
		 * A leaf may reach out over the rim only this far above the rim's top: the museum's copings and lips stand at
		 * most a few centimetres over brimming water (the oval's lip 23 mm, its seat 40 mm), so a leaf crossing the rim
		 * lower than 5 cm above it would pass through the stone.
		 */
		constexpr double OverhangClearanceMeters = 0.05;

		/** Each plant takes the area of a hexagonal planting at the bed's spacing, (sqrt 3 / 2) s^2. */
		constexpr double HexagonalAreaShare = 0.86602540378443865;

		/** The golden angle, pi (3 - sqrt 5), which spreads plants evenly round a crown (Vogel, Math. Biosci. 44, 1979). */
		constexpr double GoldenAngle = 2.39996322972865332;

		/** How many places the layout tries per plant before the bed is found too small. */
		constexpr int32 PlacesPerPlant = 8;

		const FName NatureSystem(TEXT("Nature"));

		FMuseeFinding Finding(const FMuseeStageContext& Stage, const FString& Summary)
		{
			FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
			Found.Owner = NatureSystem;
			return Found;
		}

		/** The water a bed lies in, seen from the bed: its entry, its datum in the stage, its outline seen from the datum, and the datum in the bed's frame. */
		struct FBedWater
		{
			const FNatureWater* Water = nullptr;
			FMuseeFrame Datum;
			FNatureOutline Outline;
			FVector DatumInBed = FVector::ZeroVector;
		};

		/** The body of water whose outline holds a point of the stage, or none. */
		FBedWater WaterAt(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FMuseeFrame& Bed)
		{
			for (const FNatureWater& Water : Plan.Water)
			{
				const FMuseeStageAnchor* Datum = Stage.FindAnchor(Water.Anchor, AnchorKind::Water);
				if (!Datum)
				{
					continue;
				}
				const FNatureOutline Seen = Water.Outline.SeenFrom(Datum->Frame);
				const FVector Local = Datum->Frame.PointFromParent(Bed.OriginMeters);
				if (Seen.Contains(FVector2D(Local.X, Local.Y), 0.0))
				{
					return { &Water, Datum->Frame, Seen, Bed.PointFromParent(Datum->Frame.OriginMeters) };
				}
			}
			return {};
		}

		/** The site of a plant at a crown of its bed: its water, the lean to the water's middle, and the rim it keeps off. */
		FPlantSite SiteAt(const FVector& Crown, const FMuseeFrame& Bed, const FBedWater& In)
		{
			FPlantSite Site;
			Site.Crown = Crown;
			if (!In.Water)
			{
				return Site;
			}
			Site.WaterLevel = In.DatumInBed.Z - Crown.Z;
			Site.WaterDepth = In.Water->Depth;
			Site.Inward = (FVector2D(In.DatumInBed.X, In.DatumInBed.Y) - FVector2D(Crown.X, Crown.Y)).GetSafeNormal();
			const FNatureWater& Water = *In.Water;
			const FNatureOutline Outline = In.Outline;
			const FMuseeFrame Datum = In.Datum;
			Site.IsClear = [&Water, Outline, Datum, Bed](const FVector& Point, double Margin)
			{
				const FVector Local = Datum.PointFromParent(Bed.PointToParent(Point));
				return Local.Z > Water.Freeboard + OverhangClearanceMeters || Outline.Contains(FVector2D(Local.X, Local.Y), Margin);
			};
			return Site;
		}

		/** One planting to grow: the plan's entry, its species and form, and where it goes. */
		struct FPlanting
		{
			const FNatureBed* Entry = nullptr;
			const FNatureSpecies* Species = nullptr;
			const FForm* Form = nullptr;
		};

		/** A plan point of the stage, on the soil, in the bed's frame. */
		FVector OnSoil(const FMuseeFrame& Bed, const FVector2D& Plan)
		{
			return Bed.PointFromParent(FVector(Plan.X, Plan.Y, Bed.OriginMeters.Z));
		}

		/**
		 * The crowns round the anchor, in its bed's frame: a Vogel spiral at the planting's spacing, turned so plantings
		 * sharing the bed interleave, keeping each crown inside the bed's water (less the plant's reach) and nearer its own
		 * bed than any other bed in that water.
		 */
		TArray<FVector> Spiral(const FPlanting& Planting, int32 Turn, int32 Turns, const FMuseeFrame& Bed, const FBedWater& In, const TArray<FVector2D>& Neighbours)
		{
			const FNatureBed& Entry = *Planting.Entry;
			const double Margin = Planting.Species->ReachMeters.Num() == 2 ? Planting.Species->ReachMeters[1] : 0.0;
			FRandomStream Random(Seed(Entry.Bed));
			const double Offset = Random.FRandRange(0.0, Tau) + Tau * Turn / FMath::Max(1, Turns);
			const FPlantSite Rim = SiteAt(FVector::ZeroVector, Bed, In);
			TArray<FVector> Out;
			for (int32 N = 0; N < Entry.Count * PlacesPerPlant && Out.Num() < Entry.Count; ++N)
			{
				const double R = Entry.Spacing * FMath::Sqrt(HexagonalAreaShare * (N + 0.5) / UE_DOUBLE_PI);
				const double A = Offset + N * GoldenAngle;
				const FVector2D Plan(R * FMath::Cos(A), R * FMath::Sin(A));
				const bool bInWater = !Rim.IsClear || Rim.IsClear(FVector(Plan.X, Plan.Y, 0.0), Margin);
				const bool bOwnBed = !Neighbours.ContainsByPredicate([&Plan](const FVector2D& Other) { return FVector2D::DistSquared(Plan, Other) < Plan.SizeSquared(); });
				if (bInWater && bOwnBed)
				{
					Out.Add(FVector(Plan.X, Plan.Y, 0.0));
				}
			}
			return Out;
		}

		/**
		 * Where a planting's plants stand, in its bed's frame: at its points, at its point, once for a form planted along a
		 * line or over a region (at the anchor), or on the spiral round the anchor.
		 */
		TArray<FVector> Crowns(const FPlanting& Planting, int32 Turn, int32 Turns, const FMuseeFrame& Bed, const FBedWater& In, const TArray<FVector2D>& Neighbours)
		{
			const FNatureBed& Entry = *Planting.Entry;
			TArray<FVector> Out;
			if (!Entry.Points.IsEmpty())
			{
				for (const FVector2D& Point : Entry.Points)
				{
					Out.Add(OnSoil(Bed, Point));
				}
				return Out;
			}
			if (Entry.Point.Num() == 2)
			{
				return { OnSoil(Bed, FVector2D(Entry.Point[0], Entry.Point[1])) };
			}
			if (Planting.Form->Planting != EPlanting::EachPlant || (Entry.Count == 1 && !In.Water))
			{
				return { FVector::ZeroVector };
			}
			return Spiral(Planting, Turn, Turns, Bed, In, Neighbours);
		}

		/** The site's further facts from the plan: a tree's height and crown, a hedge's line and section, a cover's region, a climber's paths. */
		void DrawnFacts(const FNatureBed& Entry, const FMuseeFrame& Bed, const FBedRegion& Region, FPlantSite& Site)
		{
			Site.Height = Entry.Height;
			Site.CrownCentre = Entry.CrownCentre;
			Site.CrownRadii = Entry.CrownRadii.Num() == 2 ? FVector2D(Entry.CrownRadii[0], Entry.CrownRadii[1]) : FVector2D::ZeroVector;
			for (const FVector2D& End : Entry.Line)
			{
				Site.Line.Add(OnSoil(Bed, End));
			}
			Site.Clip = Entry.Clipped;
			Site.Region = Region.Outline;
			Site.Holes = Region.Holes;
			for (const FNatureTraining& Path : Entry.Trained)
			{
				TArray<FVector>& Local = Site.Trained.AddDefaulted_GetRef();
				for (const FVector& Point : Path.Path)
				{
					Local.Add(Bed.PointFromParent(Point));
				}
			}
		}

		/**
		 * What a grown part is made of: a cast specimen's metal, which the visitor can't pass through; or its organ's own
		 * living tissue (a lily's pad a floating leaf, its petal a thin translucent sheet), worked and aged as its bed says,
		 * which nothing collides with but a trunk or a hedge's core.
		 */
		void MaterialOfPart(const FNatureBed& Entry, const FNatureSpecies& Species, FName Organ, FFormMadePiece& OutPiece)
		{
			if (Entry.Cast.IsSet())
			{
				OutPiece.Material = Entry.Cast.Material;
				OutPiece.Working = Entry.Cast.Working;
				OutPiece.History = Entry.Cast.History;
				OutPiece.Collision = Collision::Solid.Profile();
				return;
			}
			OutPiece.Material = Species.MaterialOf(Organ);
			OutPiece.Working = Entry.Working;
			OutPiece.History = Entry.History;
			OutPiece.Collision = Organ == Organ::Trunk ? Collision::Solid.Profile() : NAME_None;
		}

		/** What is wrong with a bed's count, places, spacing and material spec for its form, one clause each. */
		TArray<FString> BedFaults(const FNatureBed& Entry, const FForm& Form)
		{
			TArray<FString> Out;
			if (Entry.Count <= 0 || Entry.Spacing < 0.0f)
			{
				Out.Add(TEXT("it needs a count of more than zero"));
			}
			if (!Entry.Points.IsEmpty() && Entry.Points.Num() != Entry.Count)
			{
				Out.Add(FString::Printf(TEXT("it counts %d plants at %d points"), Entry.Count, Entry.Points.Num()));
			}
			if (!Entry.Point.IsEmpty() && (Entry.Point.Num() != 2 || Entry.Count != 1))
			{
				Out.Add(TEXT("a point is [x, y], for one plant"));
			}
			const bool bPlaced = !Entry.Points.IsEmpty() || !Entry.Point.IsEmpty();
			if (Form.Planting == EPlanting::EachPlant && !bPlaced && Entry.Count > 1 && Entry.Spacing <= 0.0f)
			{
				Out.Add(TEXT("more than one plant needs its points or a spacing of more than zero"));
			}
			if (Form.Planting == EPlanting::AlongLine && (Entry.Line.Num() != 2 || !Entry.Clipped.IsSet()))
			{
				Out.Add(TEXT("a hedge needs its line, [[x, y], [x, y]], and the section it is clipped to"));
			}
			if (Form.Planting == EPlanting::OverRegion && !Entry.Outline.IsValid())
			{
				Out.Add(TEXT("a cover needs the bed's outline to grow over"));
			}
			if (Entry.Cast.IsSet())
			{
				if (Entry.Count != 1 || Entry.Cast.Working.IsNone() || Entry.Cast.History.IsNone() || !Entry.Working.IsNone() || !Entry.History.IsNone())
				{
					Out.Add(TEXT("a cast specimen is one plant, cast in a material with its working and history, which are the cast's alone"));
				}
			}
			else if (Entry.Working.IsNone() || Entry.History.IsNone())
			{
				Out.Add(TEXT("it needs the working and history its plants are resolved with"));
			}
			return Out;
		}

		/** The pieces of a planting's parts, one to an organ, in a frame (its bed's, or a module's), their names ending in a suffix. */
		void AddParts(const FMuseeStageContext& Stage, const FPlanting& Planting, const FPlantParts& Parts, const FMuseeFrame& Bed, TArray<FFormMadePiece>& OutPieces,
			TArray<FMuseeFinding>& OutFindings, const FString& Suffix = FString())
		{
			const FNatureBed& Entry = *Planting.Entry;
			FMuseeFrame Frame;
			FString Why;
			if (!PieceFrame(Stage, Bed, Entry.Body, Frame, Why))
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s: %s."), *Entry.Bed.ToString(), *Why)));
				return;
			}
			for (const FName Organ : Planting.Form->Organs)
			{
				const FNatureSurface* Surface = Parts.Find(Organ);
				if (!Surface || Surface->IsEmpty())
				{
					continue;
				}
				FFormMadePiece& Piece = OutPieces.AddDefaulted_GetRef();
				Piece.Name = FName(FString::Printf(TEXT("%s.%s.%s%s"), *Entry.Bed.ToString(), *Planting.Species->Key.ToString(), *Organ.ToString(), *Suffix));
				Piece.Surface = FName(FString::Printf(TEXT("plant.%s"), *Organ.ToString()));
				MaterialOfPart(Entry, *Planting.Species, Organ, Piece);
				Piece.Body = Entry.Body;
				Piece.bNanite = true;
				Piece.Frame = Frame;
				Surface->Write(Piece.Mesh);
			}
		}

		/**
		 * A hedge along its line, as a run of identical modules, one to each plant at the spacing (so the stage's casting may
		 * place one module many times): each grown in its own frame, the line along its x, from the stream of the bed's
		 * middle modules, its first and last from their own streams, clipped across where the hedge's ends are open.
		 */
		void GrowModules(const FMuseeStageContext& Stage, const FPlanting& Planting, const FMuseeFrame& Bed, const FBedRegion& Region, TArray<FFormMadePiece>& OutPieces,
			TArray<FMuseeFinding>& OutFindings)
		{
			const FNatureBed& Entry = *Planting.Entry;
			FPlantSite Line;
			DrawnFacts(Entry, Bed, Region, Line);
			const FVector Span = Line.Line[1] - Line.Line[0];
			const FVector Along = FVector(Span.X, Span.Y, 0.0).GetSafeNormal();
			const int32 Modules = FMath::Max(1, Entry.Count);
			const double Module = FVector2D(Span.X, Span.Y).Size() / Modules;
			const FQuat Turn = Bed.Rotation * FQuat(FVector::UpVector, FMath::Atan2(Along.Y, Along.X));
			const bool bOpen = Entry.Clipped.Ends == TEXT("open");
			const FMuseeKey Stream = Entry.Bed.Child(Planting.Species->Key.ToString());
			TMap<FString, FPlantParts> Grown;
			for (int32 Index = 0; Index < Modules; ++Index)
			{
				const bool bFirst = Index == 0;
				const bool bLast = Index == Modules - 1;
				const FString Kind = bFirst && bLast ? TEXT("whole") : bFirst ? TEXT("first") : bLast ? TEXT("last") : TEXT("module");
				FPlantParts* Parts = Grown.Find(Kind);
				if (!Parts)
				{
					FPlantSite Site = Line;
					Site.Line = { FVector::ZeroVector, FVector(Module, 0.0, 0.0) };
					Site.bOpenStart = bOpen && bFirst;
					Site.bOpenEnd = bOpen && bLast;
					FRandomStream Random(Seed(Stream.Child(Kind)));
					Parts = &Grown.Add(Kind);
					Planting.Form->Grow(*Planting.Species, Site, Random, *Parts);
				}
				const FMuseeFrame Frame(Bed.PointToParent(Line.Line[0] + Along * (Module * Index)), Turn);
				AddParts(Stage, Planting, *Parts, Frame, OutPieces, OutFindings, FString::Printf(TEXT(".%d"), Index));
			}
		}

		void Grow(const FMuseeStageContext& Stage, const FPlanting& Planting, const TArray<FVector>& Places, const FMuseeFrame& Bed, const FBedWater& In,
			const FBedRegion& Region, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const FNatureBed& Entry = *Planting.Entry;
			if (!Planting.Form->Grow)
			{
				FFormMadePiece Carpet;
				FString Why;
				if (MakeCarpetPiece(Stage, Entry, *Planting.Species, Region, Bed, Carpet, Why))
				{
					OutPieces.Add(MoveTemp(Carpet));
				}
				else
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s: %s."), *Entry.Bed.ToString(), *Why)));
				}
				return;
			}
			if (Planting.Form->Planting == EPlanting::AlongLine)
			{
				GrowModules(Stage, Planting, Bed, Region, OutPieces, OutFindings);
				return;
			}
			FPlantParts Parts;
			const FMuseeKey Stream = Entry.Bed.Child(Planting.Species->Key.ToString());
			for (int32 Index = 0; Index < Places.Num(); ++Index)
			{
				FRandomStream Random(Seed(Stream.Child(FString::Printf(TEXT("plant-%d"), Index))));
				FPlantSite Site = SiteAt(Places[Index], Bed, In);
				DrawnFacts(Entry, Bed, Region, Site);
				Planting.Form->Grow(*Planting.Species, Site, Random, Parts);
			}
			AddParts(Stage, Planting, Parts, Bed, OutPieces, OutFindings);
		}
	}

	namespace
	{
		/** The plantings of the plan, checked: each finds its species and its form, or is a finding. */
		TArray<FPlanting> Plantings(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const TArray<FNatureSpecies>& Species, TArray<FMuseeFinding>& OutFindings)
		{
			TArray<FPlanting> Out;
			for (const FNatureBed& Entry : Plan.Beds)
			{
				const FNatureSpecies* Found = Species.FindByPredicate([&Entry](const FNatureSpecies& S) { return S.Name == Entry.Species; });
				TArray<FString> Faults;
				if (!Stage.FindAnchor(Entry.Bed, AnchorKind::Bed))
				{
					Faults.Add(FString::Printf(TEXT("%s is not a Bed anchor of the stage"), *Entry.Bed.ToString()));
				}
				if (!Found)
				{
					Faults.Add(FString::Printf(TEXT("no record in Data/Nature/Species names '%s'"), *Entry.Species));
				}
				else
				{
					Faults.Append(SpeciesFaults(*Found));
				}
				const FForm* Form = Found ? FindForm(Found->Form) : nullptr;
				if (Form)
				{
					Faults.Append(BedFaults(Entry, *Form));
				}
				if (!Faults.IsEmpty())
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s (%s): %s."), *Entry.Bed.ToString(), *Entry.Species, *FString::Join(Faults, TEXT("; ")))));
					continue;
				}
				Out.Add({ &Entry, Found, Form });
			}
			return Out;
		}

		/** The plan positions of the other beds lying in the same water, seen from a bed. */
		TArray<FVector2D> NeighbourBeds(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FMuseeKey& Own, const FMuseeFrame& Bed, const FBedWater& In)
		{
			TArray<FVector2D> Out;
			for (const FNatureBed& Other : Plan.Beds)
			{
				const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Other.Bed, AnchorKind::Bed);
				if (Other.Bed == Own || !Anchor || WaterAt(Stage, Plan, Anchor->Frame).Water != In.Water)
				{
					continue;
				}
				const FVector Local = Bed.PointFromParent(Anchor->Frame.OriginMeters);
				Out.AddUnique(FVector2D(Local.X, Local.Y));
			}
			return Out;
		}

		void GrowBeds(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const TArray<FPlanting>& All, TArray<FFormMadePiece>& OutPieces,
			TArray<FMuseeFinding>& OutFindings)
		{
			TArray<FMuseeKey> Beds;
			for (const FPlanting& Planting : All)
			{
				Beds.AddUnique(Planting.Entry->Bed);
			}
			for (const FMuseeKey& Key : Beds)
			{
				const FMuseeFrame& Bed = Stage.FindAnchor(Key, AnchorKind::Bed)->Frame;
				const FBedWater In = WaterAt(Stage, Plan, Bed);
				const TArray<FVector2D> Neighbours = NeighbourBeds(Stage, Plan, Key, Bed, In);
				TArray<const FPlanting*> Here;
				for (const FPlanting& Planting : All)
				{
					if (Planting.Entry->Bed == Key)
					{
						Here.Add(&Planting);
					}
				}
				for (int32 Turn = 0; Turn < Here.Num(); ++Turn)
				{
					const FPlanting& Planting = *Here[Turn];
					if (Planting.Form->bInWater && !In.Water)
					{
						OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s: %s grows in water, and the bed lies in none of the plan's."), *Key.ToString(), *Planting.Species->Name)));
						continue;
					}
					const TArray<FVector> Places = Crowns(Planting, Turn, Here.Num(), Bed, In, Neighbours);
					const bool bOnce = Planting.Form->Planting != EPlanting::EachPlant;
					if (!bOnce && Places.Num() < Planting.Entry->Count)
					{
						OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s holds %d of its %d %s at a spacing of %.2f m."), *Key.ToString(), Places.Num(),
							Planting.Entry->Count, *Planting.Species->Name, Planting.Entry->Spacing)));
					}
					Grow(Stage, Planting, Places, Bed, In, RegionOf(Stage, Plan, *Planting.Entry, Bed), OutPieces, OutFindings);
				}
			}
		}

		/** The ground of each bed that gives one: at most one ground a bed. */
		void MakeGrounds(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			TSet<FMuseeKey> Made;
			for (const FNatureBed& Entry : Plan.Beds)
			{
				const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Entry.Bed, AnchorKind::Bed);
				if (!Entry.Ground.IsSet() || !Anchor)
				{
					continue;
				}
				bool bAlready = false;
				Made.Add(Entry.Bed, &bAlready);
				FFormMadePiece Piece;
				FString Why;
				if (bAlready)
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s: its ground is given twice."), *Entry.Bed.ToString())));
				}
				else if (MakeGroundPiece(Stage, Entry, RegionOf(Stage, Plan, Entry, Anchor->Frame), Anchor->Frame, Piece, Why))
				{
					OutPieces.Add(MoveTemp(Piece));
				}
				else
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s: %s."), *Entry.Bed.ToString(), *Why)));
				}
			}
		}
	}

	void GatherNature(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FString Folder = FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
		const FString Path = Folder / FNaturePlanting::FileName;
		if (!IFileManager::Get().FileExists(*Path))
		{
			return;
		}
		const TValueOrError<FNaturePlanting, FString> Plan = ReadRecord<FNaturePlanting>(Path);
		if (Plan.HasError())
		{
			OutFindings.Add(Finding(Stage, Plan.GetError()));
			return;
		}
		if (!(Plan.GetValue().Stage == Stage.Stage))
		{
			OutFindings.Add(Finding(Stage, FString::Printf(TEXT("%s names the stage %s."), *Path, *Plan.GetValue().Stage.ToString())));
			return;
		}
		for (const FNatureWater& Water : Plan.GetValue().Water)
		{
			FFormMadePiece Piece;
			FString Why;
			if (MakeWaterPiece(Stage, Water, Piece, Why))
			{
				OutPieces.Add(MoveTemp(Piece));
			}
			else
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Water %s: %s."), *Water.Key.ToString(), *Why)));
			}
		}
		MakeGrounds(Stage, Plan.GetValue(), OutPieces, OutFindings);
		if (Plan.GetValue().Beds.IsEmpty())
		{
			return;
		}
		const TValueOrError<TArray<FNatureSpecies>, FString> Species = ReadSpecies();
		if (Species.HasError())
		{
			OutFindings.Add(Finding(Stage, Species.GetError()));
			return;
		}
		GrowBeds(Stage, Plan.GetValue(), Plantings(Stage, Plan.GetValue(), Species.GetValue(), OutFindings), OutPieces, OutFindings);
	}
}
