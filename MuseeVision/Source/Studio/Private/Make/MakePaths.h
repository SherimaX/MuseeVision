#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/**
	 * Where a make writes, and the names it gives what it writes. Every made asset lives under one root folder:
	 * /Game/Generated for the museum. Names are stable, so a remake replaces a package rather than adding one.
	 */
	struct FMakePaths
	{
		FString Root;

		/** A stage's mesh: <Root>/Stages/<Stage>/SM_<Stage>_<Piece>. */
		FString StageMesh(const FString& Stage, const FString& Piece) const
		{
			return FString::Printf(TEXT("%s/Stages/%s/SM_%s_%s"), *Root, *Stage, *Stage, *Piece);
		}

		/** A stage's level: <Root>/Stages/<Stage>/L_<Stage>. */
		FString StageLevel(const FString& Stage) const
		{
			return FString::Printf(TEXT("%s/Stages/%s/L_%s"), *Root, *Stage, *Stage);
		}

		/** A work's image, as a texture: <Root>/Art/T_<Work>. */
		FString WorkImage(const FString& Work) const
		{
			return FString::Printf(TEXT("%s/Art/T_%s"), *Root, *Work);
		}

		/**
		 * A material instance Studio makes for Matter: <Root>/Matter/<Instance>, which for the museum's root is where
		 * Musee::Matter::Resolve finds it (Musee::Matter::InstancePackage).
		 */
		FString MatterInstance(const FString& Instance) const
		{
			return FString::Printf(TEXT("%s/Matter/%s"), *Root, *Instance);
		}

		/**
		 * A light profile Studio imports for Light from a maker's photometric file: <Root>/Light/Profiles/<Name>, which for
		 * the museum's root is where Light finds it (Musee::Light::ProfilePackage).
		 */
		FString LightProfile(const FString& Name) const
		{
			return FString::Printf(TEXT("%s/Light/Profiles/%s"), *Root, *Name);
		}

		/** A joint's mesh, drawn by Site in the museum's frame and named joint.<port>.<part>: <Root>/Site/Joints/SM_<Piece>. */
		FString JointMesh(const FString& Piece) const
		{
			return FString::Printf(TEXT("%s/Site/Joints/SM_%s"), *Root, *Piece);
		}

		/** The joints' level, which the museum's level instances at its origin: <Root>/Site/L_Joints. */
		FString JointsLevel() const
		{
			return FString::Printf(TEXT("%s/Site/L_Joints"), *Root);
		}

		/** The museum's level: <Root>/Site/L_Museum. */
		FString MuseumLevel() const
		{
			return FString::Printf(TEXT("%s/Site/L_Museum"), *Root);
		}
	};
}
