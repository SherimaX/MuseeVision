#include "Sky/Ephemeris.h"

#include "HAL/IConsoleManager.h"

namespace
{
	constexpr double Deg = UE_DOUBLE_PI / 180.0;

	/** Days since J2000.0. */
	double DaysSinceJ2000(const FDateTime& Utc) { return MuseeEphemeris::JulianDay(Utc) - 2451545.0; }

	double Obliquity(double N) { return (23.439 - 0.0000004 * N) * Deg; }

	/** Moon's ecliptic longitude and latitude (radians), about ±1°. */
	void MoonEcliptic(const FDateTime& Utc, double& Lon, double& Lat)
	{
		const double N = DaysSinceJ2000(Utc);
		const double L = 218.316 + 13.176396 * N;
		const double M = (134.963 + 13.064993 * N) * Deg;
		const double F = (93.272 + 13.229350 * N) * Deg;
		const double D = (297.850 + 12.190749 * N) * Deg;
		Lon = (L + 6.289 * FMath::Sin(M) - 1.274 * FMath::Sin(M - 2 * D) + 0.658 * FMath::Sin(2 * D) + 0.214 * FMath::Sin(2 * M)) * Deg;
		Lat = 5.128 * FMath::Sin(F) * Deg;
	}

	void Equatorial(double Lon, double Lat, const FDateTime& Utc, double& Ra, double& Dec)
	{
		const double E = Obliquity(DaysSinceJ2000(Utc));
		Ra = FMath::Atan2(FMath::Sin(Lon) * FMath::Cos(E) - FMath::Tan(Lat) * FMath::Sin(E), FMath::Cos(Lon));
		Dec = FMath::Asin(FMath::Sin(Lat) * FMath::Cos(E) + FMath::Cos(Lat) * FMath::Sin(E) * FMath::Sin(Lon));
	}

	/** Local sidereal time (radians). */
	double LocalSiderealTime(const FDateTime& Utc, double Longitude)
	{
		const double Gmst = 280.46061837 + 360.98564736629 * DaysSinceJ2000(Utc);
		return FMath::Fmod(Gmst + Longitude, 360.0) * Deg;
	}

	FVector EquatorialVector(double Ra, double Dec)
	{
		return FVector(FMath::Cos(Dec) * FMath::Cos(Ra), FMath::Cos(Dec) * FMath::Sin(Ra), FMath::Sin(Dec));
	}

	/** Equatorial → RealityKit world (x east, y up, z south), as in Sky.swift's skyRotation. */
	FVector EquatorialToRealityKit(const FVector& V, double Lst, double Phi)
	{
		// e' = Rz(−LST)·v: (cos δ cos H, −cos δ sin H, sin δ) with H the hour angle.
		const FVector E(FMath::Cos(Lst) * V.X + FMath::Sin(Lst) * V.Y, -FMath::Sin(Lst) * V.X + FMath::Cos(Lst) * V.Y, V.Z);
		// Horizon frame: east = e'.y, up = cos φ e'.x + sin φ e'.z, south = sin φ e'.x − cos φ e'.z.
		return FVector(E.Y, FMath::Cos(Phi) * E.X + FMath::Sin(Phi) * E.Z, FMath::Sin(Phi) * E.X - FMath::Cos(Phi) * E.Z);
	}

	/** RealityKit (x east, y up, z south) → Unreal (X east, Y south, Z up). */
	FVector ToUnreal(const FVector& R) { return FVector(R.X, R.Z, R.Y); }

	struct FTerm { const TCHAR* Hanzi; const TCHAR* Pinyin; const TCHAR* English; };
	const FTerm Terms[24] = {
		{TEXT("立春"), TEXT("Lìchūn"), TEXT("Start of Spring")}, {TEXT("雨水"), TEXT("Yǔshuǐ"), TEXT("Rain Water")},
		{TEXT("驚蟄"), TEXT("Jīngzhé"), TEXT("Awakening of Insects")}, {TEXT("春分"), TEXT("Chūnfēn"), TEXT("Spring Equinox")},
		{TEXT("清明"), TEXT("Qīngmíng"), TEXT("Pure Brightness")}, {TEXT("穀雨"), TEXT("Gǔyǔ"), TEXT("Grain Rain")},
		{TEXT("立夏"), TEXT("Lìxià"), TEXT("Start of Summer")}, {TEXT("小滿"), TEXT("Xiǎomǎn"), TEXT("Grain Buds")},
		{TEXT("芒種"), TEXT("Mángzhòng"), TEXT("Grain in Ear")}, {TEXT("夏至"), TEXT("Xiàzhì"), TEXT("Summer Solstice")},
		{TEXT("小暑"), TEXT("Xiǎoshǔ"), TEXT("Minor Heat")}, {TEXT("大暑"), TEXT("Dàshǔ"), TEXT("Major Heat")},
		{TEXT("立秋"), TEXT("Lìqiū"), TEXT("Start of Autumn")}, {TEXT("處暑"), TEXT("Chǔshǔ"), TEXT("End of Heat")},
		{TEXT("白露"), TEXT("Báilù"), TEXT("White Dew")}, {TEXT("秋分"), TEXT("Qiūfēn"), TEXT("Autumn Equinox")},
		{TEXT("寒露"), TEXT("Hánlù"), TEXT("Cold Dew")}, {TEXT("霜降"), TEXT("Shuāngjiàng"), TEXT("Frost's Descent")},
		{TEXT("立冬"), TEXT("Lìdōng"), TEXT("Start of Winter")}, {TEXT("小雪"), TEXT("Xiǎoxuě"), TEXT("Minor Snow")},
		{TEXT("大雪"), TEXT("Dàxuě"), TEXT("Major Snow")}, {TEXT("冬至"), TEXT("Dōngzhì"), TEXT("Winter Solstice")},
		{TEXT("小寒"), TEXT("Xiǎohán"), TEXT("Minor Cold")}, {TEXT("大寒"), TEXT("Dàhán"), TEXT("Major Cold")},
	};
}

FMuseeObserver FMuseeObserver::FromTimeZone()
{
	return MuseeClock::Observer();
}

static TAutoConsoleVariable<int32> CVarMuseeCity(
	TEXT("musee.City"), 0,
	TEXT("The museum's city, for its clock and its sky: 0 Beijing, 1 New York."),
	ECVF_Default);

namespace MuseeClock
{
	ECity City()
	{
		return CVarMuseeCity.GetValueOnGameThread() == 1 ? ECity::NewYork : ECity::Beijing;
	}

	void SetCity(ECity InCity)
	{
		CVarMuseeCity->Set(InCity == ECity::NewYork ? 1 : 0, ECVF_SetByConsole);
	}

	FText CityName(ECity InCity)
	{
		return InCity == ECity::NewYork ? NSLOCTEXT("Musee", "NewYork", "New York") : NSLOCTEXT("Musee", "Beijing", "Beijing");
	}

	/** The n-th Sunday (1-based) of a month, as a date. */
	static FDateTime NthSunday(int32 Year, int32 Month, int32 N)
	{
		FDateTime D(Year, Month, 1);
		while (D.GetDayOfWeek() != EDayOfWeek::Sunday) { D += FTimespan::FromDays(1); }
		return D + FTimespan::FromDays(7 * (N - 1));
	}

	FTimespan UtcOffset(ECity InCity, const FDateTime& Utc)
	{
		if (InCity == ECity::Beijing) { return FTimespan::FromHours(8); }   // China Standard Time, no DST
		// New York: EST (UTC−5), EDT (UTC−4) from 2:00 on the second Sunday of March (07:00 UTC)
		// to 2:00 on the first Sunday of November (06:00 UTC).
		const int32 Y = Utc.GetYear();
		const FDateTime Start = NthSunday(Y, 3, 2) + FTimespan::FromHours(7);
		const FDateTime End = NthSunday(Y, 11, 1) + FTimespan::FromHours(6);
		return FTimespan::FromHours(Utc >= Start && Utc < End ? -4 : -5);
	}

	FDateTime UtcNow()
	{
		const FDateTime Local = LocalNow();
		return Local - UtcOffset(City(), FDateTime::UtcNow());
	}

	FDateTime LocalNow()
	{
		const FDateTime Utc = FDateTime::UtcNow();
		const FDateTime Now = Utc + UtcOffset(City(), Utc);
		static IConsoleVariable* Hour = IConsoleManager::Get().FindConsoleVariable(TEXT("musee.Hour"));
		const float H = Hour ? Hour->GetFloat() : -1.f;
		if (H < 0.f) { return Now; }
		return FDateTime(Now.GetYear(), Now.GetMonth(), Now.GetDay()) + FTimespan::FromHours(FMath::Fmod(H, 24.f));
	}

	FMuseeObserver Observer()
	{
		FMuseeObserver O;
		if (City() == ECity::NewYork) { O.Latitude = 40.71; O.Longitude = -74.01; }
		else { O.Latitude = 39.90; O.Longitude = 116.40; }
		return O;
	}
}

double MuseeEphemeris::JulianDay(const FDateTime& Utc)
{
	return (Utc - FDateTime(1970, 1, 1)).GetTotalDays() + 2440587.5;
}

double MuseeEphemeris::SunLongitude(const FDateTime& Utc)
{
	const double N = DaysSinceJ2000(Utc);
	const double L = FMath::Fmod(280.460 + 0.9856474 * N, 360.0);
	const double G = (357.528 + 0.9856003 * N) * Deg;
	return (L + 1.915 * FMath::Sin(G) + 0.020 * FMath::Sin(2 * G)) * Deg;
}

FMatrix MuseeEphemeris::SkyRotation(const FDateTime& Utc, const FMuseeObserver& Observer)
{
	const double Lst = LocalSiderealTime(Utc, Observer.Longitude);
	const double Phi = Observer.Latitude * Deg;
	const FVector X = ToUnreal(EquatorialToRealityKit(FVector(1, 0, 0), Lst, Phi));
	const FVector Y = ToUnreal(EquatorialToRealityKit(FVector(0, 1, 0), Lst, Phi));
	const FVector Z = ToUnreal(EquatorialToRealityKit(FVector(0, 0, 1), Lst, Phi));
	return FMatrix(X, Y, Z, FVector::ZeroVector);
}

MuseeEphemeris::FSun MuseeEphemeris::Sun(const FDateTime& Utc, const FMuseeObserver& Observer)
{
	double Ra, Dec;
	Equatorial(SunLongitude(Utc), 0, Utc, Ra, Dec);
	const FVector R = EquatorialToRealityKit(EquatorialVector(Ra, Dec), LocalSiderealTime(Utc, Observer.Longitude), Observer.Latitude * Deg);
	return {ToUnreal(R), FMath::Asin(FMath::Clamp(R.Y, -1.0, 1.0))};
}

MuseeEphemeris::FMoon MuseeEphemeris::Moon(const FDateTime& Utc, const FMuseeObserver& Observer)
{
	double Lon, Lat, Ra, Dec;
	MoonEcliptic(Utc, Lon, Lat);
	Equatorial(Lon, Lat, Utc, Ra, Dec);
	const FVector R = EquatorialToRealityKit(EquatorialVector(Ra, Dec), LocalSiderealTime(Utc, Observer.Longitude), Observer.Latitude * Deg);
	double Elong = FMath::Fmod(Lon - SunLongitude(Utc), 2 * UE_DOUBLE_PI);
	if (Elong < 0) { Elong += 2 * UE_DOUBLE_PI; }
	return {ToUnreal(R), FMath::Asin(FMath::Clamp(R.Y, -1.0, 1.0)), (1 - FMath::Cos(Elong)) / 2, Elong < UE_DOUBLE_PI};
}

int32 MuseeEphemeris::SolarTerm(const FDateTime& Utc)
{
	double Lon = FMath::Fmod(SunLongitude(Utc) / Deg - 315.0, 360.0);
	if (Lon < 0) { Lon += 360.0; }
	return static_cast<int32>(Lon / 15.0) % 24;
}

void MuseeEphemeris::SolarTermName(int32 Index, FString& Hanzi, FString& Pinyin, FString& English)
{
	const FTerm& T = Terms[((Index % 24) + 24) % 24];
	Hanzi = T.Hanzi;
	Pinyin = T.Pinyin;
	English = T.English;
}
