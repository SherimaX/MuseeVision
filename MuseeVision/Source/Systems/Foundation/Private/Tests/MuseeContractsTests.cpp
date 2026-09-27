#include "Engine/CollisionProfile.h"
#include "MuseeCaption.h"
#include "MuseeCollision.h"
#include "MuseeFoundationTests.h"
#include "MuseeStageDresser.h"
#include "MuseeText.h"
#include "MuseeUse.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationStageAnchorsTest, "Musee.Foundation.StageAnchors", Musee::Tests::Flags)

bool FMuseeFoundationStageAnchorsTest::RunTest(const FString& Parameters)
{
	const FMuseeKey Iris(TEXTVIEW("oval.golden-iris"));
	FMuseeStageContext Stage;
	Stage.Anchors = {
		{ Iris, TEXT("Use"), FMuseeFrame(FVector(-4.15, 0.0, 0.42)) },
		{ Iris, TEXT("Bed"), FMuseeFrame(FVector(-4.15, 0.0, 0.40)) },
		{ FMuseeKey(TEXTVIEW("oval.basin")), TEXT("Body"), FMuseeFrame(FVector::ZeroVector) },
	};
	const FMuseeStageAnchor* Use = Stage.FindAnchor(Iris, TEXT("Use"));
	const FMuseeStageAnchor* Bed = Stage.FindAnchor(Iris, TEXT("Bed"));
	TestTrue(TEXT("One key names a Use and a Bed, each found by its kind"), Use && Bed && Use->Kind == TEXT("Use") && Bed->Kind == TEXT("Bed")
		&& FMath::IsNearlyEqual(Bed->Frame.OriginMeters.Z, 0.40));
	TestNull(TEXT("A key is not found under a kind it doesn't have"), Stage.FindAnchor(Iris, TEXT("Body")));
	TestNull(TEXT("Nor a kind under a key that doesn't name it"), Stage.FindAnchor(FMuseeKey(TEXTVIEW("oval.basin")), TEXT("Bed")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationTextTest, "Musee.Foundation.Text", Musee::Tests::Flags)

bool FMuseeFoundationTextTest::RunTest(const FString& Parameters)
{
	FMuseeText Title;
	Title.Key = FMuseeKey(TEXTVIEW("art.monet-impression-sunrise.title"));
	Title.Source = TEXT("Impression, Sunrise");
	const FText Text = Title.ToText();
	TestEqual(TEXT("A text shows its source string"), Text.ToString(), Title.Source);
	TestEqual(TEXT("Under the museum's namespace"), FTextInspector::GetNamespace(Text).Get(FString()), FString(FMuseeText::Namespace));
	TestEqual(TEXT("And its own key, for its translations"), FTextInspector::GetKey(Text).Get(FString()), Title.Key.ToString());

	FMuseeCaption Caption;
	Caption.Title = Text;
	TestTrue(TEXT("A caption carries display text"), Caption.Title.EqualTo(Text) && Caption.Credit.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationUseTest, "Musee.Foundation.Use", Musee::Tests::Flags)

bool FMuseeFoundationUseTest::RunTest(const FString& Parameters)
{
	const FText Verb = FText::FromString(TEXT("Raise"));
	const FText Caption = FText::FromString(TEXT("Raise the sun clock"));
	const FMuseeUse Ready = FMuseeUse::Available(Verb, Caption);
	TestTrue(TEXT("An available use offers its verb and caption"), Ready.bAvailable && Ready.Verb.EqualTo(Verb) && Ready.Caption.EqualTo(Caption) && Ready.WhyNot.IsEmpty());

	const FText WhyNot = FText::FromString(TEXT("The cylinder is moving"));
	const FMuseeUse Waiting = FMuseeUse::Unavailable(Verb, Caption, WhyNot);
	TestTrue(TEXT("An unavailable use says why not"), !Waiting.bAvailable && Waiting.WhyNot.EqualTo(WhyNot));

	const FMuseeFrame Seat(FVector(4.0, 2.0, 0.45), FQuat(FMuseeFrame::Up(), UE_DOUBLE_PI));
	const FMuseeUse Sitting = FMuseeUse::Available(FText::FromString(TEXT("Sit")), FText::FromString(TEXT("Sit on the bench"))).Seating(Seat);
	TestTrue(TEXT("A seat gives its frame"), Sitting.bSeats && Sitting.SeatFrame.Equals(Seat, Musee::Meters(1e-9), Musee::Radians(1e-6)));
	TestFalse(TEXT("Other uses seat nobody"), Ready.bSeats);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationCollisionTest, "Musee.Foundation.Collision", Musee::Tests::Flags)

bool FMuseeFoundationCollisionTest::RunTest(const FString& Parameters)
{
	namespace Collision = Musee::Collision;
	const UCollisionProfile* Profiles = UCollisionProfile::Get();
	for (const Collision::FClass& Class : {Collision::Solid, Collision::Walkable, Collision::Body, Collision::Clearance, Collision::Barrier})
	{
		TestTrue(FString::Printf(TEXT("The config names channel %d %s"), static_cast<int32>(Class.Channel), Class.ProfileName),
			Profiles->ReturnChannelNameFromContainerIndex(Class.Channel) == Class.Profile());
		FCollisionResponseTemplate Template;
		TestTrue(FString::Printf(TEXT("The config holds profile %s"), Class.ProfileName), Profiles->GetProfileTemplate(Class.Profile(), Template));
		TestTrue(FString::Printf(TEXT("Profile %s gives its own channel"), Class.ProfileName), Template.ObjectType == Class.Channel);
	}

	FCollisionResponseTemplate Solid;
	FCollisionResponseTemplate Body;
	FCollisionResponseTemplate Clearance;
	Profiles->GetProfileTemplate(Collision::Solid.Profile(), Solid);
	Profiles->GetProfileTemplate(Collision::Body.Profile(), Body);
	Profiles->GetProfileTemplate(Collision::Clearance.Profile(), Clearance);
	TestTrue(TEXT("A piece blocks the body"), Solid.ResponseToChannels.GetResponse(Collision::Body.Channel) == ECR_Block);
	TestTrue(TEXT("A piece blocks clearance queries"), Solid.ResponseToChannels.GetResponse(Collision::Clearance.Channel) == ECR_Block);
	TestTrue(TEXT("The body blocks clearance queries"), Body.ResponseToChannels.GetResponse(Collision::Clearance.Channel) == ECR_Block);
	TestTrue(TEXT("The body stops no sight"), Body.ResponseToChannels.GetResponse(ECC_Visibility) == ECR_Ignore);
	TestTrue(TEXT("A clearance volume only senses"), Clearance.CollisionEnabled == ECollisionEnabled::QueryOnly);

	FCollisionResponseTemplate Barrier;
	Profiles->GetProfileTemplate(Collision::Barrier.Profile(), Barrier);
	TestTrue(TEXT("A barrier and the body block each other"), Barrier.ResponseToChannels.GetResponse(Collision::Body.Channel) == ECR_Block
		&& Body.ResponseToChannels.GetResponse(Collision::Barrier.Channel) == ECR_Block);
	TestTrue(TEXT("The look and the camera pass a barrier"), Barrier.ResponseToChannels.GetResponse(ECC_Visibility) == ECR_Ignore
		&& Barrier.ResponseToChannels.GetResponse(ECC_Camera) == ECR_Ignore);
	TestTrue(TEXT("A mechanism's clearance and the pieces pass a barrier"), Barrier.ResponseToChannels.GetResponse(Collision::Clearance.Channel) == ECR_Ignore
		&& Solid.ResponseToChannels.GetResponse(Collision::Barrier.Channel) == ECR_Ignore);
	TestTrue(TEXT("It senses the body"), Clearance.ResponseToChannels.GetResponse(Collision::Body.Channel) == ECR_Overlap);
	TestTrue(TEXT("And nothing else"), Clearance.ResponseToChannels.GetResponse(Collision::Solid.Channel) == ECR_Ignore
		&& Clearance.ResponseToChannels.GetResponse(ECC_WorldStatic) == ECR_Ignore);
	return true;
}

#endif
