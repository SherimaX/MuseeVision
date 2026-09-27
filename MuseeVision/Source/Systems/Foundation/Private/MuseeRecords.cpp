#include "MuseeRecords.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Math/UnitConversion.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "MuseeDependencySet.h"
#include "MuseeKey.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace Musee::Records
{
	namespace
	{
		const TCHAR* const RecordsDeclaration = TEXT("$records");
		const TCHAR* const UnitsDeclaration = TEXT("$units");
		const TCHAR* const SourcesDeclaration = TEXT("$sources");
		const TCHAR* const DeclarationMark = TEXT("$");
		const TCHAR* const UnitsMetadata = TEXT("Units");

		/** One file being read: its name in messages, the units it declares, and the first thing found wrong. */
		struct FReading
		{
			FString File;
			TMap<FString, EUnit> Units;
			FString Error;

			bool Fail(const FString& Where, const FString& What)
			{
				Error = Where.IsEmpty()
					? FString::Printf(TEXT("%s: %s."), *File, *What)
					: FString::Printf(TEXT("%s: %s: %s."), *File, *Where, *What);
				return false;
			}
		};

		FString Join(const FString& Where, const FString& Field)
		{
			return Where.IsEmpty() ? Field : Where + TEXT(".") + Field;
		}

		FString Element(const FString& Where, int32 Index)
		{
			return FString::Printf(TEXT("%s[%d]"), *Where, Index);
		}

		FString CppName(const UStruct& Type)
		{
			return FString::Printf(TEXT("%s%s"), Type.GetPrefixCPP(), *Type.GetName());
		}

		FString JsonTypeName(EJson Type)
		{
			switch (Type)
			{
			case EJson::String: return TEXT("a string");
			case EJson::Number: return TEXT("a number");
			case EJson::Boolean: return TEXT("true or false");
			case EJson::Array: return TEXT("an array");
			case EJson::Object: return TEXT("an object");
			default: return TEXT("null");
			}
		}

		bool Expect(const FJsonValue& Value, EJson Type, FReading& Reading, const FString& Where, const TCHAR* Expected)
		{
			return Value.Type == Type || Reading.Fail(Where, FString::Printf(TEXT("is %s where %s is expected"), *JsonTypeName(Value.Type), Expected));
		}

		bool IsRequired(const FProperty& Property)
		{
#if WITH_METADATA
			return Property.HasMetaData(RequiredMetadata);
#else
			return false;
#endif
		}

		bool IsKey(const FProperty& Property)
		{
			const FStructProperty* Struct = CastField<FStructProperty>(&Property);
			return Struct && Struct->Struct == FMuseeKey::StaticStruct();
		}

		bool CheckKey(const FString& Text, FReading& Reading, const FString& Where)
		{
			FString Why;
			return FMuseeKey::IsValid(Text, &Why) || Reading.Fail(Where, FString::Printf(TEXT("'%s' is not a key: %s"), *Text, *Why));
		}

		/** The engine's vectors a record may write as a plain array of numbers: [x, y, z] and [x, y]. */
		int32 VectorComponents(const UScriptStruct& Type)
		{
			if (&Type == TBaseStructure<FVector>::Get())
			{
				return 3;
			}
			return &Type == TBaseStructure<FVector2D>::Get() ? 2 : 0;
		}

		/** The names of a vector's components, as the engine's converter reads them from an object. */
		const TCHAR* const VectorFields[] = { TEXT("X"), TEXT("Y"), TEXT("Z") };

		bool CheckVector(int32 Components, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			const TArray<TSharedPtr<FJsonValue>>& Items = Value.AsArray();
			if (Items.Num() != Components)
			{
				return Reading.Fail(Where, FString::Printf(TEXT("has %d numbers where a vector of %d is written"), Items.Num(), Components));
			}
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				if (!Items[Index].IsValid() || !Expect(*Items[Index], EJson::Number, Reading, Element(Where, Index), TEXT("a number")))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateObject(const UStruct& Type, const FJsonObject& Object, FReading& Reading, const FString& Where);

		bool ValidateElement(const FProperty& Property, const FJsonValue& Value, FReading& Reading, const FString& Where);

		bool ValidateList(const FProperty& Inner, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			if (!Expect(Value, EJson::Array, Reading, Where, TEXT("an array")))
			{
				return false;
			}
			const TArray<TSharedPtr<FJsonValue>>& Items = Value.AsArray();
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				if (!Items[Index].IsValid() || Items[Index]->IsNull())
				{
					return Reading.Fail(Element(Where, Index), TEXT("is null; an array holds values"));
				}
				if (!ValidateElement(Inner, *Items[Index], Reading, Element(Where, Index)))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateMap(const FMapProperty& Map, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			if (!Expect(Value, EJson::Object, Reading, Where, TEXT("an object")))
			{
				return false;
			}
			for (const auto& Entry : Value.AsObject()->Values)
			{
				const FString EntryWhere = FString::Printf(TEXT("%s[%s]"), *Where, *Entry.Key);
				if (IsKey(*Map.KeyProp) && !CheckKey(FString(*Entry.Key), Reading, EntryWhere))
				{
					return false;
				}
				if (Entry.Value.IsValid() && !Entry.Value->IsNull() && !ValidateElement(*Map.ValueProp, *Entry.Value, Reading, EntryWhere))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateStruct(const FStructProperty& Struct, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			if (IsKey(Struct))
			{
				return Expect(Value, EJson::String, Reading, Where, TEXT("a key, written as a string")) && CheckKey(Value.AsString(), Reading, Where);
			}
			if (Value.Type == EJson::String)
			{
				// The engine's own text form of the struct (a colour, a date), converted by the struct itself.
				return true;
			}
			if (const int32 Components = VectorComponents(*Struct.Struct); Components > 0 && Value.Type == EJson::Array)
			{
				return CheckVector(Components, Value, Reading, Where);
			}
			return Expect(Value, EJson::Object, Reading, Where, TEXT("an object")) && ValidateObject(*Struct.Struct, *Value.AsObject(), Reading, Where);
		}

		bool ValidateScalar(const FProperty& Property, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			if (Property.IsA<FBoolProperty>())
			{
				return Expect(Value, EJson::Boolean, Reading, Where, TEXT("true or false"));
			}
			const FNumericProperty* Numeric = CastField<FNumericProperty>(&Property);
			if (Property.IsA<FEnumProperty>() || (Numeric && Numeric->IsEnum()))
			{
				return Value.Type == EJson::Number || Expect(Value, EJson::String, Reading, Where, TEXT("an enumerator's name"));
			}
			if (Numeric)
			{
				return Expect(Value, EJson::Number, Reading, Where, TEXT("a number"));
			}
			if (Property.IsA<FStrProperty>() || Property.IsA<FNameProperty>() || Property.IsA<FTextProperty>())
			{
				return Expect(Value, EJson::String, Reading, Where, TEXT("a string"));
			}
			// Object paths and the rarer kinds are converted, and their values checked, by the engine's converter.
			return true;
		}

		bool ValidateElement(const FProperty& Property, const FJsonValue& Value, FReading& Reading, const FString& Where)
		{
			if (const FArrayProperty* Array = CastField<FArrayProperty>(&Property))
			{
				return ValidateList(*Array->Inner, Value, Reading, Where);
			}
			if (const FSetProperty* Set = CastField<FSetProperty>(&Property))
			{
				return ValidateList(*Set->ElementProp, Value, Reading, Where);
			}
			if (const FMapProperty* Map = CastField<FMapProperty>(&Property))
			{
				return ValidateMap(*Map, Value, Reading, Where);
			}
			if (const FStructProperty* Struct = CastField<FStructProperty>(&Property))
			{
				return ValidateStruct(*Struct, Value, Reading, Where);
			}
			return ValidateScalar(Property, Value, Reading, Where);
		}

		bool ValidateObject(const UStruct& Type, const FJsonObject& Object, FReading& Reading, const FString& Where)
		{
			for (const auto& Field : Object.Values)
			{
				const FString Name(*Field.Key);
				const FString FieldWhere = Join(Where, Name);
				const FProperty* Property = Type.FindPropertyByName(FName(*Name));
				if (!Property)
				{
					return Reading.Fail(FieldWhere, FString::Printf(TEXT("%s has no field of this name"), *CppName(Type)));
				}
				if (Field.Value.IsValid() && !Field.Value->IsNull() && !ValidateElement(*Property, *Field.Value, Reading, FieldWhere))
				{
					return false;
				}
			}
			for (TFieldIterator<FProperty> It(&Type); It; ++It)
			{
				const TSharedPtr<FJsonValue> Value = Object.TryGetField(It->GetName());
				if (IsRequired(**It) && (!Value.IsValid() || Value->IsNull()))
				{
					return Reading.Fail(Join(Where, It->GetName()), TEXT("is required and missing"));
				}
			}
			return true;
		}

		const UStruct* ElementStruct(const FProperty& Property)
		{
			const FProperty* Element = &Property;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Element))
			{
				Element = Array->Inner;
			}
			else if (const FSetProperty* Set = CastField<FSetProperty>(Element))
			{
				Element = Set->ElementProp;
			}
			else if (const FMapProperty* Map = CastField<FMapProperty>(Element))
			{
				Element = Map->ValueProp;
			}
			const FStructProperty* Struct = CastField<FStructProperty>(Element);
			return Struct ? Struct->Struct : nullptr;
		}

		/** Every field reachable from a type, through nested structs, arrays, sets and maps. */
		void CollectFields(const UStruct& Type, TMultiMap<FName, const FProperty*>& Fields, TSet<const UStruct*>& Seen)
		{
			bool bSeen = false;
			Seen.Add(&Type, &bSeen);
			if (bSeen)
			{
				return;
			}
			for (TFieldIterator<FProperty> It(&Type); It; ++It)
			{
				Fields.Add(It->GetFName(), *It);
				if (const UStruct* Inner = ElementStruct(**It))
				{
					CollectFields(*Inner, Fields, Seen);
				}
			}
		}

		void ShapeObject(const UStruct& Type, FJsonObject& Object);

		/**
		 * The value the engine's converter is given for one element: a vector written as an array becomes the object of
		 * its components, which is the only form the converter reads; nested structs and containers are shaped within.
		 */
		TSharedPtr<FJsonValue> Shaped(const FProperty& Property, const TSharedPtr<FJsonValue>& Value)
		{
			if (!Value.IsValid() || Value->IsNull())
			{
				return Value;
			}
			const FProperty* Inner = nullptr;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(&Property)) { Inner = Array->Inner; }
			else if (const FSetProperty* Set = CastField<FSetProperty>(&Property)) { Inner = Set->ElementProp; }
			if (Inner && Value->Type == EJson::Array)
			{
				TArray<TSharedPtr<FJsonValue>> Items;
				for (const TSharedPtr<FJsonValue>& Item : Value->AsArray())
				{
					Items.Add(Shaped(*Inner, Item));
				}
				return MakeShared<FJsonValueArray>(Items);
			}
			if (const FMapProperty* Map = CastField<FMapProperty>(&Property); Map && Value->Type == EJson::Object)
			{
				for (auto& Entry : Value->AsObject()->Values)
				{
					Entry.Value = Shaped(*Map->ValueProp, Entry.Value);
				}
				return Value;
			}
			const FStructProperty* Struct = CastField<FStructProperty>(&Property);
			if (!Struct)
			{
				return Value;
			}
			if (Value->Type == EJson::Object)
			{
				ShapeObject(*Struct->Struct, *Value->AsObject());
				return Value;
			}
			const int32 Components = VectorComponents(*Struct->Struct);
			if (Components == 0 || Value->Type != EJson::Array)
			{
				return Value;
			}
			const TSharedRef<FJsonObject> Vector = MakeShared<FJsonObject>();
			for (int32 Index = 0; Index < Components; ++Index)
			{
				Vector->SetField(VectorFields[Index], Value->AsArray()[Index]);
			}
			return MakeShared<FJsonValueObject>(Vector);
		}

		void ShapeObject(const UStruct& Type, FJsonObject& Object)
		{
			for (auto& Field : Object.Values)
			{
				if (const FProperty* Property = Type.FindPropertyByName(FName(*Field.Key)))
				{
					Field.Value = Shaped(*Property, Field.Value);
				}
			}
		}

		/** Two units agree when they are the same unit, perhaps under two names (a nit is a candela per square metre). */
		bool UnitsAgree(EUnit Declared, EUnit OfField)
		{
			return Declared == OfField
				|| (FUnitConversion::AreUnitsCompatible(Declared, OfField) && FUnitConversion::Convert(1.0, Declared, OfField) == 1.0);
		}

		bool CheckDeclaredUnits(const UStruct& Type, FReading& Reading)
		{
			TMultiMap<FName, const FProperty*> Fields;
			TSet<const UStruct*> Seen;
			CollectFields(Type, Fields, Seen);
			for (const TPair<FString, EUnit>& Declared : Reading.Units)
			{
				const FString Where = Join(UnitsDeclaration, Declared.Key);
				TArray<const FProperty*> Matches;
				Fields.MultiFind(FName(*Declared.Key), Matches);
				if (Matches.IsEmpty())
				{
					return Reading.Fail(Where, FString::Printf(TEXT("%s has no field of this name"), *CppName(Type)));
				}
#if WITH_METADATA
				const TCHAR* DeclaredName = FUnitConversion::GetUnitDisplayString(Declared.Value);
				for (const FProperty* Field : Matches)
				{
					const FString& FieldUnits = Field->GetMetaData(UnitsMetadata);
					const TOptional<EUnit> FieldUnit = FUnitConversion::UnitFromString(*FieldUnits);
					if (!FieldUnit.IsSet())
					{
						return Reading.Fail(Where, FString::Printf(TEXT("is declared in %s, but the field has no unit"), DeclaredName));
					}
					if (!UnitsAgree(Declared.Value, FieldUnit.GetValue()))
					{
						return Reading.Fail(Where, FString::Printf(TEXT("is declared in %s, but the field is in %s"), DeclaredName,
							FUnitConversion::GetUnitDisplayString(FieldUnit.GetValue())));
					}
				}
#endif
			}
			return true;
		}

		bool ReadUnits(const FJsonValue& Value, FReading& Reading)
		{
			if (!Expect(Value, EJson::Object, Reading, UnitsDeclaration, TEXT("an object of field names and units")))
			{
				return false;
			}
			for (const auto& Entry : Value.AsObject()->Values)
			{
				const FString Where = Join(UnitsDeclaration, *Entry.Key);
				if (!Expect(*Entry.Value, EJson::String, Reading, Where, TEXT("a unit's name or symbol")))
				{
					return false;
				}
				const TOptional<EUnit> Unit = FUnitConversion::UnitFromString(*Entry.Value->AsString());
				if (!Unit.IsSet())
				{
					return Reading.Fail(Where, FString::Printf(TEXT("'%s' is not a unit the engine's unit table knows"), *Entry.Value->AsString()));
				}
				Reading.Units.Add(FString(*Entry.Key), Unit.GetValue());
			}
			return true;
		}

		bool ReadSources(const FJsonValue& Value, FReading& Reading)
		{
			if (Value.Type == EJson::String)
			{
				return true;
			}
			if (!Expect(Value, EJson::Array, Reading, SourcesDeclaration, TEXT("a string or an array of strings")))
			{
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Source : Value.AsArray())
			{
				if (!Expect(*Source, EJson::String, Reading, SourcesDeclaration, TEXT("a string")))
				{
					return false;
				}
			}
			return true;
		}

		/** Splits a file's object into its declarations and its records; each record is named for messages. */
		bool ReadDeclarations(const FJsonObject& Root, FReading& Reading, TArray<TPair<FString, TSharedRef<FJsonObject>>>& OutRecords)
		{
			const TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
			const TArray<TSharedPtr<FJsonValue>>* Listed = nullptr;
			for (const auto& Field : Root.Values)
			{
				const FString Name(*Field.Key);
				if (!Name.StartsWith(DeclarationMark))
				{
					Fields->Values.Add(Field.Key, Field.Value);
				}
				else if (Name == RecordsDeclaration)
				{
					if (!Expect(*Field.Value, EJson::Array, Reading, Name, TEXT("an array of records")))
					{
						return false;
					}
					Listed = &Field.Value->AsArray();
				}
				else if (Name == UnitsDeclaration)
				{
					if (!ReadUnits(*Field.Value, Reading))
					{
						return false;
					}
				}
				else if (Name == SourcesDeclaration)
				{
					if (!ReadSources(*Field.Value, Reading))
					{
						return false;
					}
				}
				else
				{
					return Reading.Fail(Name, TEXT("is not a declaration; a file declares $records, $units and $sources"));
				}
			}
			if (!Listed)
			{
				OutRecords.Emplace(FString(), Fields);
				return true;
			}
			if (!Fields->Values.IsEmpty())
			{
				return Reading.Fail(FString(), TEXT("has fields beside $records; a listed file keeps every field inside its records"));
			}
			for (int32 Index = 0; Index < Listed->Num(); ++Index)
			{
				const FString Where = Element(RecordsDeclaration, Index);
				const TSharedPtr<FJsonValue>& Item = (*Listed)[Index];
				if (!Item.IsValid() || Item->Type != EJson::Object)
				{
					return Reading.Fail(Where, TEXT("is not a record; a record is an object"));
				}
				OutRecords.Emplace(Where, Item->AsObject().ToSharedRef());
			}
			return true;
		}

		/** A file's name in messages and in dependency sets: relative to the data root when inside it. */
		FString RecordName(const FString& FullPath)
		{
			const FString Root = DataRoot() + TEXT("/");
			return FullPath.StartsWith(Root) ? FullPath.RightChop(Root.Len()) : FullPath;
		}

		bool ReadFileInto(const UScriptStruct& Type, const FString& FullPath, bool bExactlyOne, TFunctionRef<void*()> Emplace, FReading& Reading)
		{
			TArray<uint8> Bytes;
			if (!FFileHelper::LoadFileToArray(Bytes, *FullPath, FILEREAD_Silent))
			{
				return Reading.Fail(FString(), TEXT("can't be read"));
			}
			FMuseeDependencySet::Record(Reading.File, FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString());

			FString Text;
			FFileHelper::BufferToString(Text, Bytes.GetData(), Bytes.Num());
			TSharedPtr<FJsonObject> Root;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
			{
				return Reading.Fail(FString(), FString::Printf(TEXT("is not a JSON object (%s)"), *Reader->GetErrorMessage()));
			}
			TArray<TPair<FString, TSharedRef<FJsonObject>>> Records;
			if (!ReadDeclarations(*Root, Reading, Records) || !CheckDeclaredUnits(Type, Reading))
			{
				return false;
			}
			if (bExactlyOne && Records.Num() != 1)
			{
				return Reading.Fail(FString(), FString::Printf(TEXT("holds %d records where one is read"), Records.Num()));
			}
			for (const TPair<FString, TSharedRef<FJsonObject>>& Record : Records)
			{
				if (!ValidateObject(Type, *Record.Value, Reading, Record.Key))
				{
					return false;
				}
			}
			for (const TPair<FString, TSharedRef<FJsonObject>>& Record : Records)
			{
				ShapeObject(Type, *Record.Value);
				FText Reason;
				if (!FJsonObjectConverter::JsonObjectToUStruct(Record.Value, &Type, Emplace(), 0, 0, false, &Reason))
				{
					return Reading.Fail(Record.Key, Reason.ToString());
				}
			}
			return true;
		}

		bool ReadFile(const UScriptStruct& Type, const FString& FullPath, bool bExactlyOne, TFunctionRef<void*()> Emplace, FString& OutError)
		{
			FReading Reading{RecordName(FullPath)};
			const bool bRead = ReadFileInto(Type, FullPath, bExactlyOne, Emplace, Reading);
			OutError = Reading.Error;
			return bRead;
		}
	}

	FString DataRoot()
	{
		return DataRootOf(FPlatformProcess::BaseDir(), FPaths::ProjectDir());
	}

	FString DataRootOf(const FString& BaseDir, const FString& ProjectDir)
	{
		return FPaths::ConvertRelativePathToFull(BaseDir, ProjectDir / TEXT("Data"));
	}

	bool ReadInto(const UScriptStruct& Type, const FString& Path, bool bExactlyOne, TFunctionRef<void*()> Emplace, FString& OutError)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::IsRelative(Path) ? DataRoot() / Path : Path);
		IFileManager& Files = IFileManager::Get();
		if (Files.DirectoryExists(*FullPath))
		{
			if (bExactlyOne)
			{
				OutError = FString::Printf(TEXT("%s: is a folder; one record is read from one file."), *RecordName(FullPath));
				return false;
			}
			TArray<FString> Names;
			Files.FindFiles(Names, *(FullPath / TEXT("*.json")), true, false);
			Names.Sort();
			for (const FString& Name : Names)
			{
				if (!ReadFile(Type, FullPath / Name, false, Emplace, OutError))
				{
					return false;
				}
			}
			return true;
		}
		if (!Files.FileExists(*FullPath))
		{
			OutError = FString::Printf(TEXT("%s: there is no record file or folder here."), *RecordName(FullPath));
			return false;
		}
		return ReadFile(Type, FullPath, bExactlyOne, Emplace, OutError);
	}
}
