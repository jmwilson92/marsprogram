"""
ARES — author the stylized base material. CREATES AN ASSET.

WHY THIS EXISTS
  Everything the campus draws without an assigned Fab material goes through one
  base material and a dynamic instance of it. That base was
  /Engine/BasicShapes/BasicShapeMaterial, which exposes a single parameter:
  Color. Flat albedo, fixed roughness, no metallic, no edge response. Under one
  directional sun that reads as matte plastic on every surface in the game at
  once — which is most of what "looks like MS Paint" actually is. Colour-blocking
  the facades does not touch it, because the problem is the shading model, not
  the palette.

  A material is an asset, so it cannot be written in C++. This builds one node
  by node through the editor's own material API.

WHAT IT BUILDS  /Game/Ares/Materials/M_AresTint

  Color        (vector) -> BaseColor, modulated by PerInstanceRandom
  Roughness    (scalar) -> Roughness
  Metallic     (scalar) -> Metallic
  Emissive     (scalar) -> Color * Emissive, into EmissiveColor
  RimStrength  (scalar) -> Fresnel * RimColor, added to EmissiveColor

  The Color name is deliberate: the C++ already sets a parameter by that name,
  so this is a drop-in replacement and the fallback still works if the asset is
  missing. The other parameters are simply ignored on the old base material.

  PerInstanceRandom is the interesting one. Instanced meshes get a stable random
  value per instance, so ±7% brightness variation breaks the dead uniformity of
  ten thousand identical boxes without any per-instance work in C++. Fresnel on
  emissive gives edges a light catch, which is what makes stylized geometry read
  as solid rather than as flat shapes.

HOW TO RUN
  Output Log command box. Which line you need depends on the dropdown to its
  left — see ares_report.py.

  Cmd:
      py "C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_make_materials.py"

  Python:
      exec(open(r"C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_make_materials.py").read())

  Safe to re-run: it rebuilds the material from scratch each time, so tweaking
  the numbers below and running it again is the tuning loop.

  Then rebuild C++ once so ACapeCampus knows to look for it. After that the
  campus picks it up on every construction-script run, no restart.
"""

import unreal

PACKAGE_PATH = "/Game/Ares/Materials"
ASSET_NAME = "M_AresTint"
FULL_PATH = "%s/%s" % (PACKAGE_PATH, ASSET_NAME)

# Defaults. Every one of these is overridden per-surface from C++ at runtime;
# they only decide what the material looks like on its own in the editor.
DEFAULT_COLOR = (0.5, 0.5, 0.52, 1.0)
DEFAULT_ROUGHNESS = 0.72
DEFAULT_METALLIC = 0.0
DEFAULT_EMISSIVE = 0.0

# Instance-to-instance brightness spread. 0.07 is enough to stop a wall of
# boxes reading as one flat plane, and low enough not to look like noise.
VARIATION = 0.07

# Edge light. Colour is a cool sky bounce; strength is deliberately low, since
# this is a shading cue and not a glow effect.
RIM_COLOR = (0.55, 0.68, 0.95, 1.0)
RIM_STRENGTH = 0.10
RIM_EXPONENT = 4.0


def log(message):
    print("[ares-mat] %s" % message)


mel = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary


def make(material, expression_class, x, y, **properties):
    node = mel.create_material_expression(material, expression_class, x, y)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def wire(source, source_output, target, target_input):
    ok = mel.connect_material_expressions(source, source_output, target, target_input)
    if not ok:
        log("!! could not connect %s.%s -> %s.%s"
            % (type(source).__name__, source_output or "(out)",
               type(target).__name__, target_input))
    return ok


def wire_property(source, source_output, material_property, label):
    ok = mel.connect_material_property(source, source_output, material_property)
    if not ok:
        log("!! could not connect %s -> %s" % (type(source).__name__, label))
    return ok


# ------------------------------------------------------------------ build ---

# Rebuilt from scratch rather than edited, so re-running is idempotent instead
# of stacking a second copy of every node onto the existing graph.
if assets.does_asset_exist(FULL_PATH):
    log("replacing existing %s" % FULL_PATH)
    assets.delete_asset(FULL_PATH)

material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    ASSET_NAME, PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew())

if not material:
    log("!! could not create the material asset. Nothing else here will work.")
else:
    color = make(material, unreal.MaterialExpressionVectorParameter, -900, -200,
                 parameter_name="Color",
                 default_value=unreal.LinearColor(*DEFAULT_COLOR))

    roughness = make(material, unreal.MaterialExpressionScalarParameter, -900, 120,
                     parameter_name="Roughness", default_value=DEFAULT_ROUGHNESS)

    metallic = make(material, unreal.MaterialExpressionScalarParameter, -900, 220,
                    parameter_name="Metallic", default_value=DEFAULT_METALLIC)

    emissive = make(material, unreal.MaterialExpressionScalarParameter, -900, 320,
                    parameter_name="Emissive", default_value=DEFAULT_EMISSIVE)

    rim_strength = make(material, unreal.MaterialExpressionScalarParameter, -900, 460,
                        parameter_name="RimStrength", default_value=RIM_STRENGTH)

    rim_color = make(material, unreal.MaterialExpressionVectorParameter, -900, 560,
                     parameter_name="RimColor",
                     default_value=unreal.LinearColor(*RIM_COLOR))

    # --- base colour: Color * lerp(1-VARIATION, 1+VARIATION, PerInstanceRandom)
    per_instance = make(material, unreal.MaterialExpressionPerInstanceRandom, -700, -40)

    spread = make(material, unreal.MaterialExpressionLinearInterpolate, -500, -40,
                  const_a=1.0 - VARIATION, const_b=1.0 + VARIATION)
    wire(per_instance, "", spread, "Alpha")

    base_color = make(material, unreal.MaterialExpressionMultiply, -300, -160)
    wire(color, "", base_color, "A")
    wire(spread, "", base_color, "B")

    # --- emissive: Color * Emissive, plus a fresnel rim
    glow = make(material, unreal.MaterialExpressionMultiply, -500, 320)
    wire(color, "", glow, "A")
    wire(emissive, "", glow, "B")

    fresnel = make(material, unreal.MaterialExpressionFresnel, -700, 520,
                   exponent=RIM_EXPONENT, base_reflect_fraction=0.0)

    rim_tinted = make(material, unreal.MaterialExpressionMultiply, -500, 520)
    wire(fresnel, "", rim_tinted, "A")
    wire(rim_color, "", rim_tinted, "B")

    rim = make(material, unreal.MaterialExpressionMultiply, -350, 520)
    wire(rim_tinted, "", rim, "A")
    wire(rim_strength, "", rim, "B")

    emissive_total = make(material, unreal.MaterialExpressionAdd, -180, 400)
    wire(glow, "", emissive_total, "A")
    wire(rim, "", emissive_total, "B")

    # --- outputs
    wire_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR, "BaseColor")
    wire_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS, "Roughness")
    wire_property(metallic, "", unreal.MaterialProperty.MP_METALLIC, "Metallic")
    wire_property(emissive_total, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR, "EmissiveColor")

    try:
        mel.layout_material_expressions(material)
    except Exception:
        pass

    mel.recompile_material(material)
    assets.save_asset(FULL_PATH)

    log("")
    log("built %s" % FULL_PATH)
    log("parameters: Color, Roughness, Metallic, Emissive, RimStrength, RimColor")

    # ACapeCampus looks for this material every time its construction script
    # runs, so poking the campus makes the change visible now instead of at the
    # next unrelated edit.
    rebuilt = 0
    try:
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        for actor in actors:
            if type(actor).__name__ == "CapeCampus":
                actor.rerun_construction_scripts()
                rebuilt += 1
    except Exception as exc:
        log("could not rebuild the campus automatically (%s)" % exc)

    log("")
    if rebuilt:
        log("rebuilt %d CapeCampus — the change should be visible now." % rebuilt)
        log("If everything still looks matte, the C++ half is not in yet:")
        log("close the editor, run .\\Build.bat, reopen.")
    else:
        log("No CapeCampus rebuilt. Place one, or nudge any property on it.")
    log("SAVE THE LEVEL (Ctrl+S).")
