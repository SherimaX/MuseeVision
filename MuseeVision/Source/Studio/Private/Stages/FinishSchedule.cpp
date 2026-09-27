#include "Stages/FinishSchedule.h"

#include "MuseeRecords.h"

TValueOrError<FStudioFinishSchedule, FString> Musee::Studio::ReadFinishSchedule(const FString& ScheduleDir)
{
	return ReadRecord<FStudioFinishSchedule>(ScheduleDir / FinishesFile);
}
