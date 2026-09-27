#include "MatterInstances.h"

#include "Containers/StringConv.h"
#include "Hash/Blake3.h"
#include "MatterColour.h"
#include "MatterMasterGraph.h"
#include "MatterMasterParameters.h"
#include "MatterRecords.h"
#include "Materials/MaterialInstanceConstant.h"

namespace Musee::Matter
{
namespace
{
	using namespace MasterParameter;

	/** The units the masters state, as the typed records write them. */
	constexpr const TCHAR* Fraction = TEXT("1");
	constexpr const TCHAR* Millimeters = TEXT("mm");
	constexpr const TCHAR* PerMillimeter = TEXT("1/mm");
	constexpr const TCHAR* Years = TEXT("year");
	constexpr const TCHAR* PerYear = TEXT("1/year");
	constexpr const TCHAR* Count = TEXT("count");
	constexpr const TCHAR* CieLabUnit = TEXT("CIELAB");

	/** A working that lays a material over a host face (Data/Matter/Shading/Workings, kind). */
	const FName LayerKind(TEXT("layer"));

	/** Writes parameters into an instance, each once, each one the master declares, each from a record field. */
	struct FWriter
	{
		UMaterialInstanceConstant& Instance;
		TSet<FName> Declared;
		TSet<FName> Written;
		TArray<FString>& Problems;
		TArray<FString> Values;

		bool Accept(const TCHAR* Parameter)
		{
			const FName Name(Parameter);
			if (!Declared.Contains(Name))
			{
				Problems.Add(FString::Printf(TEXT("The master declares no parameter %s."), Parameter));
				return false;
			}
			bool bAlready = false;
			Written.Add(Name, &bAlready);
			if (bAlready)
			{
				Problems.Add(FString::Printf(TEXT("The parameter %s is written twice."), Parameter));
			}
			return !bAlready;
		}

		void Scalar(const TCHAR* Parameter, const FMatterQuantity& Quantity, const TCHAR* Unit, const FString& Field)
		{
			if (!Accept(Parameter))
			{
				return;
			}
			if (!Quantity.HasValue())
			{
				Problems.Add(FString::Printf(TEXT("%s gives no value for %s."), *Field, Parameter));
				return;
			}
			if (Quantity.Unit != Unit)
			{
				Problems.Add(FString::Printf(TEXT("%s is in '%s' where %s is in '%s'."), *Field, *Quantity.Unit, Parameter, Unit));
				return;
			}
			const float Value = static_cast<float>(Quantity.Value);
			Instance.SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Parameter), Value);
			Values.Add(FString::Printf(TEXT("%s=%.9g"), Parameter, Value));
		}

		void Colour(const TCHAR* Parameter, const FMatterTriple& Triple, const TCHAR* Unit, const FString& Field)
		{
			if (!Accept(Parameter))
			{
				return;
			}
			const bool bLab = Triple.Space == CieLabSpace;
			if ((bLab && (Triple.Unit != CieLabUnit || FCString::Strcmp(Unit, Fraction) != 0)) || (!bLab && Triple.Unit != Unit))
			{
				Problems.Add(FString::Printf(TEXT("%s is in '%s' (%s) where %s is in '%s'."), *Field, *Triple.Unit, *Triple.Space, Parameter, Unit));
				return;
			}
			const TValueOrError<FLinearColor, FString> Linear = LinearRec709(Triple);
			if (Linear.HasError())
			{
				Problems.Add(FString::Printf(TEXT("%s %s."), *Field, *Linear.GetError()));
				return;
			}
			const FLinearColor Value = Linear.GetValue();
			Instance.SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Parameter), Value);
			Values.Add(FString::Printf(TEXT("%s=%.9g,%.9g,%.9g"), Parameter, Value.R, Value.G, Value.B));
		}
	};

	FString FieldOf(const TCHAR* Folder, const FMuseeKey& Key, const TCHAR* Field)
	{
		return FString::Printf(TEXT("Matter/Shading/%s/%s.json: %s"), Folder, *Key.ToString(), Field);
	}

	void WritePores(FWriter& W, const FMatterSubstance& S)
	{
		W.Scalar(Porosity, S.Dielectric.Porosity, Fraction, FieldOf(TEXT("Substances"), S.Key, TEXT("dielectric.porosity")));
		W.Scalar(WettingDepthMillimeters, S.Dielectric.WettingDepth, Millimeters, FieldOf(TEXT("Substances"), S.Key, TEXT("dielectric.wettingDepth")));
	}

	void WriteHistory(FWriter& W, const FMatterHistory& H)
	{
		const auto Field = [&H](const TCHAR* Name) { return FieldOf(TEXT("Histories"), H.Key, Name); };
		W.Scalar(AgeYears, H.Age, Years, Field(TEXT("age")));
		W.Scalar(PassagesPerYear, H.PassagesPerYear, PerYear, Field(TEXT("passagesPerYear")));
		W.Scalar(SkyExposure, H.SkyExposure, Fraction, Field(TEXT("skyExposure")));
		W.Scalar(KeptWet, H.KeptWet, Fraction, Field(TEXT("keptWet")));
		W.Scalar(DepositionPerYear, H.Deposition, PerYear, Field(TEXT("deposition")));
		W.Scalar(WashRemovalPerYear, H.WashRemoval, PerYear, Field(TEXT("washRemoval")));
		W.Colour(SoilAlbedo, H.SoilAlbedo, Fraction, Field(TEXT("soilAlbedo")));
		W.Scalar(WornRoughness, H.WornRoughness, Fraction, Field(TEXT("wornRoughness")));
		W.Scalar(WornHaziness, H.WornHaziness, Fraction, Field(TEXT("wornHaziness")));
		W.Scalar(WearPassages, H.WearPassages, Count, Field(TEXT("wearPassages")));
	}

	void WriteFaceRoughness(FWriter& W, const FMatterWorking& Wk)
	{
		W.Scalar(Roughness, Wk.Roughness, Fraction, FieldOf(TEXT("Workings"), Wk.Key, TEXT("roughness")));
		W.Scalar(TextureDepthMillimeters, Wk.TextureDepth, Millimeters, FieldOf(TEXT("Workings"), Wk.Key, TEXT("textureDepth")));
	}

	void WriteBody(FWriter& W, const FMatterSubstance& S, const FMatterWorking& Wk, bool bBedded)
	{
		const auto Field = [&S](const TCHAR* Name) { return FieldOf(TEXT("Substances"), S.Key, Name); };
		W.Colour(DiffuseAlbedo, S.Dielectric.DiffuseAlbedo, Fraction, Field(TEXT("dielectric.diffuseAlbedo")));
		W.Scalar(RefractiveIndex, S.Dielectric.RefractiveIndex, Fraction, Field(TEXT("dielectric.refractiveIndex")));
		WritePores(W, S);
		W.Scalar(GrainMillimeters, S.Structure.Grain, Millimeters, Field(TEXT("structure.grain")));
		W.Scalar(BedMillimeters, S.Structure.Bed, Millimeters, Field(TEXT("structure.bed")));
		if (bBedded)
		{
			W.Scalar(BedAspect, S.Structure.BedAspect, Fraction, Field(TEXT("structure.bedAspect")));
		}
		W.Scalar(HurstExponent, S.Structure.Hurst, Fraction, Field(TEXT("structure.hurst")));
		W.Scalar(ToneSpread, S.Structure.ToneSpread, Fraction, Field(TEXT("structure.toneSpread")));
		W.Scalar(RoughnessSpread, S.Structure.RoughnessSpread, Fraction, Field(TEXT("structure.roughnessSpread")));
		WriteFaceRoughness(W, Wk);
		W.Scalar(Haziness, Wk.Haziness, Fraction, FieldOf(TEXT("Workings"), Wk.Key, TEXT("haziness")));
		W.Scalar(ReliefMillimeters, Wk.Relief, Millimeters, FieldOf(TEXT("Workings"), Wk.Key, TEXT("relief")));
	}

	void WriteMedium(FWriter& W, const FMatterSubstance& S, bool bScattering)
	{
		W.Colour(AbsorptionPerMillimeter, S.Medium.Absorption, PerMillimeter, FieldOf(TEXT("Substances"), S.Key, TEXT("medium.absorption")));
		if (bScattering)
		{
			W.Colour(ScatteringPerMillimeter, S.Medium.Scattering, PerMillimeter, FieldOf(TEXT("Substances"), S.Key, TEXT("medium.scattering")));
		}
	}

	/** A metal's face: its reflection, its grain, and the film over it: the working's patina where the working lays one,
	 *  otherwise the substance's own tarnish, grown as far as the history says. */
	void WriteMetal(FWriter& W, const FMatterSubstance& S, const FMatterWorking& Wk, const FMatterHistory& H)
	{
		W.Colour(MetalF0, S.Conductor.F0, Fraction, FieldOf(TEXT("Substances"), S.Key, TEXT("conductor.f0")));
		W.Colour(MetalF82, S.Conductor.F82, Fraction, FieldOf(TEXT("Substances"), S.Key, TEXT("conductor.f82")));
		const bool bPatina = Wk.Film.Cover.HasValue();
		const FMatterFilm& Film = bPatina ? Wk.Film : S.Tarnish;
		const FString Where = bPatina ? FieldOf(TEXT("Workings"), Wk.Key, TEXT("film.")) : FieldOf(TEXT("Substances"), S.Key, TEXT("tarnish."));
		W.Colour(FilmAlbedo, Film.DiffuseAlbedo, Fraction, Where + TEXT("diffuseAlbedo"));
		W.Scalar(FilmRefractiveIndex, Film.RefractiveIndex, Fraction, Where + TEXT("refractiveIndex"));
		W.Scalar(FilmRoughness, Film.Roughness, Fraction, Where + TEXT("roughness"));
		if (bPatina)
		{
			W.Scalar(FilmCover, Wk.Film.Cover, Fraction, Where + TEXT("cover"));
		}
		else
		{
			W.Scalar(FilmCover, H.TarnishCover, Fraction, FieldOf(TEXT("Histories"), H.Key, TEXT("tarnishCover")));
		}
		WriteFaceRoughness(W, Wk);
		W.Scalar(RoughnessAcross, Wk.RoughnessAcross, Fraction, FieldOf(TEXT("Workings"), Wk.Key, TEXT("roughnessAcross")));
	}

	void WriteLeaf(FWriter& W, const FMatterWorking& Gilt)
	{
		const auto Field = [&Gilt](const TCHAR* Name) { return FieldOf(TEXT("Workings"), Gilt.Key, Name); };
		W.Colour(LeafF0, Gilt.Leaf.F0, Fraction, Field(TEXT("leaf.f0")));
		W.Colour(LeafF82, Gilt.Leaf.F82, Fraction, Field(TEXT("leaf.f82")));
		W.Scalar(LeafRoughness, Gilt.Roughness, Fraction, Field(TEXT("roughness")));
		W.Scalar(LeafTextureDepthMillimeters, Gilt.TextureDepth, Millimeters, Field(TEXT("textureDepth")));
	}

	void WriteMedia(FWriter& W, const FSpecRecords& R)
	{
		const FMatterSubstance& S = R.Substance;
		const FMatterWorking& Wk = R.Working;
		const FName Master = R.Master.Master;
		W.Scalar(RefractiveIndex, S.Dielectric.RefractiveIndex, Fraction, FieldOf(TEXT("Substances"), S.Key, TEXT("dielectric.refractiveIndex")));
		WritePores(W, S);
		WriteFaceRoughness(W, Wk);
		if (Master == FName(Masters::Glass))
		{
			WriteMedium(W, S, false);
			W.Scalar(PaneThicknessMillimeters, Wk.PaneThickness, Millimeters, FieldOf(TEXT("Workings"), Wk.Key, TEXT("paneThickness")));
		}
		else if (Master == FName(Masters::Water))
		{
			WriteMedium(W, S, true);
			W.Colour(DyeAbsorptionPerMillimeter, Wk.Medium.Absorption, PerMillimeter, FieldOf(TEXT("Workings"), Wk.Key, TEXT("medium.absorption")));
		}
		else
		{
			W.Colour(PaintWhiteAlbedo, S.Dielectric.DiffuseAlbedo, Fraction, FieldOf(TEXT("Substances"), S.Key, TEXT("dielectric.diffuseAlbedo")));
		}
	}

	/** Every parameter the master shades with, from the spec's records. */
	void WriteAll(FWriter& W, const FSpecRecords& R)
	{
		const FName Master = R.Master.Master;
		const FMatterWorking& Face = R.Host.IsSet() ? R.Host.GetValue() : R.Working;
		if (Master == FName(Masters::Stone) || Master == FName(Masters::Marble))
		{
			WriteBody(W, R.Substance, Face, true);
			if (Master == FName(Masters::Marble))
			{
				WriteMedium(W, R.Substance, true);
			}
		}
		else if (Master == FName(Masters::White))
		{
			WriteBody(W, R.Substance, Face, false);
		}
		else if (Master == FName(Masters::Metal))
		{
			WriteMetal(W, R.Substance, Face, R.History);
		}
		else
		{
			WriteMedia(W, R);
		}
		if (R.Host.IsSet())
		{
			WriteLeaf(W, R.Working);
		}
		WriteHistory(W, R.History);
	}

	TSet<FName> DeclaredParameters(UMaterialInterface& Master)
	{
		TSet<FName> Declared;
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Master.GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			Declared.Add(Info.Name);
		}
		Master.GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			Declared.Add(Info.Name);
		}
		// The piece's own table index comes from its primitive, never from an instance.
		Declared.Remove(FName(PieceTableIndex));
		return Declared;
	}

	TValueOrError<FMatterHistory, FString> ReadRegime(const FMatterSubstance& Substance, const FMatterWorking& Working, const FMuseeKey& Regime)
	{
		// A layer's history is the layer's own (gilt wears through); otherwise the master's.
		const FMuseeKey Scope = Working.Kind == LayerKind ? Working.Key : Substance.Master;
		return ReadHistory(Scope, Regime);
	}
}

FString FMatterSpec::ToString() const
{
	return FString::Printf(TEXT("%s/%s/%s"), *Palette.ToString(), *Working.ToString(), *History.ToString());
}

TValueOrError<FSpecRecords, FString> ReadSpec(const FMatterSpec& Spec)
{
	FSpecRecords Records;
	TValueOrError<FMatterPalette, FString> Palette = ReadPalette();
	if (Palette.HasError())
	{
		return MakeError(Palette.StealError());
	}
	const FMatterPaletteEntry* Entry = Palette.GetValue().Find(Spec.Palette);
	if (!Entry)
	{
		return MakeError(FString::Printf(TEXT("The palette has no entry '%s'."), *Spec.Palette.ToString()));
	}
	Records.Entry = *Entry;
	if (Entry->Substance.IsNone())
	{
		return MakeError(FString::Printf(TEXT("The palette entry '%s' names no typed substance record yet."), *Spec.Palette.ToString()));
	}
	TValueOrError<FMatterSubstance, FString> Substance = ReadSubstance(Entry->Substance);
	TValueOrError<FMatterWorking, FString> Working = ReadWorking(Spec.Working);
	if (Substance.HasError() || Working.HasError())
	{
		return MakeError(Substance.HasError() ? Substance.StealError() : Working.StealError());
	}
	Records.Substance = Substance.StealValue();
	Records.Working = Working.StealValue();
	Records.Master = FMasterKind{ Records.Substance.Master.ToName(), NAME_None };
	if (Records.Working.Kind == LayerKind)
	{
		const FMuseeKey* HostKey = Records.Working.Hosts.Find(Records.Substance.Master.ToString());
		if (!HostKey)
		{
			return MakeError(FString::Printf(TEXT("The working '%s' names no host face for the %s master."), *Spec.Working.ToString(), *Records.Substance.Master.ToString()));
		}
		TValueOrError<FMatterWorking, FString> Host = ReadWorking(*HostKey);
		if (Host.HasError())
		{
			return MakeError(Host.StealError());
		}
		Records.Host = Host.StealValue();
		Records.Master.Layer = Records.Working.Key.ToName();
	}
	if (!AllMasters().Contains(Records.Master))
	{
		return MakeError(FString::Printf(TEXT("No master shades '%s' worked as '%s'."), *Records.Substance.Master.ToString(), *Spec.Working.ToString()));
	}
	TValueOrError<FMatterHistory, FString> History = ReadRegime(Records.Substance, Records.Working, Spec.History);
	if (History.HasError())
	{
		return MakeError(History.StealError());
	}
	Records.History = History.StealValue();
	return MakeValue(MoveTemp(Records));
}

FInstanceFilled FillInstance(const FMatterSpec& Spec, UMaterialInstanceConstant& Instance, TFunctionRef<UMaterialInterface*(const FMasterKind&)> FindMaster)
{
	FInstanceFilled Filled;
	TValueOrError<FSpecRecords, FString> Records = [&Spec, &Filled]()
	{
		const FMuseeDependencySet::FScope Reading(Filled.Inputs);
		return ReadSpec(Spec);
	}();
	if (Records.HasError())
	{
		Filled.Problems.Add(FString::Printf(TEXT("%s: %s"), *Spec.ToString(), *Records.GetError()));
		return Filled;
	}
	const FSpecRecords& R = Records.GetValue();
	Filled.Master = R.Master;
	UMaterialInterface* Master = FindMaster(R.Master);
	if (!Master)
	{
		Filled.Problems.Add(FString::Printf(TEXT("%s: the %s master has not been made."), *Spec.ToString(), *R.Master.Master.ToString()));
		return Filled;
	}
	Instance.SetParentEditorOnly(Master);
	FWriter Writer{ Instance, DeclaredParameters(*Master), {}, Filled.Problems, {} };
	WriteAll(Writer, R);
	for (const FName Declared : Writer.Declared)
	{
		if (!Writer.Written.Contains(Declared))
		{
			Filled.Problems.Add(FString::Printf(TEXT("%s: no record writes the master's parameter %s."), *Spec.ToString(), *Declared.ToString()));
		}
	}
	Filled.Written = FString::Join(Writer.Values, TEXT(";"));
	Instance.PostEditChange();
	Instance.ParameterStateId = InstanceGuid(Filled, Spec, TEXT("Instance.ParameterStateId"));
	StampLightingGuid(Instance, InstanceGuid(Filled, Spec, TEXT("Instance.LightingGuid")));
	return Filled;
}

FGuid InstanceGuid(const FInstanceFilled& Filled, const FMatterSpec& Spec, FStringView Role)
{
	const FString Text = FString::Printf(TEXT("musee.matter.instance filler=%d spec=%s written=%s role=%.*s"),
		InstanceFillerVersion, *Spec.ToString(), *Filled.Written, Role.Len(), Role.GetData());
	const FTCHARToUTF8 Utf8(*Text, Text.Len());
	return FGuid::NewGuidFromHash(FBlake3::HashBuffer(Utf8.Get(), Utf8.Length()));
}
}
