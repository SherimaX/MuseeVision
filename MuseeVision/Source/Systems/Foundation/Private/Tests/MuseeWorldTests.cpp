#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "MuseeFoundationTests.h"
#include "MuseeRegistry.h"
#include "MuseeSignals.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	/** A game world for one test, destroyed with the scope. */
	class FScopedWorld
	{
	public:
		FScopedWorld()
			: World(UWorld::CreateWorld(EWorldType::Game, false))
		{
		}

		~FScopedWorld()
		{
			World->DestroyWorld(false);
		}

		UWorld* operator->() const { return World; }

	private:
		UWorld* World;
	};

	FMuseeChannel ChannelValue(const FMuseeKey& Key, float Value)
	{
		FMuseeChannel Channel;
		Channel.Key = Key;
		Channel.Value = Value;
		return Channel;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRegistryTest, "Musee.Foundation.Registry", Musee::Tests::Flags)

bool FMuseeFoundationRegistryTest::RunTest(const FString& Parameters)
{
	const Musee::Tests::FScopedWorld World;
	UMuseeRegistry* Registry = World->GetSubsystem<UMuseeRegistry>();
	if (!TestNotNull(TEXT("Every world has a registry"), Registry))
	{
		return false;
	}
	USceneComponent* Basin = NewObject<USceneComponent>(GetTransientPackage());
	USceneComponent* Other = NewObject<USceneComponent>(GetTransientPackage());
	const FMuseeKey Key(TEXTVIEW("salon.pond.basin"));
	int32 Announced = 0;
	Registry->OnRegistered().AddLambda([&Announced](const FMuseeKey&, USceneComponent*) { ++Announced; });

	TestTrue(TEXT("A component registers its key"), Registry->Register(Key, Basin));
	TestTrue(TEXT("The key resolves to it"), Registry->Find(Key) == Basin);
	TestTrue(TEXT("Registering again does nothing"), Registry->Register(Key, Basin));
	TestEqual(TEXT("And is announced once"), Announced, 1);

	AddExpectedErrorPlain(TEXT("is already carried by"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("One key names one live component"), Registry->Register(Key, Other));
	Registry->Unregister(Key, Other);
	TestTrue(TEXT("Only the holder frees its key"), Registry->Find(Key) == Basin);
	Registry->Unregister(Key, Basin);
	TestNull(TEXT("A freed key resolves to nothing"), Registry->Find(Key));

	AddExpectedErrorPlain(TEXT("takes a key and the component"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("The none key is refused"), Registry->Register(FMuseeKey(), Basin));
	TestFalse(TEXT("A null component is refused"), Registry->Register(Key, nullptr));

	TestTrue(TEXT("A key registered anew"), Registry->Register(Key, Other));
	Other->MarkAsGarbage();
	TestNull(TEXT("A destroyed component resolves to nothing"), Registry->Find(Key));
	TestTrue(TEXT("And frees its key"), Registry->Register(Key, Basin));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationSignalsTest, "Musee.Foundation.Signals", Musee::Tests::Flags)

bool FMuseeFoundationSignalsTest::RunTest(const FString& Parameters)
{
	const Musee::Tests::FScopedWorld World;
	UMuseeSignals* Signals = World->GetSubsystem<UMuseeSignals>();
	if (!TestNotNull(TEXT("Every world has signals"), Signals))
	{
		return false;
	}

	FMuseeContact Footfall;
	Footfall.Body = FMuseeKey(TEXTVIEW("visitor.body"));
	Footfall.Substance = FMuseeKey(TEXTVIEW("travertine"));
	Footfall.SpeedMetersPerSecond = 0.35f;
	TArray<FMuseeContact> Heard;
	Signals->OnContact().AddLambda([&Heard](const FMuseeContact& Contact) { Heard.Add(Contact); });
	Signals->Publish(Footfall);
	TestTrue(TEXT("A contact is heard as published"), Heard.Num() == 1 && Heard[0].Substance == Footfall.Substance && Heard[0].Speed() == Footfall.Speed());

	FMuseeActuation Lift;
	Lift.Mechanism = FMuseeKey(TEXTVIEW("rotunda.cylinder"));
	Lift.SpeedMetersPerSecond = 0.05f;
	Lift.LoadKilograms = 1200.0f;
	TOptional<FMuseeActuation> HeardLift;
	Signals->OnActuation().AddLambda([&HeardLift](const FMuseeActuation& Actuation) { HeardLift = Actuation; });
	Signals->Publish(Lift);
	TestTrue(TEXT("An actuation is heard as published"), HeardLift.IsSet() && HeardLift->Mechanism == Lift.Mechanism && HeardLift->Load() == Lift.Load());

	const FMuseeKey Shade(TEXTVIEW("salon.lantern.shade"));
	const FMuseeKey Lock(TEXTVIEW("cube.lock"));
	TestFalse(TEXT("A point nobody published has no value"), Signals->Read(Shade).IsSet());
	int32 Changes = 0;
	FDelegateHandle Handle = Signals->Subscribe(Shade, UMuseeSignals::FChannelDelegate::CreateLambda([&Changes](const FMuseeChannel&) { ++Changes; }));
	Signals->Publish(Musee::Tests::ChannelValue(Shade, 0.5f));
	TestEqual(TEXT("A new value is read"), Signals->Read(Shade).Get(-1.0f), 0.5f);
	Signals->Publish(Musee::Tests::ChannelValue(Shade, 0.5f));
	TestEqual(TEXT("Only a change of value is heard"), Changes, 1);
	Signals->Publish(Musee::Tests::ChannelValue(Shade, 1.0f));
	TestEqual(TEXT("Each change is heard"), Changes, 2);
	Signals->Unsubscribe(Shade, Handle);
	Signals->Publish(Musee::Tests::ChannelValue(Shade, 0.0f));
	TestEqual(TEXT("An unsubscribed listener hears nothing"), Changes, 2);
	TestEqual(TEXT("The point keeps its latest value"), Signals->Read(Shade).Get(-1.0f), 0.0f);

	// A listener may leave its point and join another while it hears a change.
	int32 LockHeard = 0;
	FDelegateHandle SelfHandle;
	SelfHandle = Signals->Subscribe(Shade, UMuseeSignals::FChannelDelegate::CreateLambda([&](const FMuseeChannel&)
	{
		Signals->Unsubscribe(Shade, SelfHandle);
		Signals->Subscribe(Lock, UMuseeSignals::FChannelDelegate::CreateLambda([&LockHeard](const FMuseeChannel&) { ++LockHeard; }));
	}));
	Signals->Publish(Musee::Tests::ChannelValue(Shade, 0.25f));
	Signals->Publish(Musee::Tests::ChannelValue(Lock, 1.0f));
	TestEqual(TEXT("Listeners change their points while they hear"), LockHeard, 1);

	AddExpectedErrorPlain(TEXT("without a key"), EAutomationExpectedErrorFlags::Contains, 1);
	Signals->Publish(Musee::Tests::ChannelValue(FMuseeKey(), 1.0f));
	return true;
}

#endif
