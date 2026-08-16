"""
ARES — wire project assets into the campus art slots. THIS MODIFIES THE LEVEL.

Scans /Game for static meshes and materials, scores each against keywords for
every slot, and assigns the best match. What would otherwise be ten manual
drag-and-drops becomes one run, and it is repeatable after importing more Fab
content.

Run ares_report.py FIRST. If the report says you have no meshes or materials,
this has nothing to work with — import from Fab before running it.

HOW TO RUN
  Output Log command box. Which line you need depends on the dropdown to its
  left — see ares_report.py for why crossing them fails confusingly.

  Cmd:
      py "C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_setup_art.py"

  Python:
      exec(open(r"C:/Users/laugh/Projects/marsprogram/Tools/ue_python/ares_setup_art.py").read())

  Then SAVE THE LEVEL (Ctrl+S). Nothing here is saved for you — an unsaved
  crash would throw all of it away, which has already happened once.

TUNING
  Edit SLOT_RULES below. Each entry is (slot property, mesh keywords, material
  keywords). Keywords are matched case-insensitively against the asset path, so
  "concrete" matches /Game/Megascans/Surfaces/Concrete_Rough_xyz.
"""

import unreal

# ---------------------------------------------------------------- config ---

# (slot_name, mesh keywords, material keywords)
# Empty mesh keywords means "material only" — the layout keeps its box shape and
# just gets a surface, which is correct for walls, floors and ground.
SLOT_RULES = [
    ("structure_slot",    [],                                  ["concrete", "plaster", "wall", "painted", "metal_panel"]),
    ("furniture_slot",    ["desk", "console", "table", "rack"], ["metal", "plastic", "painted"]),
    ("screen_slot",       [],                                  ["emissive", "screen", "monitor", "glow"]),
    ("steel_barrel_slot", ["cylinder", "tank", "pipe", "barrel"], ["steel", "metal", "brushed", "aluminum", "aluminium"]),
    ("steel_detail_slot", [],                                  ["steel", "metal", "brushed", "aluminum", "aluminium"]),
    ("nosecone_slot",     ["cone"],                            ["steel", "metal", "brushed"]),
    ("ground_slot",       [],                                  ["grass", "ground", "dirt", "soil", "turf", "lawn"]),
    ("trunk_slot",        ["tree", "trunk", "pine", "oak", "palm"], []),
    ("canopy_slot",       ["leaf", "leaves", "canopy", "foliage", "branch"], []),
    ("ground_cover_slot", ["grass", "clump", "weed", "fern", "plant"], []),
]

# Paths containing these are never considered. Engine primitives are our
# fallback already, and picking one here would be a no-op that looks like a win.
EXCLUDE = ["/Engine/", "BasicShapes", "_Inst_Preview", "/Developers/"]


# --------------------------------------------------------------- helpers ---

def log(message):
    print("[ares-art] %s" % message)


def get_level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        pass
    try:
        return unreal.EditorLevelLibrary.get_all_level_actors()
    except Exception as exc:
        log("!! cannot list actors: %s" % exc)
        return []


def asset_class_name(path):
    try:
        data = unreal.EditorAssetLibrary.find_asset_data(path)
    except Exception:
        return "?"
    try:
        return str(data.asset_class_path.asset_name)
    except Exception:
        try:
            return str(data.asset_class)
        except Exception:
            return "?"


def collect_assets():
    """Returns (meshes, materials) as lists of object paths, engine content excluded."""
    try:
        paths = unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False)
    except Exception as exc:
        log("!! cannot list assets: %s" % exc)
        return [], []

    meshes, materials = [], []
    for path in paths:
        if any(token in path for token in EXCLUDE):
            continue
        clean = path.split(".")[0]
        kind = asset_class_name(path)
        if kind == "StaticMesh":
            meshes.append(clean)
        elif kind in ("Material", "MaterialInstanceConstant"):
            materials.append(clean)
    return meshes, materials


def best_match(candidates, keywords):
    """
    Highest-scoring asset for a keyword set, or None.

    Scored rather than first-match: a path can contain several keywords, and the
    one that matches most is almost always the one you meant. Shorter paths win
    ties, because a deeply nested variant is usually a detail asset.
    """
    if not keywords or not candidates:
        return None

    best, best_score = None, 0
    for path in candidates:
        lowered = path.lower()
        score = sum(1 for word in keywords if word.lower() in lowered)
        if score == 0:
            continue
        if score > best_score or (score == best_score and best and len(path) < len(best)):
            best, best_score = path, score
    return best


def set_slot(campus, slot_name, mesh_path, material_path):
    """Reads the slot struct, fills what we found, writes it back."""
    try:
        slot = campus.get_editor_property(slot_name)
    except Exception as exc:
        log("!! no property '%s' on CapeCampus (%s)" % (slot_name, exc))
        return False

    changed = []

    if mesh_path:
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if mesh:
            slot.set_editor_property("mesh", mesh)
            changed.append("mesh=%s" % mesh_path.rsplit("/", 1)[-1])

    if material_path:
        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if material:
            slot.set_editor_property("material", material)
            changed.append("material=%s" % material_path.rsplit("/", 1)[-1])

    if not changed:
        log("%-20s no match" % slot_name)
        return False

    # Structs are returned by value, so the modified copy has to be written back
    # or nothing happens — a silent no-op that looks exactly like success.
    campus.set_editor_property(slot_name, slot)
    log("%-20s %s" % (slot_name, "  ".join(changed)))
    return True


# ------------------------------------------------------------------ main ---

log("scanning /Game ...")
mesh_paths, material_paths = collect_assets()
log("found %d static meshes, %d materials (engine content excluded)"
    % (len(mesh_paths), len(material_paths)))

if not mesh_paths and not material_paths:
    log("")
    log("NOTHING TO ASSIGN. Import assets from Fab first:")
    log("  Window -> Fab -> sign in -> pick a surface or prop -> Add To Project")
    log("Then run this again.")
else:
    campuses = [a for a in get_level_actors() if type(a).__name__ == "CapeCampus"]
    if not campuses:
        log("!! no CapeCampus in the level. Place one, set it to 0,0,0, then re-run.")
    else:
        campus = campuses[0]
        log("")
        assigned = 0
        for slot_name, mesh_words, material_words in SLOT_RULES:
            mesh = best_match(mesh_paths, mesh_words)
            material = best_match(material_paths, material_words)
            if set_slot(campus, slot_name, mesh, material):
                assigned += 1

        # Force the construction script to re-run so the change is visible now.
        try:
            campus.rerun_construction_scripts()
        except Exception:
            pass

        log("")
        log("assigned %d of %d slots." % (assigned, len(SLOT_RULES)))
        log("SAVE THE LEVEL NOW (Ctrl+S) — none of this is saved automatically.")
