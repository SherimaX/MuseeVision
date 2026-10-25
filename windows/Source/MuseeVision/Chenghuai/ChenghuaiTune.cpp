#include "Chenghuai/ChenghuaiTune.h"

#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace ChenghuaiTune
{
	namespace
	{
		FCriticalSection Lock;   // the rockery builds on worker threads
		TMap<FString, double> Values;
		TMap<FString, double> Asked;   // key → default
		int32 Gen = 0;
	}

	FString Path() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/chenghuai.json")); }

	int32 Generation() { return Gen; }

	void Load()
	{
		FScopeLock Guard(&Lock);
		Values.Reset();
		Asked.Reset();
		++Gen;
		if (!FParse::Param(FCommandLine::Get(), TEXT("Live"))) { return; }
		FString Text;
		TSharedPtr<FJsonObject> J;
		if (!FFileHelper::LoadFileToString(Text, *Path()) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J) { return; }
		for (const auto& KV : J->Values)
		{
			double V;
			if (KV.Value.IsValid() && KV.Value->TryGetNumber(V)) { Values.Add(FString(*KV.Key), V); }
		}
	}

	double Num(const FString& Key, double Default)
	{
		FScopeLock Guard(&Lock);
		Asked.FindOrAdd(Key, Default);
		const double* V = Values.Find(Key);
		return V ? *V : Default;
	}

	TArray<FString> WriteKeys()
	{
		FScopeLock Guard(&Lock);
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		TArray<FString> Keys;
		Asked.GetKeys(Keys);
		Keys.Sort();
		for (const FString& K : Keys)
		{
			TSharedRef<FJsonObject> E = MakeShared<FJsonObject>();
			E->SetNumberField(TEXT("default"), Asked[K]);
			if (const double* V = Values.Find(K)) { E->SetNumberField(TEXT("value"), *V); }
			Out->SetObjectField(K, E);
		}
		FString Text;
		FJsonSerializer::Serialize(Out, TJsonWriterFactory<>::Create(&Text));
		FFileHelper::SaveStringToFile(Text, *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/chenghuai_keys.json")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		TArray<FString> Unused;
		for (const auto& KV : Values)
		{
			if (!Asked.Contains(KV.Key)) { Unused.Add(KV.Key); }
		}
		return Unused;
	}
}
