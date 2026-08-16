"""
ARES — scene and asset report. READ ONLY. Changes nothing.

Run this and paste the output back. It is the closest thing to letting me see
the editor: what is in the level, what assets exist, what the campus art slots
are currently pointing at, and what the sky is set to.

It also INTROSPECTS property names rather than assuming them. Unreal's Python
bindings rename C++ properties (StructureSlot -> structure_slot, bRandomYaw ->
possibly random_yaw or b_random_yaw depending on version), and guessing those
wrong is the main way an editor script fails. This prints the real names so the
setup script can use them with confidence.

HOW TO RUN
  1. Edit -> Plugins -> search "Python" -> enable "Python Editor Script Plugin"
  2. Restart the editor
  3. Window -> Output Log. In the command box at the bottom, type:

         py "C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_report.py"

     `py` runs a Python file and works in the default "Cmd" mode, so there is
     no dropdown to switch. Pasting raw Python into Cmd mode does NOT work —
     the console reports it as a deprecated command and does nothing.

  Everything prints into the Output Log. Select and copy it.
"""

import unreal


def rule(title):
    print("")
    print("=" * 70)
    print(title)
    print("=" * 70)


def get_level_actors():
    """Actor listing moved subsystems in UE5; try the current path then the old one."""
    try:
        subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        return subsystem.get_all_level_actors()
    except Exception:
        pass
    try:
        return unreal.EditorLevelLibrary.get_all_level_actors()
    except Exception as exc:
        print("  !! could not list actors: %s" % exc)
        return []


def describe_properties(obj, label):
    """Prints the editor-property names Python actually exposes on an object."""
    print("  %s (%s)" % (label, type(obj).__name__))
    names = []
    for name in dir(obj):
        if name.startswith("_"):
            continue
        try:
            obj.get_editor_property(name)
        except Exception:
            continue
        names.append(name)
    if not names:
        print("    (no readable editor properties found)")
        return
    for name in sorted(names):
        try:
            value = obj.get_editor_property(name)
        except Exception:
            continue
        text = str(value)
        if len(text) > 90:
            text = text[:87] + "..."
        print("    %-34s = %s" % (name, text))


# ---------------------------------------------------------------- level ---

rule("LEVEL ACTORS")

actors = get_level_actors()
print("total: %d" % len(actors))

by_class = {}
for actor in actors:
    key = type(actor).__name__
    by_class.setdefault(key, []).append(actor)

for key in sorted(by_class):
    print("  %-40s x%d" % (key, len(by_class[key])))

# ------------------------------------------------------------ the campus ---

rule("CAPE CAMPUS")

campus_list = by_class.get("CapeCampus", [])
if not campus_list:
    print("  !! NO CapeCampus IN THE LEVEL.")
    print("     Place Actors -> search CapeCampus -> drag in -> set location 0,0,0 -> Ctrl+S")
else:
    campus = campus_list[0]
    print("  location: %s" % campus.get_actor_location())
    print("")
    describe_properties(campus, "properties")

# --------------------------------------------------------------- the sky ---

rule("CAPE SKY")

sky_list = by_class.get("CapeSky", [])
if not sky_list:
    print("  !! NO CapeSky IN THE LEVEL. The campus spawns one at play time,")
    print("     but the editor viewport stays unlit. Place one and SAVE.")
else:
    describe_properties(sky_list[0], "properties")

# -------------------------------------------------------------- content ---

rule("PROJECT CONTENT (/Game)")

try:
    asset_paths = unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False)
except Exception as exc:
    print("  !! could not list assets: %s" % exc)
    asset_paths = []

meshes = []
materials = []
other = []

for path in asset_paths:
    clean = path.split(".")[0]
    try:
        asset_class = unreal.EditorAssetLibrary.find_asset_data(path).asset_class_path.asset_name
    except Exception:
        # Older builds expose asset_class as a plain name.
        try:
            asset_class = str(unreal.EditorAssetLibrary.find_asset_data(path).asset_class)
        except Exception:
            asset_class = "?"

    text = str(asset_class)
    if text == "StaticMesh":
        meshes.append(clean)
    elif text in ("Material", "MaterialInstanceConstant"):
        materials.append(clean)
    else:
        other.append("%-30s %s" % (text, clean))

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

print("")
print("other assets: %d" % len(other))
for line in sorted(other)[:30]:
    print("  %s" % line)
if len(other) > 30:
    print("  ... and %d more" % (len(other) - 30))

rule("END OF REPORT — copy everything above")
