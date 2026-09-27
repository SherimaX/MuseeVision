#include "Schedule/LightDresser.h"

#include "GameFramework/Actor.h"
#include "LightLuminaire.h"
#include "LightLuminaireComponent.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "Schedule/LightingRecords.h"
#include "UObject/Package.h"

namespace Musee::Light
{
	namespace
	{
		const TCHAR* const ScheduleFile = TEXT("Lighting.json");
		const FName MountKind(TEXT("Mount"));

		/** The places whose weather may fall on an opening (Decision D-005, the stage contract). */
		const TCHAR* const WeatherPlaces[] = { TEXT("museum"), TEXT("giverny"), TEXT("london") };

		/** Where the stage's schedule is: its folder as given, or under Data/ or the project. */
		FString FindSchedule(const FString& Folder)
		{
			const FString Candidates[] = {
				Folder / ScheduleFile,
				Records::DataRoot() / Folder / ScheduleFile,
				FPaths::ProjectDir() / Folder / ScheduleFile,
			};
			for (const FString& Candidate : Candidates)
			{
				if (!FPaths::IsRelative(Candidate) && FPaths::FileExists(Candidate)) { return FPaths::ConvertRelativePathToFull(Candidate); }
			}
			return FString();
		}

		/** One luminaire of the schedule, checked against the stage and the catalogue. */
		struct FPlacement
		{
			FMuseeKey Type;
			const FMuseeStageAnchor* Mount = nullptr;
			FVector Aim = FVector::ZeroVector;
			ELightOperation Operation = ELightOperation::Always;
			FMuseeKey Follows;
			USceneComponent* Body = nullptr;
		};

		TOptional<ELightOperation> ParseOperation(const FString& Text)
		{
			if (Text == TEXT("always")) { return ELightOperation::Always; }
			if (Text == TEXT("dusk")) { return ELightOperation::Dusk; }
			if (Text == TEXT("presence")) { return ELightOperation::Presence; }
			return {};
		}

		USceneComponent* FindBody(const AActor& StageActor, const FString& Body)
		{
			TArray<USceneComponent*> Components;
			StageActor.GetComponents(Components);
			const FName Tag(*Body);
			USceneComponent* const* Found = Components.FindByPredicate([Tag](const USceneComponent* C) { return C->ComponentHasTag(Tag); });
			return Found ? *Found : nullptr;
		}

		/** Checks one entry; the reason it can't be placed, or empty. */
		FString Check(const FMuseeStageContext& Stage, const FLightScheduleLuminaire& Entry, FPlacement& Out)
		{
			const TOptional<FMuseeKey> Type = FMuseeKey::Parse(Entry.Type);
			const FLightLuminaire* Luminaire = Type.IsSet() ? FindLuminaire(Type.GetValue()) : nullptr;
			if (!Luminaire) { return FString::Printf(TEXT("its type '%s' isn't in the catalogue"), *Entry.Type); }
			Out.Type = Type.GetValue();
			const TOptional<FMuseeKey> Mount = FMuseeKey::Parse(Entry.Mount);
			Out.Mount = Mount.IsSet() ? Stage.FindAnchor(Mount.GetValue()) : nullptr;
			if (!Out.Mount || Out.Mount->Kind != MountKind) { return FString::Printf(TEXT("the stage has no Mount anchor '%s'"), *Entry.Mount); }
			if (Entry.Aim.Num() == 3) { Out.Aim = FVector(Entry.Aim[0], Entry.Aim[1], Entry.Aim[2]).GetSafeNormal(); }
			if (Entry.Aim.Num() != 0 && Out.Aim.IsZero()) { return TEXT("its aim isn't a direction [x, y, z]"); }
			const TOptional<ELightOperation> Operation = ParseOperation(Entry.Operation);
			if (!Operation.IsSet()) { return FString::Printf(TEXT("its operation '%s' is none of always, dusk and presence"), *Entry.Operation); }
			Out.Operation = Operation.GetValue();
			if (!Entry.Follows.IsEmpty())
			{
				const TOptional<FMuseeKey> Follows = FMuseeKey::Parse(Entry.Follows);
				if (!Follows.IsSet()) { return FString::Printf(TEXT("the channel it follows, '%s', isn't a key"), *Entry.Follows); }
				Out.Follows = Follows.GetValue();
			}
			if (!Entry.Body.IsEmpty())
			{
				Out.Body = FindBody(*Stage.StageActor, Entry.Body);
				if (!Out.Body) { return FString::Printf(TEXT("the stage has no moving body '%s'"), *Entry.Body); }
			}
			if (Luminaire->Emitter == FLightLuminaire::EEmitter::Linear && Entry.Length <= 0.0) { return TEXT("a linear type needs the length laid"); }
			return FString();
		}

		/** The luminaire's frame in the stage: at its mount, its light leaving along the aim, its mount's z kept up. */
		FMuseeFrame FrameOf(const FPlacement& Placement)
		{
			const FMuseeFrame& Mount = Placement.Mount->Frame;
			const FVector Along = Placement.Aim.IsZero() ? Mount.Rotation.GetAxisX() : Placement.Aim;
			const FVector Up = FMath::Abs(Along | Mount.Rotation.GetAxisZ()) < 1.0 - UE_KINDA_SMALL_NUMBER ? Mount.Rotation.GetAxisZ() : Mount.Rotation.GetAxisY();
			return FMuseeFrame(Mount.OriginMeters, FRotationMatrix::MakeFromXZ(Along, Up).ToQuat());
		}

		void Place(const FMuseeStageContext& Stage, const FLightScheduleLuminaire& Entry, const FPlacement& Placement)
		{
			AActor* Actor = Stage.StageActor;
			USceneComponent* Root = Actor->GetRootComponent();
			USceneComponent* Parent = Placement.Body ? Placement.Body : Root;
			const FName Name = MakeUniqueObjectName(Actor, ULightLuminaireComponent::StaticClass(), FName(*(TEXT("Luminaire_") + Entry.Id.Replace(TEXT("."), TEXT("_")))));
			ULightLuminaireComponent* Component = NewObject<ULightLuminaireComponent>(Actor, Name);
			Component->Configure(Placement.Type, Placement.Operation, Placement.Follows, Meters(Entry.Length));
			Component->ComponentTags.Add(FName(FString(LuminaireTagPrefix) + Entry.Id));
			const FTransform InStage = ToEngine(FrameOf(Placement)) * Root->GetComponentTransform();
			Component->SetupAttachment(Parent);
			Component->SetRelativeTransform(InStage.GetRelativeTransform(Parent->GetComponentTransform()));
			Actor->AddInstanceComponent(Component);
			Component->RegisterComponent();
		}
	}

	void DressStage(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		const FName Drawer(*Stage.Stage.ToString());
		const auto Find = [&OutFindings, Drawer](const FString& Summary) { OutFindings.Add(FMuseeFinding::Make(Drawer, Summary, FMuseeMeasurement())); };
		const FString Path = FindSchedule(Stage.ScheduleDir);
		if (Path.IsEmpty() || !Stage.StageActor || !Stage.StageActor->GetRootComponent())
		{
			if (!Path.IsEmpty()) { Find(TEXT("Light can't dress a stage without its stage actor.")); }
			return;
		}
		TValueOrError<FLightSchedule, FString> Read = ReadRecord<FLightSchedule>(Path);
		if (Read.HasError())
		{
			Find(FString::Printf(TEXT("The lighting schedule can't be read: %s"), *Read.GetError()));
			return;
		}
		const FLightSchedule& Schedule = Read.GetValue();
		if (Schedule.Stage != Stage.Stage.ToString())
		{
			Find(FString::Printf(TEXT("The lighting schedule at %s is stage '%s''s, not '%s''s."), *Path, *Schedule.Stage, *Stage.Stage.ToString()));
		}
		for (const FLightScheduleDaylight& Daylight : Schedule.Daylight)
		{
			if (!TArrayView<const TCHAR* const>(WeatherPlaces).ContainsByPredicate([&Daylight](const TCHAR* Place) { return Daylight.Weather == Place; }))
			{
				Find(FString::Printf(TEXT("Opening '%s' takes the weather of '%s', which isn't one of Time's places."), *Daylight.Opening, *Daylight.Weather));
			}
		}
		for (const FLightScheduleLuminaire& Entry : Schedule.Luminaires)
		{
			FPlacement Placement;
			const FString Why = Check(Stage, Entry, Placement);
			if (!Why.IsEmpty())
			{
				Find(FString::Printf(TEXT("Luminaire '%s' isn't placed: %s."), *Entry.Id, *Why));
				continue;
			}
			const FLightLuminaire* Luminaire = FindLuminaire(Placement.Type);
			if (!Luminaire->bLit)
			{
				Find(FString::Printf(TEXT("Luminaire '%s' stands unlit: %s."), *Entry.Id, *Luminaire->WhyUnlit));
			}
			Place(Stage, Entry, Placement);
		}
	}
}
