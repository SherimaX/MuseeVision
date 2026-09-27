#pragma once

#include "CoreMinimal.h"
#include "Make/MakeReport.h"

class UTextureLightProfile;

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/**
	 * Imports a maker's photometric file (an IESNA LM-63 file's bytes) into a light profile through the engine's own IES
	 * import: the engine's IES converter (the IESFile module the texture factory runs) resamples the candela table onto
	 * its 256 by 256 grid, normalised to its greatest value, and the profile takes the settings the factory gives an IES
	 * file (half-float RGBA source, HDR, clamped, no mips, the IES light-profile group, its brightness the table's
	 * greatest candela value and its texture multiplier the converter's). Nothing about the import's time or place is
	 * kept, so the same bytes make the same profile. False, with the converter's reason, for a file it can't read.
	 */
	bool ImportLightProfile(const TArray<uint8>& FileBytes, UTextureLightProfile& Profile, FString& OutWhy);

	/**
	 * Makes the light profile of every catalogue type lit from its maker's photometric file (Light's catalogue, kept in
	 * Data/Light/Photometry), at <Root>/Light/Profiles/<name>, which for the museum's root is where Light finds it
	 * (Musee::Light::ProfilePackage). A profile whose file is unchanged is left as it is, so a second make writes
	 * nothing; one that can't be made is a finding. Returns the fingerprint of the profiles made, for the levels that
	 * place the luminaires lit by them.
	 */
	FString MakeLightProfiles(const FStageMakeOptions& Options, FMakeReport& Report);
}
