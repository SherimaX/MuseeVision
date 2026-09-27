#include "MotionFaults.h"
#include "MotionMechanism.h"
#include "MotionSchedule.h"

// The faults of a schedule as a whole: what no single mechanism's spec can see.

namespace Musee::Motion
{
	namespace
	{
		/** A telescope's strokes add up to the length it gains within this: a tenth of a millimetre. */
		constexpr double StrokeToleranceMeters = 1.0e-4;

		TArray<FString> ChannelFaults(const FMotionSchedule& Schedule)
		{
			TArray<FString> Out;
			for (const FMotionChannelSpec& Channel : Schedule.Channels)
			{
				const FMotionFollowing& Map = Channel.Follows;
				if (Map.IsSet() && (!Map.IsSound() || FMath::Min(Map.Out[0], Map.Out[1]) < 0.0 || FMath::Max(Map.Out[0], Map.Out[1]) > 1.0))
				{
					Out.Add(FString::Printf(TEXT("Channel %s follows with two different channel values and two values from 0 to 1"), *Channel.Key.ToString()));
				}
				if (Channel.Rest < 0.0f || Channel.Rest > 1.0f || (Map.IsSet() && Channel.Rest != 0.0f))
				{
					Out.Add(FString::Printf(TEXT("Channel %s rests from 0 to 1, and only when it follows nothing"), *Channel.Key.ToString()));
				}
			}
			return Out;
		}

		TArray<FString> PerformanceFaults(const FMotionSchedule& Schedule)
		{
			TArray<FString> Out;
			for (const FMotionPerformanceSpec& Performance : Schedule.Performances)
			{
				const FString Where = FString::Printf(TEXT("Performance %s"), *Performance.Key.ToString());
				if (Performance.Seconds <= 0.0f || Performance.FadeIn < 0.0f || Performance.FadeOut < 0.0f
					|| Performance.FadeIn + Performance.FadeOut > Performance.Seconds)
				{
					Out.Add(Where + TEXT(" runs for more than zero seconds, its fades within them"));
				}
				Out.Append(RequirementFaults(Performance.Requires, Where));
			}
			return Out;
		}

		TArray<FString> InterlockFaults(const FMotionSchedule& Schedule)
		{
			TArray<FString> Out;
			for (const FMotionInterlock& Interlock : Schedule.Interlocks)
			{
				const FString Where = FString::Printf(TEXT("Interlock %s"), *Interlock.Key.ToString());
				if (Interlock.Mechanism.IsNone() == Interlock.Channel.IsNone())
				{
					Out.Add(Where + TEXT(" guards either a mechanism or a channel"));
				}
				else if (!Interlock.Mechanism.IsNone() && !Schedule.FindMechanism(Interlock.Mechanism))
				{
					Out.Add(FString::Printf(TEXT("%s guards %s, which is no mechanism of the stage"), *Where, *Interlock.Mechanism.ToString()));
				}
				if (Interlock.Above < 0.0 || Interlock.Below > 1.0 || Interlock.Above >= Interlock.Below)
				{
					Out.Add(Where + TEXT(" guards a band from 0 to 1, its floor below its ceiling"));
				}
				if (Interlock.Allows.IsEmpty())
				{
					Out.Add(Where + TEXT(" names what allows the band"));
				}
				for (const FMotionRequirement& Allowed : Interlock.Allows)
				{
					Out.Append(RequirementFaults(Allowed, Where));
				}
			}
			return Out;
		}

		TArray<FString> TelescopeFaults(const FMotionExtentSpec& Extent, const FString& Where)
		{
			TArray<FString> Out;
			double Strokes = 0.0;
			double Guide = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Extent.Telescope.Num(); ++Index)
			{
				const FMotionTelescopeStage& Stage = Extent.Telescope[Index];
				Strokes += Stage.Stroke;
				const bool bLast = Index + 1 == Extent.Telescope.Num();
				if (Stage.Radius <= 0.0 || Stage.Radius > Guide || Stage.Bore < 0.0 || Stage.Bore >= Stage.Radius || (bLast != (Stage.Bore == 0.0)))
				{
					Out.Add(FString::Printf(TEXT("%s: stage %d fits in the bore before it, and only the last is solid"), *Where, Index + 1));
				}
				if (Stage.Stroke <= 0.0 || Stage.Stroke >= Stage.Length || Stage.Length > Extent.ClosedLength())
				{
					Out.Add(FString::Printf(TEXT("%s: stage %d is no longer than the closed telescope and keeps part of itself in its guide"), *Where, Index + 1));
				}
				Guide = Stage.Bore;
			}
			const double Gained = Extent.Length.IsSound() ? FMath::Abs(Extent.Length.Out[1] - Extent.Length.Out[0]) : 0.0;
			if (!FMath::IsNearlyEqual(Strokes, Gained, StrokeToleranceMeters))
			{
				Out.Add(FString::Printf(TEXT("%s: its strokes add up to %.4f m, where its length gains %.4f m"), *Where, Strokes, Gained));
			}
			return Out;
		}

		TArray<FString> ExtentFaults(const FMotionSchedule& Schedule)
		{
			TArray<FString> Out;
			for (const FMotionExtentSpec& Extent : Schedule.Extents)
			{
				const FString Where = FString::Printf(TEXT("Extent %s"), *Extent.Key.ToString());
				if (!Extent.Length.IsSound() || FMath::Min(Extent.Length.Out[0], Extent.Length.Out[1]) < 0.0)
				{
					Out.Add(Where + TEXT(" follows its length with two different channel values and two lengths of zero or more"));
				}
				if (Extent.Axis.IsNearlyZero())
				{
					Out.Add(Where + TEXT(" runs along a direction"));
				}
				if (Extent.Telescope.IsEmpty() == !Extent.Turns.IsSet())
				{
					Out.Add(Where + TEXT(" is either a telescope or a column of turns"));
				}
				if (Extent.Turns.IsSet() && (Extent.Turns.Bore < 0.0 || Extent.Turns.Bore >= Extent.Turns.Radius))
				{
					Out.Add(Where + TEXT(": its turns' bore lies within their radius"));
				}
				if (!Extent.Telescope.IsEmpty())
				{
					Out.Append(TelescopeFaults(Extent, Where));
				}
			}
			return Out;
		}

		TArray<FString> GroupFaults(const FMotionSchedule& Schedule)
		{
			TArray<FString> Out;
			TMap<FMuseeKey, int32> AwayAtStart;
			for (const FMotionMechanismSpec& Spec : Schedule.Mechanisms)
			{
				if (Spec.Group.IsNone())
				{
					continue;
				}
				if (Spec.StartPosition() > 0.0)
				{
					++AwayAtStart.FindOrAdd(Spec.Group);
				}
				if (Spec.Control.IsNone() || Spec.Close.IsEmpty())
				{
					Out.Add(FString::Printf(TEXT("Mechanism %s is in group %s, so a control works it and it has a close sequence"),
						*Spec.Key.ToString(), *Spec.Group.ToString()));
				}
			}
			for (const TPair<FMuseeKey, int32>& Group : AwayAtStart)
			{
				if (Group.Value > 1)
				{
					Out.Add(FString::Printf(TEXT("Group %s starts with %d of its mechanisms away from closed; at most one may be"), *Group.Key.ToString(), Group.Value));
				}
			}
			return Out;
		}
	}

	TArray<FString> ScheduleFaults(const FMotionSchedule& Schedule)
	{
		TArray<FString> Out = ChannelFaults(Schedule);
		Out.Append(PerformanceFaults(Schedule));
		Out.Append(InterlockFaults(Schedule));
		Out.Append(ExtentFaults(Schedule));
		Out.Append(GroupFaults(Schedule));
		return Out;
	}
}
