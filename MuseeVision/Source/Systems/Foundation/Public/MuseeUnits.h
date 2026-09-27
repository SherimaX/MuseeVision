#pragma once

#include "CoreMinimal.h"
#include "Math/UnitConversion.h"

#include <compare>
#include <type_traits>

namespace Musee
{
	/**
	 * A physical quantity: a number that carries the unit it was measured in, so a length is never added to an
	 * illuminance or passed where a time is wanted. The unit is the engine's own unit enumeration, so a quantity, a
	 * record's declared unit and a property's Units metadata all name units the same way.
	 *
	 * Only the arithmetic the physics allows is defined:
	 * - every quantity adds to, subtracts from and compares with its own kind;
	 * - a quantity on a ratio scale (it has a true zero: metres, kilograms, lux, kelvin) also scales by a pure number
	 *   and divides by its own kind into a pure ratio;
	 * - a quantity on a logarithmic scale (exposure values, decibels) is a logarithm of a ratio: it is shifted by
	 *   offsets (a stop, a gain) but never scaled, because a multiple of a logarithm means nothing physical;
	 * - negation exists only where a negative value is a real thing: a displacement, a time offset, a turn the other
	 *   way, a speed in reverse, a gain below unity or a stop down. A mass, a flux, an intensity, an illuminance, a
	 *   luminance and an absolute temperature are never negative, though the difference of two may be.
	 * Products of different kinds are defined only where the museum needs them (distance, speed and time).
	 */
	template <EUnit TheUnit>
	class TQuantity
	{
	public:
		/** The unit this quantity is measured in. */
		static constexpr EUnit Unit = TheUnit;

		/** Exposure values and decibels are logarithms of ratios. */
		static constexpr bool bLogarithmic = TheUnit == EUnit::ExposureValue || TheUnit == EUnit::Decibels;

		/** Whether a negative value of this unit is a real thing (see the type's comment). */
		static constexpr bool bSigned = TheUnit == EUnit::Meters || TheUnit == EUnit::Seconds
			|| TheUnit == EUnit::Degrees || TheUnit == EUnit::Radians || TheUnit == EUnit::MetersPerSecond
			|| bLogarithmic;

		/** Zero of the unit. */
		constexpr TQuantity() = default;

		/** A value in this unit. */
		constexpr explicit TQuantity(double InValue)
			: Value(InValue)
		{
		}

		/** The number, in this unit. */
		[[nodiscard]] constexpr double Get() const { return Value; }

		constexpr bool operator==(const TQuantity&) const = default;
		constexpr auto operator<=>(const TQuantity&) const = default;

		[[nodiscard]] constexpr TQuantity operator+(TQuantity Other) const { return TQuantity(Value + Other.Value); }
		[[nodiscard]] constexpr TQuantity operator-(TQuantity Other) const { return TQuantity(Value - Other.Value); }

		constexpr TQuantity& operator+=(TQuantity Other)
		{
			Value += Other.Value;
			return *this;
		}

		constexpr TQuantity& operator-=(TQuantity Other)
		{
			Value -= Other.Value;
			return *this;
		}

		[[nodiscard]] constexpr TQuantity operator-() const
			requires bSigned
		{
			return TQuantity(-Value);
		}

		[[nodiscard]] constexpr TQuantity operator*(double Factor) const
			requires (!bLogarithmic)
		{
			return TQuantity(Value * Factor);
		}

		[[nodiscard]] constexpr TQuantity operator/(double Divisor) const
			requires (!bLogarithmic)
		{
			return TQuantity(Value / Divisor);
		}

		/** The pure ratio of two quantities of one kind. */
		[[nodiscard]] constexpr double operator/(TQuantity Other) const
			requires (!bLogarithmic)
		{
			return Value / Other.Value;
		}

		constexpr TQuantity& operator*=(double Factor)
			requires (!bLogarithmic)
		{
			Value *= Factor;
			return *this;
		}

		constexpr TQuantity& operator/=(double Divisor)
			requires (!bLogarithmic)
		{
			Value /= Divisor;
			return *this;
		}

	private:
		double Value = 0.0;
	};

	/** A pure number times a quantity on a ratio scale. */
	template <EUnit TheUnit>
		requires (!TQuantity<TheUnit>::bLogarithmic)
	[[nodiscard]] constexpr TQuantity<TheUnit> operator*(double Factor, TQuantity<TheUnit> Quantity)
	{
		return Quantity * Factor;
	}

	/** Length: the museum's one unit of length. */
	using Meters = TQuantity<EUnit::Meters>;
	/** Mass. */
	using Kilograms = TQuantity<EUnit::Kilograms>;
	/** Duration, or an offset in time. */
	using Seconds = TQuantity<EUnit::Seconds>;
	/** Plane angle in degrees: how bearings, slopes and the sun's altitude are written. */
	using Degrees = TQuantity<EUnit::Degrees>;
	/** Plane angle in radians: how trigonometry takes it. */
	using Radians = TQuantity<EUnit::Radians>;
	/** Speed along a path. */
	using MetersPerSecond = TQuantity<EUnit::MetersPerSecond>;
	/** Illuminance: luminous flux falling on a surface, lumens per square metre. */
	using Lux = TQuantity<EUnit::Lux>;
	/** Luminous flux: the light a source emits in all directions. */
	using Lumens = TQuantity<EUnit::Lumens>;
	/** Luminous intensity: flux per steradian in one direction. */
	using Candela = TQuantity<EUnit::Candela>;
	/** Luminance: intensity per square metre of a surface seen from a direction (one nit is one cd/m2). */
	using Nits = TQuantity<EUnit::CandelaPerMeter2>;
	/** Absolute temperature; for light, the correlated colour temperature of a source. */
	using Kelvin = TQuantity<EUnit::Kelvin>;
	/** Exposure value at ISO 100: the base-2 logarithm of a scene's exposure, one unit per stop. */
	using EV100 = TQuantity<EUnit::ExposureValue>;
	/** A level or a gain: ten times the base-10 logarithm of a power ratio. */
	using Decibels = TQuantity<EUnit::Decibels>;

	/** Speed is distance over time. */
	[[nodiscard]] constexpr MetersPerSecond operator/(Meters Distance, Seconds Duration)
	{
		return MetersPerSecond(Distance.Get() / Duration.Get());
	}

	/** Distance is speed times time. */
	[[nodiscard]] constexpr Meters operator*(MetersPerSecond Speed, Seconds Duration)
	{
		return Meters(Speed.Get() * Duration.Get());
	}

	/** Distance is time times speed. */
	[[nodiscard]] constexpr Meters operator*(Seconds Duration, MetersPerSecond Speed)
	{
		return Speed * Duration;
	}

	/** Time is distance over speed. */
	[[nodiscard]] constexpr Seconds operator/(Meters Distance, MetersPerSecond Speed)
	{
		return Seconds(Distance.Get() / Speed.Get());
	}

	/** A half turn is pi radians and 180 degrees, by the definition of the degree. */
	[[nodiscard]] constexpr Radians ToRadians(Degrees Angle)
	{
		return Radians(Angle.Get() * (UE_DOUBLE_PI / 180.0));
	}

	/** A half turn is pi radians and 180 degrees, by the definition of the degree. */
	[[nodiscard]] constexpr Degrees ToDegrees(Radians Angle)
	{
		return Degrees(Angle.Get() * (180.0 / UE_DOUBLE_PI));
	}

	namespace Private
	{
		/**
		 * Unreal's world unit is the centimetre, so a metre is 100 engine units. The museum's code works in metres; this factor is
		 * used only by ToEngine and FromEngine, so centimetres appear nowhere else.
		 */
		inline constexpr double EngineUnitsPerMeter = 100.0;
	}

	/** A length in engine units. */
	[[nodiscard]] constexpr double ToEngine(Meters Length)
	{
		return Length.Get() * Private::EngineUnitsPerMeter;
	}

	/** A speed in engine units per second. */
	[[nodiscard]] constexpr double ToEngine(MetersPerSecond Speed)
	{
		return Speed.Get() * Private::EngineUnitsPerMeter;
	}

	/** A point or displacement given in metres, in engine units. The museum's frames and the engine share axes (x east, y south, z up). */
	[[nodiscard]] inline FVector ToEngine(const FVector& PointMeters)
	{
		return PointMeters * Private::EngineUnitsPerMeter;
	}

	/** An engine length (or, as FromEngine<MetersPerSecond>, an engine speed) in metres. */
	template <typename TQuantityType = Meters>
		requires (std::is_same_v<TQuantityType, Meters> || std::is_same_v<TQuantityType, MetersPerSecond>)
	[[nodiscard]] constexpr TQuantityType FromEngine(double EngineUnits)
	{
		return TQuantityType(EngineUnits / Private::EngineUnitsPerMeter);
	}

	/** An engine point or displacement, in metres. */
	[[nodiscard]] inline FVector FromEngine(const FVector& EngineUnits)
	{
		return EngineUnits / Private::EngineUnitsPerMeter;
	}
}
