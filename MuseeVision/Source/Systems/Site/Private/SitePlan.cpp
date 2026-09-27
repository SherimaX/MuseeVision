#include "SitePlan.h"

#include "MuseeRecords.h"
#include "SiteFindings.h"

namespace Musee::Site
{
	namespace
	{
		const FSitePort* FindPort(const TMap<FMuseeKey, TArray<FSitePort>>& PortsByStage, const FMuseeKey& Port)
		{
			const TArray<FSitePort>* Ports = PortsByStage.Find(StageOfPort(Port));
			return Ports ? Ports->FindByPredicate([&Port](const FSitePort& P) { return P.Key == Port; }) : nullptr;
		}

		/** Whether two ports can meet: the same kind and class, facing each other. Findings say why not. */
		bool CheckPair(const FSitePort& A, const FSitePort& B, TArray<FMuseeFinding>& OutFindings)
		{
			const FName Drawer(*StageOfPort(B.Key).ToString());
			if (A.Kind != B.Kind || !(A.Class == B.Class))
			{
				OutFindings.Add(SiteFinding(TEXT("Site.PortsMate"), Drawer, FString::Printf(TEXT("%s and %s differ in kind or class (%s, %s)"),
					*A.Key.ToString(), *B.Key.ToString(), *A.Class.ToString(), *B.Class.ToString()), TEXT("mismatched classes"), 1.0, 0.0, 0.0, B.Key));
				return false;
			}
			const double Turn = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(-FVector::DotProduct(A.Facing(), B.Facing()), -1.0, 1.0)));
			if (Turn > FacingToleranceDegrees)
			{
				OutFindings.Add(SiteFinding(TEXT("Site.PortsMate"), Drawer, FString::Printf(TEXT("%s and %s don't face each other"),
					*A.Key.ToString(), *B.Key.ToString()), TEXT("turn from facing, degrees"), Turn, 0.0, FacingToleranceDegrees, B.Key));
				return false;
			}
			return true;
		}
	}

	TValueOrError<FSitePlan, FString> ReadPlan(const FString& Path)
	{
		return ReadRecord<FSitePlan>(Path);
	}

	FMuseeKey StageOfPort(const FMuseeKey& Port)
	{
		return Port.Parent();
	}

	FSiteLayout Solve(const FSitePlan& Plan, const TMap<FMuseeKey, TArray<FSitePort>>& PortsByStage, TArray<FMuseeFinding>& OutFindings)
	{
		FSiteLayout Layout;
		if (!PortsByStage.Contains(Plan.Anchor))
		{
			return Layout;
		}
		Layout.TranslationsMeters.Add(Plan.Anchor, FVector::ZeroVector);
		TArray<FMuseeKey> Frontier = {Plan.Anchor};
		while (!Frontier.IsEmpty())
		{
			const FMuseeKey Here = Frontier.Pop(EAllowShrinking::No);
			for (const FSiteMating& Mating : Plan.Matings)
			{
				const bool bFromA = StageOfPort(Mating.A) == Here;
				if (!bFromA && !(StageOfPort(Mating.B) == Here))
				{
					continue;
				}
				const FMuseeKey Near = bFromA ? Mating.A : Mating.B;
				const FMuseeKey Far = bFromA ? Mating.B : Mating.A;
				const FSitePort* NearPort = FindPort(PortsByStage, Near);
				const FSitePort* FarPort = FindPort(PortsByStage, Far);
				if (!NearPort || !PortsByStage.Contains(StageOfPort(Far)))
				{
					continue;
				}
				if (!FarPort)
				{
					OutFindings.Add(SiteFinding(TEXT("Site.PortsMate"), FName(*StageOfPort(Far).ToString()), FString::Printf(
						TEXT("the site plan mates %s, which its stage doesn't declare"), *Far.ToString()), TEXT("ports declared"), 0.0, 1.0, 1.0, Far));
					continue;
				}
				if (!CheckPair(*NearPort, *FarPort, OutFindings))
				{
					continue;
				}
				// The passage runs out from either face along its facing (the two face each other), and the far stage
				// stands where that puts its port.
				const FVector Placed = Layout.TranslationsMeters[Here] + NearPort->Frame.OriginMeters
					+ NearPort->Facing() * Mating.PassageMeters - FarPort->Frame.OriginMeters;
				const FMuseeKey There = StageOfPort(Far);
				Layout.MatedPorts.Add(Near);
				Layout.MatedPorts.Add(Far);
				if (const FVector* Already = Layout.TranslationsMeters.Find(There))
				{
					const double Miss = FVector::Distance(*Already, Placed);
					if (Miss > LoopToleranceMeters)
					{
						OutFindings.Add(SiteFinding(TEXT("Site.LoopsClose"), FName(*There.ToString()), FString::Printf(
							TEXT("the matings place %s twice, %.4f m apart"), *There.ToString(), Miss), TEXT("loop closure, m"), Miss, 0.0, LoopToleranceMeters));
					}
					continue;
				}
				Layout.TranslationsMeters.Add(There, Placed);
				Frontier.Add(There);
			}
		}
		if (const FVector* Stage = Layout.TranslationsMeters.Find(Plan.Arrival.Stage); Stage && Plan.Arrival.PointMeters.Num() == 3)
		{
			const TArray<double>& P = Plan.Arrival.PointMeters;
			Layout.bHasArrival = true;
			Layout.ArrivalMeters = *Stage + FVector(P[0], P[1], P[2]);
			Layout.ArrivalFacingDegrees = Plan.Arrival.FacingDegrees;
		}
		return Layout;
	}
}
