#include "MotionFaults.h"

#include "MotionMechanism.h"
#include "MotionMechanismSpec.h"
#include "MotionProfile.h"

namespace Musee::Motion
{
	namespace
	{
		TArray<FString> StepFaults(const FMotionMechanismSpec& Spec, const TArray<FMotionStep>& Steps, const FString& Sequence)
		{
			TArray<FString> Out;
			for (int32 Index = 0; Index < Steps.Num(); ++Index)
			{
				const FMotionStep& Step = Steps[Index];
				const FString Where = FString::Printf(TEXT("%s step %d"), *Sequence, Index);
				if (Step.IsMove() == !Step.Channel.IsNone())
				{
					Out.Add(Where + TEXT(" must either move or ramp a channel"));
				}
				if (Step.IsMove() && !Spec.PositionOf(Step.Move).IsSet())
				{
					Out.Add(FString::Printf(TEXT("%s moves to '%s', which is none of its states"), *Where, *Step.Move.ToString()));
				}
				if (Step.Time < 0.0f || Step.To < 0.0f || Step.To > 1.0f)
				{
					Out.Add(Where + TEXT(" needs a time of zero or more and a value from 0 to 1"));
				}
			}
			return Out;
		}

		TArray<FString> TimingFaults(const FMotionMechanismSpec& Spec)
		{
			TArray<FString> Out;
			const FMotionDrive& Drive = Spec.Drive;
			if (!Drive.IsSet())
			{
				if (Spec.Time <= 0.0f)
				{
					Out.Add(TEXT("its time must be more than zero, or it gives a drive"));
				}
				if (!ParseProfile(Spec.Profile).IsSet())
				{
					Out.Add(FString::Printf(TEXT("its profile '%s' is neither linear nor ease-in-out"), *Spec.Profile.ToString()));
				}
				return Out;
			}
			if (Spec.Time > 0.0f || !Spec.Profile.IsNone())
			{
				Out.Add(TEXT("it gives a drive in place of a time and a profile, never both"));
			}
			if (Drive.Speed <= 0.0 || Drive.Accel <= 0.0 || Drive.Jerk <= 0.0)
			{
				Out.Add(TEXT("its drive's speed, acceleration and jerk must each be more than zero"));
			}
			for (int32 Index = 0; Index < Drive.Zones.Num(); ++Index)
			{
				const FMotionDriveZone& Zone = Drive.Zones[Index];
				if (Zone.From < 0.0 || Zone.To > 1.0 || Zone.From >= Zone.To || Zone.Speed <= 0.0 || Zone.Accel <= 0.0)
				{
					Out.Add(FString::Printf(TEXT("its drive's zone %d must run forwards within the travel, with a speed and an acceleration more than zero"), Index));
				}
			}
			return Out;
		}

		TArray<FString> StopFaults(const FMotionMechanismSpec& Spec)
		{
			TArray<FString> Out;
			TSet<FName> Named;
			for (int32 Index = 0; Index < Spec.Stops.Num(); ++Index)
			{
				const FMotionStop& Stop = Spec.Stops[Index];
				if (Stop.Stop.IsNone() || Stop.Stop == UMotionMechanism::Open || Stop.Stop == UMotionMechanism::Closed)
				{
					Out.Add(FString::Printf(TEXT("its stop %d needs a name other than open and closed"), Index));
				}
				else if (Named.Contains(Stop.Stop))
				{
					Out.Add(FString::Printf(TEXT("it names the stop '%s' twice"), *Stop.Stop.ToString()));
				}
				Named.Add(Stop.Stop);
				if (Stop.At < 0.0 || Stop.At > 1.0)
				{
					Out.Add(FString::Printf(TEXT("its stop '%s' must lie from 0 to 1 of the travel"), *Stop.Stop.ToString()));
				}
			}
			const auto StateFault = [&Spec, &Out](const TCHAR* Role, FName State)
			{
				if (!State.IsNone() && !Spec.PositionOf(State).IsSet())
				{
					Out.Add(FString::Printf(TEXT("its %s state '%s' is none of its states"), Role, *State.ToString()));
				}
			};
			StateFault(TEXT("drawn"), Spec.Drawn);
			StateFault(TEXT("home"), Spec.Home);
			return Out;
		}

		TArray<FString> RideFaults(const FMotionMechanismSpec& Spec)
		{
			TArray<FString> Out;
			for (int32 Index = 0; Index < Spec.Rides.Num(); ++Index)
			{
				const FMotionRide& Ride = Spec.Rides[Index];
				const FString Where = FString::Printf(TEXT("ride %d"), Index);
				if (!Spec.PositionOf(Ride.At).IsSet())
				{
					Out.Add(FString::Printf(TEXT("its %s is offered at '%s', which is none of its states"), *Where, *Ride.At.ToString()));
				}
				if (Ride.Sequence.IsEmpty())
				{
					Out.Add(FString::Printf(TEXT("its %s has no steps"), *Where));
				}
				for (const FMotionChannelCondition& Condition : Ride.Requires.Channels)
				{
					if (Condition.AtLeast < 0.0f || Condition.AtMost > 1.0f || Condition.AtLeast > Condition.AtMost)
					{
						Out.Add(FString::Printf(TEXT("its %s requires %s between bounds from 0 to 1, the least first"), *Where, *Condition.Channel.ToString()));
					}
				}
				Out.Append(StepFaults(Spec, Ride.Sequence, Where));
			}
			return Out;
		}
	}

	TArray<FString> Faults(const FMotionMechanismSpec& Spec)
	{
		TArray<FString> Out;
		if (Spec.Joint != TEXT("prismatic") && Spec.Joint != TEXT("revolute"))
		{
			Out.Add(FString::Printf(TEXT("its joint '%s' is neither prismatic nor revolute"), *Spec.Joint.ToString()));
		}
		if (Spec.Axis.Num() != 3 || FVector(Spec.Axis[0], Spec.Axis[1], Spec.Axis[2]).IsNearlyZero())
		{
			Out.Add(TEXT("its axis is not a direction of three numbers"));
		}
		if (Spec.Travel <= 0.0f)
		{
			Out.Add(TEXT("its travel must be more than zero"));
		}
		Out.Append(TimingFaults(Spec));
		if (!Spec.Guard.IsNone() && Spec.Guard != TEXT("none") && Spec.Guard != TEXT("body-in-travel") && Spec.Guard != TEXT("body-in-travel-reverse"))
		{
			Out.Add(FString::Printf(TEXT("its guard '%s' is not none, body-in-travel or body-in-travel-reverse"), *Spec.Guard.ToString()));
		}
		if (Spec.Clearance < 0.0f)
		{
			Out.Add(TEXT("its clearance is negative"));
		}
		Out.Append(StopFaults(Spec));
		if (Spec.Follows.IsSet() && (!Spec.Control.IsNone() || !Spec.Open.IsEmpty() || !Spec.Close.IsEmpty() || !Spec.Rides.IsEmpty()))
		{
			Out.Add(TEXT("it follows a channel, so it has no control, no sequences and no rides"));
		}
		if (Spec.Follows.IsSet() && (Spec.Follows.In.Num() != 2 || Spec.Follows.Out.Num() != 2 || Spec.Follows.In[0] == Spec.Follows.In[1]))
		{
			Out.Add(TEXT("it follows a channel with an input range of two different values and an output range of two"));
		}
		Out.Append(StepFaults(Spec, Spec.Open, TEXT("open")));
		Out.Append(StepFaults(Spec, Spec.Close, TEXT("close")));
		Out.Append(RideFaults(Spec));
		return Out;
	}
}
