#include "SiteDescription.h"

#include "SiteFindings.h"

namespace
{
	/** Whether a key lies under the stage's own scope. */
	bool IsUnder(const FMuseeKey& Key, const FMuseeKey& Stage)
	{
		const FString Text = Key.ToString();
		return Text.StartsWith(Stage.ToString() + TEXT("."));
	}

	bool IsBody(const FSiteDescription& Description, const FMuseeKey& Key)
	{
		return Description.Anchors.ContainsByPredicate([&Key](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == FName(TEXT("Body")); });
	}

	/** Whether following a body's parents from Start comes back to it. */
	bool RidesItself(const TMap<FMuseeKey, FMuseeKey>& Parents, const FMuseeKey& Start)
	{
		FMuseeKey At = Start;
		for (int32 Step = 0; Step <= Parents.Num(); ++Step)
		{
			const FMuseeKey* Parent = Parents.Find(At);
			if (!Parent)
			{
				return false;
			}
			if (*Parent == Start)
			{
				return true;
			}
			At = *Parent;
		}
		return true;
	}
}

TArray<FMuseeFinding> Musee::Site::CheckDescription(const FSiteDescription& Description)
{
	TArray<FMuseeFinding> Findings;
	const FName Drawer(*Description.Stage.ToString());
	auto Break = [&](const FString& Summary, const FMuseeKey& Subject = FMuseeKey())
	{
		Findings.Add(SiteFinding(TEXT("Site.Described"), Drawer, Summary, TEXT("breaks of the stage contract"), 1.0, 0.0, 0.0, Subject));
	};
	if (Description.Stage.IsNone())
	{
		Break(TEXT("the description names no stage"));
		return Findings;
	}
	auto CheckKey = [&](const FMuseeKey& Key, const TCHAR* What)
	{
		if (!IsUnder(Key, Description.Stage))
		{
			Break(FString::Printf(TEXT("%s %s is not scoped under the stage"), What, *Key.ToString()), Key);
		}
	};
	for (const FSitePort& Port : Description.Ports)
	{
		CheckKey(Port.Key, TEXT("port"));
		if (Port.Class.IsNone())
		{
			Break(FString::Printf(TEXT("port %s names no class"), *Port.Key.ToString()), Port.Key);
		}
		if (FMath::Abs(Port.Facing().Z) > 0.5 && !Port.bHasWell)
		{
			Break(FString::Printf(TEXT("vertical port %s declares no well"), *Port.Key.ToString()), Port.Key);
		}
	}
	for (const FMuseeStageAnchor& Anchor : Description.Anchors)
	{
		CheckKey(Anchor.Key, TEXT("anchor"));
	}
	for (const TPair<FMuseeKey, FMuseeKey>& Riding : Description.BodyParents)
	{
		if (!IsBody(Description, Riding.Key) || !IsBody(Description, Riding.Value))
		{
			Break(FString::Printf(TEXT("body %s rides on %s, and both must be Body anchors"), *Riding.Key.ToString(), *Riding.Value.ToString()), Riding.Key);
		}
		else if (RidesItself(Description.BodyParents, Riding.Key))
		{
			Break(FString::Printf(TEXT("body %s rides round a loop back on itself"), *Riding.Key.ToString()), Riding.Key);
		}
	}
	for (const FFormOpening& Opening : Description.Openings)
	{
		CheckKey(Opening.Key, TEXT("opening"));
	}
	TSet<FName> Names;
	for (const FFormDrawing& Piece : Description.Pieces)
	{
		bool bSeen = false;
		Names.Add(Piece.Name, &bSeen);
		if (bSeen)
		{
			Break(FString::Printf(TEXT("two pieces are named %s"), *Piece.Name.ToString()));
		}
	}
	for (const FFormEnclosure& Enclosure : Description.Enclosures)
	{
		CheckKey(Enclosure.Key, TEXT("enclosure"));
		for (const FName Piece : Enclosure.Pieces)
		{
			if (!Names.Contains(Piece))
			{
				Break(FString::Printf(TEXT("enclosure %s is bounded by %s, which the stage doesn't draw"), *Enclosure.Key.ToString(),
					*Piece.ToString()), Enclosure.Key);
			}
		}
	}
	return Findings;
}
