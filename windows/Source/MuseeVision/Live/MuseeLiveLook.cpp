#include "Live/MuseeLiveLook.h"
#include "MuseeVision.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UObjectIterator.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace
{
TSharedPtr<FJsonObject> ReadJson(const FString& Path, bool& bOk)   // bOk false: the file is there but isn't JSON (yet)
{
	bOk = true;
	if (!FPaths::FileExists(Path)) { return nullptr; }
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J)
	{
		bOk = false;
		return nullptr;
	}
	return J;
}

void SaveAtomic(const FString& Text, const FString& Path)
{
	const FString Tmp = Path + TEXT(".tmp");
	if (FFileHelper::SaveStringToFile(Text, *Tmp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		IFileManager::Get().Move(*Path, *Tmp, true);
	}
}

FString ToJsonText(const TSharedRef<FJsonObject>& O)
{
	FString Out;
	FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Out));
	return Out;
}

template <class T> FString Key(const T& K) { return FString(*K); }   // JSON keys are shared strings in 5.8

/** A property by a forgiving name: "bloom_intensity", "BloomIntensity" and "bloomintensity" all find BloomIntensity. */
FProperty* FindLoose(const UStruct* S, const FString& Name)
{
	const FString K = Name.Replace(TEXT("_"), TEXT("")).ToLower();
	for (TFieldIterator<FProperty> It(S); It; ++It)
	{
		const FString N = It->GetName().ToLower();
		if (N.StartsWith(TEXT("boverride_"))) { continue; }
		const FString N2 = N.Replace(TEXT("_"), TEXT(""));
		if (N2 == K || (N2.StartsWith(TEXT("b")) && N2.Mid(1) == K)) { return *It; }
	}
	return nullptr;
}

/** A JSON value as Unreal's text form for the property: 0.3, True, AEM_Manual, (R=1,G=0.9,B=0.8,A=1) … */
FString AsText(const FProperty* P, const TSharedPtr<FJsonValue>& V)
{
	switch (V->Type)
	{
	case EJson::Number:
	{
		const double D = V->AsNumber();
		return D == FMath::RoundToDouble(D) && FMath::Abs(D) < 1e9 ? FString::Printf(TEXT("%lld"), (long long)D) : FString::SanitizeFloat(D);
	}
	case EJson::Boolean: return V->AsBool() ? TEXT("True") : TEXT("False");
	case EJson::String: return V->AsString();
	case EJson::Array:
	{
		TArray<double> A;
		for (const TSharedPtr<FJsonValue>& E : V->AsArray()) { A.Add(E->AsNumber()); }
		while (A.Num() < 4) { A.Add(A.Num() == 3 ? 1.0 : 0.0); }
		const FStructProperty* SP = CastField<FStructProperty>(P);
		const FName S = SP ? SP->Struct->GetFName() : NAME_None;
		if (S == TEXT("Rotator")) { return FString::Printf(TEXT("(Pitch=%g,Yaw=%g,Roll=%g)"), A[0], A[1], A[2]); }
		if (S == TEXT("Vector") || S == TEXT("Vector3f")) { return FString::Printf(TEXT("(X=%g,Y=%g,Z=%g)"), A[0], A[1], A[2]); }
		if (S == TEXT("Vector2D")) { return FString::Printf(TEXT("(X=%g,Y=%g)"), A[0], A[1]); }
		if (S == TEXT("Vector4") || S == TEXT("Vector4f")) { return FString::Printf(TEXT("(X=%g,Y=%g,Z=%g,W=%g)"), A[0], A[1], A[2], A[3]); }
		return FString::Printf(TEXT("(R=%g,G=%g,B=%g,A=%g)"), A[0], A[1], A[2], A[3]);
	}
	default: return FString();
	}
}

bool SetText(FProperty* P, void* Container, UObject* Owner, const FString& Text)
{
	return P->ImportText_Direct(*Text, P->ContainerPtrToValuePtr<void>(Container), Owner, PPF_None) != nullptr;
}

FString GetText(const FProperty* P, const void* Container, UObject* Owner)
{
	FString Out;
	P->ExportTextItem_Direct(Out, P->ContainerPtrToValuePtr<void>(Container), nullptr, Owner, PPF_None);
	return Out;
}

FString Label(const AActor* A)
{
#if WITH_EDITOR
	return A->GetActorLabel();
#else
	return A->GetName();
#endif
}

/** A file's exact write time and size ("missing" when it isn't there). Unreal's own file times drop the milliseconds,
 *  so two same-size edits within a second would look like one. */
FString FileKey(const FString& Path)
{
#if PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA A;
	if (!GetFileAttributesExW(*Path, GetFileExInfoStandard, &A)) { return TEXT("missing"); }
	return FString::Printf(TEXT("%u.%u:%u.%u"), A.ftLastWriteTime.dwHighDateTime, A.ftLastWriteTime.dwLowDateTime, A.nFileSizeHigh, A.nFileSizeLow);
#else
	const FFileStatData S = IFileManager::Get().GetStatData(*Path);
	return S.bIsValid ? FString::Printf(TEXT("%lld:%lld"), S.ModificationTime.GetTicks(), S.FileSize) : TEXT("missing");
#endif
}

FString PackageOf(const UObject* O) { return FPackageName::ObjectPathToPackageName(O->GetPathName()); }

FLinearColor Color(const TArray<TSharedPtr<FJsonValue>>& A)
{
	return FLinearColor(A.Num() > 0 ? A[0]->AsNumber() : 0, A.Num() > 1 ? A[1]->AsNumber() : 0, A.Num() > 2 ? A[2]->AsNumber() : 0, A.Num() > 3 ? A[3]->AsNumber() : 1);
}
}

bool UMuseeLiveLook::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("Live"))) { return false; }
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

void UMuseeLiveLook::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	LiveDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live"));
	IFileManager::Get().MakeDirectory(*LiveDir, true);
	bActive = true;
	WriteMaterials();   // before any live copy is swapped in: the instances as cooked
	NextPoll = 0.0;     // the first Tick applies look.json
	UE_LOG(LogMusee, Display, TEXT("live: watching %s"), *(LiveDir / TEXT("look.json")));
}

void UMuseeLiveLook::Tick(float DeltaTime)
{
	if (!bActive) { return; }
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPoll) { return; }
	NextPoll = Now + 0.1;
	const FString K = FileKey(LiveDir / TEXT("look.json"));   // a missing file is a state too: deleting it is a change
	if (K == LookKey) { return; }
	LookKey = K;
	const double T0 = FPlatformTime::Seconds();
	Errors.Reset();
	ApplyLook();
	LastMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	++Applied;
	UE_LOG(LogMusee, Display, TEXT("live: look applied in %.1f ms%s"), LastMs, Errors.Num() ? *FString::Printf(TEXT(" (%d errors)"), Errors.Num()) : TEXT(""));
	for (const FString& E : Errors) { UE_LOG(LogMusee, Warning, TEXT("live: %s"), *E); }
	WriteState();
}

void UMuseeLiveLook::ApplyLook()
{
	bool bOk;
	TSharedPtr<FJsonObject> J = ReadJson(LiveDir / TEXT("look.json"), bOk);
	if (!bOk)
	{
		Errors.Add(TEXT("look.json is not valid JSON (nothing changed)"));
		LookKey.Reset();   // try again on the next poll: the writer may be mid-save
		return;
	}
	if (!J) { J = MakeShared<FJsonObject>(); }   // no file: everything as the level has it
	const TSharedPtr<FJsonObject>* Sec;

	// post-process: an unbound volume over every other one; only the settings named are overridden
	if (!LookVolume)
	{
		LookVolume = GetWorld()->SpawnActorDeferred<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity);
		LookVolume->bUnbound = true;
		LookVolume->Priority = 1000.f;   // over the museum's volumes (10) and Chenghuai's exposure (6)
		LookVolume->BlendWeight = 1.f;
		LookVolume->FinishSpawning(FTransform::Identity);
	}
	LookVolume->Settings = FPostProcessSettings();
	if (J->TryGetObjectField(TEXT("post"), Sec))
	{
		for (const auto& KV : (*Sec)->Values)
		{
			UStruct* S = FPostProcessSettings::StaticStruct();
			FProperty* P = FindLoose(S, Key(KV.Key));
			if (!P || !SetText(P, &LookVolume->Settings, nullptr, AsText(P, KV.Value)))
			{
				Errors.Add(FString::Printf(TEXT("post.%s: %s"), *Key(KV.Key), P ? TEXT("value not accepted") : TEXT("no such post-process setting")));
				continue;
			}
			if (FBoolProperty* Ov = CastField<FBoolProperty>(S->FindPropertyByName(FName(*(TEXT("bOverride_") + P->GetName())))))
			{
				Ov->SetPropertyValue_InContainer(&LookVolume->Settings, true);
			}
		}
	}

	// console variables (Lumen, ray tracing, DLSS, …); one taken out of look.json goes back to what it was
	TSet<FString> Seen;
	if (J->TryGetObjectField(TEXT("cvars"), Sec))
	{
		for (const auto& KV : (*Sec)->Values)
		{
			const FString Name = Key(KV.Key);
			IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(*Name);
			if (!V) { Errors.Add(FString::Printf(TEXT("cvars.%s: no such console variable"), *Name)); continue; }
			if (!CVarDefaults.Contains(Name)) { CVarDefaults.Add(Name, V->GetString()); }
			V->Set(*AsText(nullptr, KV.Value), ECVF_SetByConsole);
			Seen.Add(Name);
		}
	}
	for (auto It = CVarDefaults.CreateIterator(); It; ++It)
	{
		if (Seen.Contains(It.Key())) { continue; }
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(*It.Key())) { V->Set(*It.Value(), ECVF_SetByConsole); }
		It.RemoveCurrent();
	}

	// lights by label or name pattern ("Rotunda*", "*Spot*"): any property of the light component. Lights a structure
	// drives itself (Chenghuai's rect lights follow the daylight) take the structure's value again on its next change
	for (const FPropDefault& D : PropDefaults)
	{
		if (UActorComponent* C = Cast<UActorComponent>(D.Obj.Get()))
		{
			if (FProperty* P = C->GetClass()->FindPropertyByName(FName(*D.Prop)))
			{
				SetText(P, C, C, D.Text);
				C->MarkRenderStateDirty();
			}
		}
	}
	PropDefaults.Reset();
	if (J->TryGetObjectField(TEXT("lights"), Sec))
	{
		for (const auto& KV : (*Sec)->Values)
		{
			const FString Pattern = Key(KV.Key);
			const TSharedPtr<FJsonObject> Props = KV.Value->AsObject();
			int32 Hits = 0;
			for (TActorIterator<AActor> It(GetWorld()); It && Props; ++It)
			{
				if (!Label(*It).MatchesWildcard(Pattern) && !It->GetName().MatchesWildcard(Pattern)) { continue; }
				TInlineComponentArray<ULightComponentBase*> Lights(*It);
				for (ULightComponentBase* LC : Lights)
				{
					++Hits;
					for (const auto& F : Props->Values)
					{
						FProperty* P = FindLoose(LC->GetClass(), Key(F.Key));
						if (!P) { Errors.Add(FString::Printf(TEXT("lights.%s.%s: %s has no such property"), *Pattern, *Key(F.Key), *LC->GetClass()->GetName())); continue; }
						PropDefaults.Add({LC, P->GetName(), GetText(P, LC, LC)});
						if (!SetText(P, LC, LC, AsText(P, F.Value))) { Errors.Add(FString::Printf(TEXT("lights.%s.%s: value not accepted"), *Pattern, *Key(F.Key))); }
					}
					LC->MarkRenderStateDirty();
				}
			}
			if (!Hits) { Errors.Add(FString::Printf(TEXT("lights.%s: no light matches (state.json lists them)"), *Pattern)); }
		}
	}

	// material parameters: each instance named gets a live copy with those parameters, swapped in wherever it is used
	TSet<FString> Want;
	if (J->TryGetObjectField(TEXT("materials"), Sec))
	{
		for (const auto& KV : (*Sec)->Values)
		{
			FString Path;
			UMaterialInterface* MI = FindMaterial(Key(KV.Key), Path);
			if (!MI) { Errors.Add(FString::Printf(TEXT("materials.%s: no such material in the scene (materials.json lists them)"), *Key(KV.Key))); continue; }
			UMaterialInstanceDynamic* D = Mids.FindRef(Path);
			if (!Want.Contains(Path))
			{
				Want.Add(Path);
				if (!D) { D = UMaterialInstanceDynamic::Create(MI, this); Mids.Add(Path, D); }
				else { D->ClearParameterValues(); }
			}
			TArray<FMaterialParameterInfo> Sc, Ve, Tx;
			TArray<FGuid> Ids;
			MI->GetAllScalarParameterInfo(Sc, Ids);
			MI->GetAllVectorParameterInfo(Ve, Ids);
			MI->GetAllTextureParameterInfo(Tx, Ids);
			auto Has = [](const TArray<FMaterialParameterInfo>& A, const FString& N) { return A.ContainsByPredicate([&](const FMaterialParameterInfo& I) { return I.Name.ToString() == N; }); };
			const TSharedPtr<FJsonObject> Params = KV.Value->AsObject();
			if (!Params) { continue; }
			for (const auto& P : Params->Values)
			{
				const FString PName = Key(P.Key);
				const FName PN(*PName);
				if (P.Value->Type == EJson::Number && Has(Sc, PName)) { D->SetScalarParameterValue(PN, P.Value->AsNumber()); }
				else if (P.Value->Type == EJson::Array && Has(Ve, PName)) { D->SetVectorParameterValue(PN, Color(P.Value->AsArray())); }
				else if (P.Value->Type == EJson::String && Has(Tx, PName))
				{
					UTexture* Tex = LoadObject<UTexture>(nullptr, *P.Value->AsString(), nullptr, LOAD_NoWarn | LOAD_Quiet);
					if (Tex) { D->SetTextureParameterValue(PN, Tex); }
					else { Errors.Add(FString::Printf(TEXT("materials.%s.%s: no texture %s"), *Key(KV.Key), *PName, *P.Value->AsString())); }
				}
				else { Errors.Add(FString::Printf(TEXT("materials.%s.%s: no such parameter of that type (static switches need an editor build; materials.json lists the parameters)"), *Key(KV.Key), *PName)); }
			}
		}
	}
	for (auto It = Mids.CreateIterator(); It; ++It)   // taken out of look.json: SwapLiveMaterials puts the instance back
	{
		if (!Want.Contains(It.Key())) { It.RemoveCurrent(); }
	}
	SwapLiveMaterials();
}

UMaterialInterface* UMuseeLiveLook::FindMaterial(const FString& Name, FString& OutPath) const
{
	if (Name.StartsWith(TEXT("/")))
	{
		const FString Obj = Name.Contains(TEXT(".")) ? Name : Name + TEXT(".") + FPackageName::GetShortName(Name);
		UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *Obj, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (M) { OutPath = PackageOf(M); }
		return M;
	}
	// by name: the instance as the scene uses it (one of our live copies stands for its parent)
	const UWorld* World = GetWorld();
	for (TObjectIterator<UMeshComponent> It; It; ++It)
	{
		if (It->GetWorld() != World) { continue; }
		for (int32 i = 0; i < It->GetNumMaterials(); ++i)
		{
			UMaterialInterface* M = It->GetMaterial(i);
			if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M); D && D->GetOuter() == this) { M = D->Parent; }
			if (M && M->GetName() == Name) { OutPath = PackageOf(M); return M; }
		}
	}
	return nullptr;
}

void UMuseeLiveLook::SwapLiveMaterials()
{
	TMap<UMaterialInterface*, UMaterialInterface*> Live;   // instance → its live copy
	for (const auto& KV : Mids)
	{
		if (KV.Value) { Live.Add(KV.Value->Parent, KV.Value); }
	}
	const UWorld* World = GetWorld();
	for (TObjectIterator<UMeshComponent> It; It; ++It)
	{
		UMeshComponent* C = *It;
		if (C->GetWorld() != World) { continue; }
		for (int32 i = 0; i < C->GetNumMaterials(); ++i)
		{
			UMaterialInterface* M = C->GetMaterial(i);
			if (!M) { continue; }
			UMaterialInterface* To = nullptr;
			if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M); D && D->GetOuter() == this)
			{
				To = Live.FindRef(D->Parent);
				if (!To) { To = D->Parent; }   // taken out of look.json: back to the instance
			}
			else { To = Live.FindRef(M); }
			if (To && To != M) { C->SetMaterial(i, To); }
		}
	}
}

void UMuseeLiveLook::WriteState()
{
	TSharedRef<FJsonObject> S = MakeShared<FJsonObject>();
	S->SetNumberField(TEXT("applied"), Applied);
	S->SetNumberField(TEXT("last_ms"), FMath::RoundToDouble(LastMs * 10.0) / 10.0);
	TArray<TSharedPtr<FJsonValue>> Errs;
	for (const FString& E : Errors) { Errs.Add(MakeShared<FJsonValueString>(E)); }
	S->SetArrayField(TEXT("errors"), Errs);
	TSharedRef<FJsonObject> Lights = MakeShared<FJsonObject>();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const ULightComponentBase* LC = It->FindComponentByClass<ULightComponentBase>();
		if (!LC) { continue; }
		TSharedRef<FJsonObject> I = MakeShared<FJsonObject>();
		I->SetStringField(TEXT("class"), LC->GetClass()->GetName());
		I->SetNumberField(TEXT("intensity"), LC->Intensity);
		if (const ULightComponent* L = Cast<ULightComponent>(LC)) { I->SetNumberField(TEXT("temperature"), L->Temperature); }
		Lights->SetObjectField(Label(*It), I);
	}
	S->SetObjectField(TEXT("lights"), Lights);
	SaveAtomic(ToJsonText(S), LiveDir / TEXT("state.json"));
}

void UMuseeLiveLook::WriteMaterials()
{
	TSet<UMaterialInterface*> All;
	const UWorld* World = GetWorld();
	for (TObjectIterator<UMeshComponent> It; It; ++It)
	{
		if (It->GetWorld() != World) { continue; }
		for (int32 i = 0; i < It->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* M = It->GetMaterial(i)) { All.Add(M); }
		}
	}
	TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
	for (UMaterialInterface* M : All)
	{
		UMaterialInstance* MI = Cast<UMaterialInstance>(M);
		if (!MI || MI->IsA<UMaterialInstanceDynamic>()) { continue; }
		TSharedRef<FJsonObject> E = MakeShared<FJsonObject>();
		E->SetStringField(TEXT("master"), MI->GetMaterial() ? MI->GetMaterial()->GetName() : TEXT(""));
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		TSharedRef<FJsonObject> Sc = MakeShared<FJsonObject>(), Ve = MakeShared<FJsonObject>(), Tx = MakeShared<FJsonObject>();
		MI->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& I : Infos)
		{
			float V;
			if (MI->GetScalarParameterValue(I, V)) { Sc->SetNumberField(I.Name.ToString(), V); }
		}
		MI->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& I : Infos)
		{
			FLinearColor C;
			if (MI->GetVectorParameterValue(I, C))
			{
				Ve->SetArrayField(I.Name.ToString(), {MakeShared<FJsonValueNumber>(C.R), MakeShared<FJsonValueNumber>(C.G), MakeShared<FJsonValueNumber>(C.B), MakeShared<FJsonValueNumber>(C.A)});
			}
		}
		MI->GetAllTextureParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& I : Infos)
		{
			UTexture* T = nullptr;
			if (MI->GetTextureParameterValue(I, T) && T) { Tx->SetStringField(I.Name.ToString(), T->GetPathName()); }
		}
		E->SetObjectField(TEXT("scalars"), Sc);
		E->SetObjectField(TEXT("vectors"), Ve);
		E->SetObjectField(TEXT("textures"), Tx);
		Out->SetObjectField(PackageOf(MI), E);
	}
	SaveAtomic(ToJsonText(Out), LiveDir / TEXT("materials.json"));
}
