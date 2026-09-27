#include "Operation/OperationRecords.h"

#include "LightLog.h"
#include "MuseeRecords.h"

namespace Musee::Light
{
	const FLightOperationRecord* OperationDesign()
	{
		static const TOptional<FLightOperationRecord> Kept = []() -> TOptional<FLightOperationRecord>
		{
			TValueOrError<FLightOperationRecord, FString> Read = ReadRecord<FLightOperationRecord>(TEXT("Light/Operation.json"));
			if (Read.HasError())
			{
				UE_LOG(LogMuseeLight, Error, TEXT("The luminaires' operation can't be read: %s"), *Read.GetError());
				return {};
			}
			return Read.StealValue();
		}();
		return Kept.GetPtrOrNull();
	}
}
