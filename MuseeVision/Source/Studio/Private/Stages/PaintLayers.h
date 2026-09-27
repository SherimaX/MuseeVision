#pragma once

#include "CoreMinimal.h"
#include "Make/MakeReport.h"
#include "MuseeStageDresser.h"

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/**
	 * Makes each hung work's paint layer before a stage's pieces are cast, since its face resolves to it (Art's
	 * FArtPaintLayer, D-015: Art and Matter define, Studio writes):
	 * - the work's image as a texture, <Root>/Art/T_<Work>, sRGB, its source's id the hash of its pixels and its
	 *   lighting GUID derived from its inputs, so the same image makes the same bytes;
	 * - the work's paint instance, `paint.<id>` with the paint's working and history, at <Root>/Matter/MI_<...>, which
	 *   for the museum is Musee::Matter::InstancePackage, where Musee::Matter::Resolve finds it. Matter composes it
	 *   (Musee::Matter::ComposePaintLayer) over the paint master, from the paint's palette finish, the texture and the
	 *   image's size in metres.
	 * Only what changed is written. A layer that can't be composed (no paint master made, an image that can't be read, a
	 * palette finish Matter refuses) is a finding.
	 */
	void MakePaintLayers(const FMuseeStageContext& Stage, const FStageMakeOptions& Options, FMakeReport& Report);
}
