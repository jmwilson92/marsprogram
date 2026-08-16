"""
ARES — scene and asset report. READ ONLY. Changes nothing.

Run this and paste the output back. It is the closest thing to letting me see
the editor: what is in the level, what the campus art slots are pointing at,
what the sky is set to, and what assets exist to assign.

WHY IT PROBES NAMES INSTEAD OF LISTING THEM
  An earlier version walked dir(actor). That only ever returned inherited
  AActor properties — net_priority, sprite_scale, tags — and not one Ares
  property, so the report looked like the campus had no settings at all.
  Unreal does not put every editable property into dir().

  So this asks for each property by name instead. Unreal renames C++ properties
  on the way to Python (StructureSlot -> structure_slot, bRandomYaw ->
  random_yaw, dropping the bool's leading b), and the exact rule differs by
  version, so each name is tried in several spellings and the one that answers
  is reported. A property printed as MISSING is one Python genuinely cannot
  reach, which is the thing worth knowing — ares_setup_art.py writes through
  this same mechanism and can only assign what shows up here.

HOW TO RUN
  1. Edit -> Plugins -> search "Python" -> enable "Python Editor Script Plugin"
  2. Restart the editor
  3. Window -> Output Log. Look at the dropdown to the LEFT of the command box
     at the bottom: it says either "Cmd" or "Python", and the two take
     different input. Use the line that matches yours.

     Cmd:
         py "C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_report.py"

     Python:
         exec(open(r"C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_report.py").read())

     Crossing them fails in both directions, and neither failure names the real
     problem: `py "..."` in Python mode is a SyntaxError, and raw Python in Cmd
     mode is reported as a deprecated command and silently does nothing.

  Everything prints into the Output Log. Select and copy it.
"""

import re

import unreal

# ------------------------------------------------------------- properties ---

# C++ names, as written in the headers. Spelling for Python is worked out below.
CAMPUS_SLOTS = [
    "StructureSlot", "FurnitureSlot", "ScreenSlot", "SteelBarrelSlot",
    "SteelDetailSlot", "NoseconeSlot", "GroundSlot", "TrunkSlot",
    "CanopySlot", "GroundCoverSlot",
]

CAMPUS_SETTINGS = [
    "bShowSigns", "bInteriorLights", "bBuildInteriors", "bDressExteriors",
    "bBuildLaunchVehicle", "bBuildLandscape", "bEnsureSky",
    "bTrunkSlotIsWholeTree",
    "InteriorLightIntensity", "TreeCount", "GroundTileSizeM",
    "GroundCoverCount", "GroundCoverRadiusM", "GroundCoverCullDistanceM",
    "GroundCoverSizeCm", "BikeCount", "RoadWidth",
]

SKY_SETTINGS = [
    "SunElevationDegrees", "SunAzimuthDegrees", "SunIntensityLux",
    "SunAngularDiameter", "Saturation", "Contrast", "BloomIntensity",
    "VignetteIntensity", "bLockExposure", "LockedExposureEV100",
    "ExposureCompensation", "bVolumetricClouds", "bHeightFog", "FogDensity",
    "FogHeightFalloff", "FogStartDistanceM", "bVolumetricFog",
    "SkyLightIntensity",
]

# Fields inside FCapeMeshSlot, so a slot prints as its contents rather than as
# an opaque <Struct 'CapeMeshSlot'>.
SLOT_FIELDS = ["Mesh", "Material", "Fit", "ExtraScale", "bRandomYaw"]


def python_names(cpp_name):
    """Candidate Python spellings for a C++ property name, best guess first."""
    name = cpp_name
    # Unreal drops the Hungarian b from booleans: bRandomYaw -> random_yaw.
    stripped = name[1:] if len(name) > 1 and name[0] == "b" and name[1].isupper() else name

    def snake(text):
        text = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", text)
        text = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", text)
        return text.lower()

    # dict.fromkeys keeps insertion order and drops duplicates.
    return list(dict.fromkeys([snake(stripped), snake(name), stripped, name]))


def read_property(obj, cpp_name):
    """(value, python_name_that_worked) or (None, None) if nothing resolves."""
    for candidate in python_names(cpp_name):
        try:
            return obj.get_editor_property(candidate), candidate
        except Exception:
            continue
    return None, None


def short(value):
    text = str(value)
    # Asset values print as a long <Object '/Game/...' (0x...) Class '...'>
    # wrapper. The path is the only part worth reading.
    match = re.search(r"'([^']+)'", text)
    if text.startswith("<Object") and match:
        return match.group(1)
    if text.startswith("<Struct 'Vector'"):
        nums = re.findall(r"[xyz]: (-?[\d.]+)", text)
        if len(nums) == 3:
            return "(%s, %s, %s)" % tuple(round(float(n), 3) for n in nums)
    return text if len(text) <= 88 else text[:85] + "..."


def report_properties(obj, cpp_names):
    missing = []
    for cpp_name in cpp_names:
        value, resolved = read_property(obj, cpp_name)
        if resolved is None:
            missing.append(cpp_name)
            print("    %-28s MISSING — not reachable from Python" % cpp_name)
        else:
            print("    %-28s = %s   [%s]" % (cpp_name, short(value), resolved))
    return missing


def report_slot(obj, cpp_name):
    slot, resolved = read_property(obj, cpp_name)
    if resolved is None:
        print("    %-20s MISSING — not reachable from Python" % cpp_name)
        return False

    parts = []
    for field in SLOT_FIELDS:
        value, _ = read_property(slot, field)
        if value is None:
            continue
        text = short(value)
        if field in ("Mesh", "Material"):
            text = text.rsplit("/", 1)[-1] if text != "None" else "-"
        parts.append("%s=%s" % (field.lower(), text))
    print("    %-20s %s" % (cpp_name, "  ".join(parts)))
    return True


def rule(title):
    print("")
    print("=" * 70)
    print(title)
    print("=" * 70)


def get_level_actors():
    """Actor listing moved subsystems in UE5; try the current path then the old one."""
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        pass
    try:
        return unreal.EditorLevelLibrary.get_all_level_actors()
    except Exception as exc:
        print("  !! could not list actors: %s" % exc)
        return []


# ---------------------------------------------------------------- level ---

rule("LEVEL ACTORS")

actors = get_level_actors()
print("total: %d" % len(actors))

by_class = {}
for actor in actors:
    by_class.setdefault(type(actor).__name__, []).append(actor)

for key in sorted(by_class):
    print("  %-40s x%d" % (key, len(by_class[key])))

# ------------------------------------------------------------ the campus ---

rule("CAPE CAMPUS")

campus_list = by_class.get("CapeCampus", [])
unreachable = []

if not campus_list:
    print("  !! NO CapeCampus IN THE LEVEL.")
    print("     Place Actors -> search CapeCampus -> drag in -> location 0,0,0 -> Ctrl+S")
else:
    campus = campus_list[0]
    print("  location: %s" % short(campus.get_actor_location()))

    print("")
    print("  art slots:")
    for slot_name in CAMPUS_SLOTS:
        if not report_slot(campus, slot_name):
            unreachable.append(slot_name)

    print("")
    print("  settings:")
    unreachable += report_properties(campus, CAMPUS_SETTINGS)

    print("")
    buildings, resolved = read_property(campus, "Buildings")
    if resolved is None:
        print("  buildings: MISSING — not reachable from Python")
        unreachable.append("Buildings")
    else:
        print("  buildings: %d" % len(buildings))
        for building in buildings:
            ident, _ = read_property(building, "Id")
            size, _ = read_property(building, "Size")
            print("    %-6s size=%s" % (ident, short(size)))

# --------------------------------------------------------------- the sky ---

rule("CAPE SKY")

sky_list = by_class.get("CapeSky", [])
if not sky_list:
    print("  !! NO CapeSky IN THE LEVEL. The campus spawns one at play time,")
    print("     but the editor viewport stays unlit. Place one and SAVE.")
else:
    unreachable += report_properties(sky_list[0], SKY_SETTINGS)

# -------------------------------------------------------------- content ---

rule("PROJECT CONTENT (/Game)")

try:
    asset_paths = unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False)
except Exception as exc:
    print("  !! could not list assets: %s" % exc)
    asset_paths = []

meshes, materials = [], []

for path in asset_paths:
    clean = path.split(".")[0]
    try:
        asset_class = str(unreal.EditorAssetLibrary.find_asset_data(path).asset_class_path.asset_name)
    except Exception:
        try:
            asset_class = str(unreal.EditorAssetLibrary.find_asset_data(path).asset_class)
        except Exception:
            asset_class = "?"

    if asset_class == "StaticMesh":
        meshes.append(clean)
    elif asset_class in ("Material", "MaterialInstanceConstant"):
        materials.append(clean)

print("static meshes: %d" % len(meshes))
for path in sorted(meshes)[:60]:
    print("  %s" % path)
if len(meshes) > 60:
    print("  ... and %d more" % (len(meshes) - 60))

print("")
print("materials: %d" % len(materials))
for path in sorted(materials)[:60]:
    print("  %s" % path)
if len(materials) > 60:
    print("  ... and %d more" % (len(materials) - 60))

# ------------------------------------------------------------- verdict ---

rule("VERDICT")

if not campus_list:
    print("  Place a CapeCampus before anything else here means much.")
elif unreachable:
    print("  %d propert(ies) are NOT reachable from Python:" % len(unreachable))
    for name in unreachable:
        print("    %s" % name)
    print("")
    print("  ares_setup_art.py writes through the same mechanism, so it cannot")
    print("  assign any slot in that list. This needs a C++ change, not a")
    print("  script change — paste this section back.")
else:
    print("  All properties reachable. ares_setup_art.py can assign every slot.")

rule("END OF REPORT — copy everything above")
