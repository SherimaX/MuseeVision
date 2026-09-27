#include "Growth/Organs.h"
#include "Growth/Plant.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
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

		/** The water a bed lies in, seen from the bed: its entry, its datum in the stage, and the datum in the bed's frame. */
		struct FBedWater
		{
			const FNatureWater* Water = nullptr;
			FMuseeFrame Datum;
			FVector DatumInBed = FVector::ZeroVector;
		};

		/** The body of water whose outline holds a point of the stage, or none. */
		FBedWater WaterAt(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FMuseeFrame& Bed)
		{
			for (const FNatureWater& Water : Plan.Water)
			{
				const FMuseeStageAnchor* Datum = Stage.FindAnchor(Water.Anchor);
				if (!Datum || Datum->Kind != AnchorKind::Water)
				{
					continue;
				}
				const FVector Local = Datum->Frame.PointFromParent(Bed.OriginMeters);
				if (Water.Outline.Contains(FVector2D(Local.X, Local.Y), 0.0))
				{
					return { &Water, Datum->Frame, Bed.PointFromParent(Datum->Frame.OriginMeters) };
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
			Site.Inward = (FVector2D(In.DatumInBed.X, In.DatumInBed.Y) - FVector2D(Crown.X, Crown.Y)).GetSafeNormal();
			const FNatureWater& Water = *In.Water;
			const FMuseeFrame Datum = In.Datum;
			Site.IsClear = [&Water, Datum, Bed](const FVector& Point, double Margin)
			{
				const FVector Local = Datum.PointFromParent(Bed.PointToParent(Point));
				return Local.Z > Water.Freeboard + OverhangClearanceMeters || Water.Outline.Contains(FVector2D(Local.X, Local.Y), Margin);
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

		/**
		 * The crowns of one planting, in its bed's frame: a Vogel spiral round the anchor at the planting's spacing,
		 * turned so plantings sharing the bed interleave, keeping each crown inside the bed's water (less the plant's
		 * reach) and nearer its own bed than any other bed in that water.
		 */
		TArray<FVector> Crowns(const FPlanting& Planting, int32 Turn, int32 Turns, const FMuseeFrame& Bed, const FBedWater& In, const TArray<FVector2D>& Neighbours)
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

		void Grow(const FMuseeStageContext& Stage, const FPlanting& Planting, const TArray<FVector>& Places, const FMuseeFrame& Bed,
			const FBedWater& In, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const FNatureBed& Entry = *Planting.Entry;
			FPlantParts Parts;
			const FMuseeKey Stream = Entry.Bed.Child(Planting.Species->Key.ToString());
			for (int32 Index = 0; Index < Places.Num(); ++Index)
			{
				FRandomStream Random(Seed(Stream.Child(FString::Printf(TEXT("plant-%d"), Index))));
				Planting.Form->Grow(*Planting.Species, SiteAt(Places[Index], Bed, In), Random, Parts);
			}
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
				Piece.Name = FName(FString::Printf(TEXT("%s.%s.%s"), *Entry.Bed.ToString(), *Planting.Species->Key.ToString(), *Organ.ToString()));
				Piece.Surface = FName(FString::Printf(TEXT("plant.%s"), *Organ.ToString()));
				Piece.Material = Planting.Species->MaterialOf(Organ);
				Piece.Body = Entry.Body;
				Piece.Collision = NAME_None;
				Piece.bNanite = true;
				Piece.Frame = Frame;
				Surface->Write(Piece.Mesh);
			}
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
				const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Entry.Bed);
				TArray<FString> Faults;
				if (!Anchor || Anchor->Kind != AnchorKind::Bed)
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
				if (Entry.Count <= 0 || Entry.Spacing <= 0.0f)
				{
					Faults.Add(TEXT("it needs a count and a spacing of more than zero"));
				}
				if (!Faults.IsEmpty())
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s (%s): %s."), *Entry.Bed.ToString(), *Entry.Species, *FString::Join(Faults, TEXT("; ")))));
					continue;
				}
				Out.Add({ &Entry, Found, FindForm(Found->Form) });
			}
			return Out;
		}

		/** The plan positions of the other beds lying in the same water, seen from a bed. */
		TArray<FVector2D> NeighbourBeds(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FMuseeKey& Own, const FMuseeFrame& Bed, const FBedWater& In)
		{
			TArray<FVector2D> Out;
			for (const FNatureBed& Other : Plan.Beds)
			{
				const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Other.Bed);
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
				const FMuseeFrame& Bed = Stage.FindAnchor(Key)->Frame;
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
					if (Places.Num() < Planting.Entry->Count)
					{
						OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Bed %s holds %d of its %d %s at a spacing of %.2f m."), *Key.ToString(), Places.Num(),
							Planting.Entry->Count, *Planting.Species->Name, Planting.Entry->Spacing)));
					}
					Grow(Stage, Planting, Places, Bed, In, OutPieces, OutFindings);
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
