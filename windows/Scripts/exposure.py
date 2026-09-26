"""
The museum's exposure: re-apply the metering of setup_project.py (EXPOSURE_*) and the exposure
compensation curve below to the Museum map's post-process volume, without rebuilding the materials
or the scaffolding:

  UnrealEditor-Cmd MuseeVision.uproject -run=pythonscript -script=<repo>/windows/Scripts/exposure.py

The curve makes the sky-lit rooms high-key, as the renderings (and a photographer) expose them: a
view that meters very bright is nearly always one filled by a velarium, a laylight or the misty
glass, which would otherwise be pulled down to mid-grey and take the room with it. Lamplit and
night views (EV100 10 and below) are left alone.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import setup_project as S  # noqa: E402

# Average scene EV100 → exposure compensation (stops), on top of EXPOSURE_BIAS.
# Direct sun in a view (the Chinese court, the Rotunda under its eye, EV100 14 and up) is held back a
# little: on top of the global bias it would wash out.
EXPOSURE_CURVE = [(0.0, 0.0), (10.0, 0.0), (11.5, 0.4), (12.5, 0.8), (13.5, 0.4), (14.5, -0.3), (15.5, -0.6), (16.5, -0.7), (18.0, -0.7)]
CURVE_NAME = "C_ExposureCompensation"

# Exposure zones: open courts meter like the sky-lit rooms (EV100 12–13) but must not get their
# high-key boost, which washes sunlit stone out. A volume over each wing (its actors' bounds, grown a
# little) sets its own bias and drops the curve while the visitor is inside, blending at the edges.
# (Albion: its glass court on the south door, in the Chinese Wing's place, and metered as that open court was; its lamps'
# volume, AAlbionLamps, restores the museum's bias at night.)
EXPOSURE_ZONES = {"Albion": -0.2}
ZONE_TAG = "musee.exposure_zone"


def make_curve():
    csv = os.path.join(unreal.Paths.project_saved_dir(), CURVE_NAME + ".csv")
    with open(csv, "w", encoding="utf-8") as f:
        f.write("EV100,Stops\n")
        for ev, stops in EXPOSURE_CURVE:
            f.write(f"{ev},{stops}\n")
    factory = unreal.CSVImportFactory()
    settings = factory.get_editor_property("automated_import_settings")
    settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_CURVE_FLOAT)
    factory.set_editor_property("automated_import_settings", settings)
    task = unreal.AssetImportTask()
    task.filename = csv
    task.destination_path = P.MATERIALS
    task.destination_name = CURVE_NAME
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.factory = factory
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.load_asset(f"{P.MATERIALS}/{CURVE_NAME}")


def apply(ppv, curve):
    s = ppv.settings
    for name, value in (("auto_exposure_bias", S.EXPOSURE_BIAS),
                        ("histogram_log_min", 0.0), ("histogram_log_max", 18.0),
                        ("auto_exposure_min_brightness", S.EXPOSURE_MIN_EV),
                        ("auto_exposure_max_brightness", S.EXPOSURE_MAX_EV),
                        ("auto_exposure_low_percent", S.EXPOSURE_LOW_PERCENT),
                        ("auto_exposure_high_percent", S.EXPOSURE_HIGH_PERCENT),
                        ("white_temp", S.WHITE_TEMP),
                        ("local_exposure_highlight_contrast_scale", S.LOCAL_HIGHLIGHTS),
                        ("local_exposure_shadow_contrast_scale", S.LOCAL_SHADOWS)):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    if curve:
        s.set_editor_property("override_auto_exposure_bias_curve", True)
        s.set_editor_property("auto_exposure_bias_curve", curve)
    ppv.set_editor_property("settings", s)


def wing_bounds(eas, wing):
    lo, hi = None, None
    for a in eas.get_all_level_actors():
        tags = [str(t) for t in a.tags]
        if f"musee.wing:{wing}" not in tags or "musee.retired" in tags:
            continue
        o, e = a.get_actor_bounds(False)
        if e.x <= 0 or e.x > 5000 or e.y > 5000:
            continue
        a0, a1 = o - e, o + e
        lo = a0 if lo is None else unreal.Vector(min(lo.x, a0.x), min(lo.y, a0.y), min(lo.z, a0.z))
        hi = a1 if hi is None else unreal.Vector(max(hi.x, a1.x), max(hi.y, a1.y), max(hi.z, a1.z))
    return lo, hi


def make_zones(eas):
    for a in eas.get_all_level_actors():
        tags = [str(t) for t in a.tags]
        if ZONE_TAG in tags and "musee.native" not in tags:   # native.py's zones (the Reserve's white balance) are its own
            eas.destroy_actor(a)
    for wing, bias in EXPOSURE_ZONES.items():
        lo, hi = wing_bounds(eas, wing)
        if lo is None:
            unreal.log_warning(f"[exposure] no actors for {wing}")
            continue
        centre = (lo + hi) * 0.5
        size = hi - lo
        ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, centre, unreal.Rotator(0, 0, 0))
        ppv.set_actor_label(f"Exposure zone {wing}")
        ppv.tags = [unreal.Name(ZONE_TAG)]
        ppv.set_editor_property("unbound", False)
        ppv.set_editor_property("priority", 10.0)
        ppv.set_editor_property("blend_radius", 300.0)
        # The spawned volume's brush is a 200 cm cube: scale it to the wing.
        ppv.set_actor_scale3d(unreal.Vector(size.x / 200.0 + 0.02, size.y / 200.0 + 0.02, size.z / 200.0 + 0.05))
        s = ppv.settings
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_bias", bias)
        s.set_editor_property("override_auto_exposure_bias_curve", True)
        s.set_editor_property("auto_exposure_bias_curve", None)
        ppv.set_editor_property("settings", s)
        o, e = ppv.get_actor_bounds(False)
        unreal.log(f"[exposure] zone {wing}: bias {bias}, no curve, box {e.x * 2 / 100:.1f} x {e.y * 2 / 100:.1f} x {e.z * 2 / 100:.1f} m")


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level(P.MAP_PATH)
    curve = make_curve()
    unreal.log(f"[exposure] curve {curve.get_path_name() if curve else 'NOT MADE'}")
    for ppv in [a for a in eas.get_all_level_actors()
                if isinstance(a, unreal.PostProcessVolume) and ZONE_TAG not in [str(t) for t in a.tags]]:
        apply(ppv, curve)
        settings = ppv.get_editor_property("settings")
        for name, value in S.RAY_TRACING.items():
            settings.set_editor_property("override_" + name, True)
            settings.set_editor_property(name, value)
        ppv.set_editor_property("settings", settings)
        unreal.log(f"[exposure] {ppv.get_actor_label()}: bias {S.EXPOSURE_BIAS}, EV {S.EXPOSURE_MAX_EV} max, metering "
                   f"{S.EXPOSURE_LOW_PERCENT}–{S.EXPOSURE_HIGH_PERCENT} %, curve {EXPOSURE_CURVE}")
    make_zones(eas)
    les.save_current_level()


if __name__ == "__main__":
    main()
