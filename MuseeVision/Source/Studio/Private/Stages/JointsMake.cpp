#include "Stages/JointsMake.h"

#include "FormMake.h"
#include "Levels/StageLevel.h"
#include "SiteJoint.h"
#include "SitePlan.h"
#include "SitePortClass.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageCast.h"
#include "Stages/StageFabric.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/** The joints' level's actor, and the Level Instance that places it in the museum. */
		const TCHAR* const JointsName = TEXT("Joints");

		/** The key the joints' made pieces go under: the site's, since Site draws them. */
		const TCHAR* const SiteKey = TEXT("site");

		const FSitePort* FindPort(const TMap<FMuseeKey, TArray<FSitePort>>& Ports, const FMuseeKey& Key)
		{
			const TArray<FSitePort>* OfStage = Ports.Find(Musee::Site::StageOfPort(Key));
			return OfStage ? OfStage->FindByPredicate([&Key](const FSitePort& Port) { return Port.Key == Key; }) : nullptr;
		}

		/** One mating's joint, appended to OutDrawings; false with a finding when it can't be drawn. */
		bool DrawJoint(const FSiteMating& Mating, const FSiteLayout& Layout, const TMap<FMuseeKey, TArray<FSitePort>>& Ports,
			const FSiteJointRecord& Record, const FStageMakeOptions& Options, TArray<FFormDrawing>& OutDrawings, TArray<FMuseeFinding>& OutFindings)
		{
			const FSitePort* A = FindPort(Ports, Mating.A);
			const FSitePort* B = FindPort(Ports, Mating.B);
			const FVector* AtA = Layout.TranslationsMeters.Find(Musee::Site::StageOfPort(Mating.A));
			const FVector* AtB = Layout.TranslationsMeters.Find(Musee::Site::StageOfPort(Mating.B));
			FString Why = !A || !B || !AtA || !AtB ? TEXT("its ports' stages are not both placed") : FString();
			if (Why.IsEmpty())
			{
				const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(A->Class, Options.PortClassesRecord);
				FSiteJoint Joint;
				if (Class.HasError())
				{
					Why = Class.GetError();
				}
				else if (Musee::Site::Joint(Musee::Site::Placed(*A, *AtA), Musee::Site::Placed(*B, *AtB), Mating.PassageMeters, Class.GetValue(),
					Record, Joint, Why))
				{
					OutDrawings.Append(MoveTemp(Joint.Drawings));
					return true;
				}
			}
			OutFindings.Add(MakeFinding(TEXT("Site.PortsMate"), TEXT("Site"), FString::Printf(TEXT("the joint of %s and %s is not made: %s"),
				*Mating.A.ToString(), *Mating.B.ToString(), *Why), Mating.A.ToString()));
			return false;
		}
	}

	bool MakeJoints(const FSitePlan& Plan, const FSiteLayout& Layout, const TMap<FMuseeKey, TArray<FSitePort>>& Ports,
		const FStageMakeOptions& Options, FMakeReport& Report)
	{
		const TValueOrError<FSiteJointRecord, FString> Record = Musee::Site::ReadJoints(Options.JointsRecord);
		if (Record.HasError())
		{
			Report.Findings.Add(MakeFinding(TEXT("Site.PlanResolves"), TEXT("Site"), Record.GetError()));
			return false;
		}
		FStageFabric Fabric;
		Fabric.Description.Stage = FMuseeKey(SiteKey);
		for (const FSiteMating& Mating : Plan.Matings)
		{
			if (Layout.MatedPorts.Contains(Mating.A) && Layout.MatedPorts.Contains(Mating.B))
			{
				DrawJoint(Mating, Layout, Ports, Record.GetValue(), Options, Fabric.Description.Pieces, Report.Findings);
			}
		}
		if (Fabric.Description.Pieces.IsEmpty())
		{
			return false;
		}
		Musee::FormMaking::Make(Fabric.Description.Pieces, Fabric.Pieces, Report.Findings);
		const FStageCast Cast = CastPieces(Fabric, Options, FString(), [&Options](FName Piece) { return Options.Paths.JointMesh(AssetToken(Piece)); },
			Report);
		FStageLevel Level;
		Level.StageName = JointsName;
		Level.PackageName = Options.Paths.JointsLevel();
		Level.Placements = Cast.Placements;
		Level.DependencyHash = JointsName;
		const FMadePackage Made = WriteStageLevel(Level);
		Report.Packages.Add(Made);
		return Made.Outcome != EMadeOutcome::Failed && !Level.Placements.IsEmpty();
	}
}
