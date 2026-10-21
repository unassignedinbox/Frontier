#=======================================================================================================================================
#                                                    EXPORTVEHICLECURVES.PY
#=======================================================================================================================================
# 🧩 Blender add-on (vehicle plan V1): File ▸ Export ▸ Vehicle Curves (.toml) — selected edge loops of the active mesh as
#    ordered LIMIT points (the subdivided surface the designer sees, not the cage), in metres, one half mirror-snapped.
# ⚠️ Written against the Blender 4.x Python API; NOT run in the Frontier sandbox (no Blender there). The DraftingWorkspace
#    side (DecodeVehicleCurves) is validated with the synthetic sample instead.

bl_info = {
    "name": "Vehicle Curves exporter (Frontier)",
    "author": "Frontier",
    "version": (1, 0, 0),
    "blender": (4, 0, 0),
    "location": "File > Export > Vehicle Curves (.toml)",
    "description": "Selected edge loops → ordered limit points for DraftingWorkspace curve fitting and lofting",
    "category": "Import-Export",
}

#-----------------------------------------------------------------------------------------------------------------------
#                                                    IMPORTS
#-----------------------------------------------------------------------------------------------------------------------

from collections import deque

import bmesh
import bpy
from bpy.props import BoolProperty, FloatProperty, StringProperty
from bpy_extras.io_utils import ExportHelper

#-----------------------------------------------------------------------------------------------------------------------
#                                                    CHAINS
#-----------------------------------------------------------------------------------------------------------------------

def CollectChains(Mesh):
    """Selected edges → ordered vertex chains. Open chains start at a vertex of selected degree 1; the rest are closed loops.
       Branching selections (a vertex with 3+ selected edges) are refused: the caller must pick loops one by one."""
    Neighbours = {}
    for Edge in Mesh.edges:
        if not Edge.select:
            continue
        First, Second = Edge.verts[0].index, Edge.verts[1].index
        Neighbours.setdefault(First, []).append(Second)
        Neighbours.setdefault(Second, []).append(First)
    for Vertex, Linked in Neighbours.items():
        if len(Linked) > 2:
            raise ValueError(f"vertex {Vertex} joins {len(Linked)} selected edges — select single loops (no branches)")
    Visited, Chains = set(), []
    Starts = [Vertex for Vertex, Linked in Neighbours.items() if len(Linked) == 1] + list(Neighbours)
    for Start in Starts:
        if Start in Visited:
            continue
        Chain, Previous, Current = [Start], None, Start
        Visited.add(Start)
        while True:
            Onward = [Vertex for Vertex in Neighbours[Current] if Vertex != Previous and Vertex not in Visited]
            if not Onward:
                break
            Previous, Current = Current, Onward[0]
            Chain.append(Current)
            Visited.add(Current)
        Closed = len(Neighbours[Start]) == 2 and Start in Neighbours[Chain[-1]] and len(Chain) > 2
        Chains.append((Chain, Closed))
    return Chains


def SubdividedPath(Adjacency, Source, Target, Steps):
    """Shortest path Source → Target in the evaluated (subdivided) mesh, at most Steps edges: along one cage edge of a
       Catmull–Clark grid it is the straight run of 2^level edges (unique), i.e. the limit points between two cage vertices."""
    Parents, Frontier = {Source: None}, deque([(Source, 0)])
    while Frontier:
        Vertex, Depth = Frontier.popleft()
        if Vertex == Target:
            break
        if Depth == Steps:
            continue
        for Next in Adjacency[Vertex]:
            if Next not in Parents:
                Parents[Next] = Vertex
                Frontier.append((Next, Depth + 1))
    if Target not in Parents:
        return None
    Path, Vertex = [], Target
    while Vertex is not None:
        Path.append(Vertex)
        Vertex = Parents[Vertex]
    return Path[::-1]

#-----------------------------------------------------------------------------------------------------------------------
#                                                    OPERATOR
#-----------------------------------------------------------------------------------------------------------------------

class ExportVehicleCurves(bpy.types.Operator, ExportHelper):
    """Export the selected edge loops of the active mesh as Vehicle Curves (limit points, metres)"""
    bl_idname    = "export_scene.vehicle_curves"
    bl_label     = "Export Vehicle Curves"
    filename_ext = ".toml"
    filter_glob: StringProperty(default="*.toml", options={'HIDDEN'})
    LimitPoints: BoolProperty(name="Limit points", default=True, description="Points on the subdivided surface (Subdivision modifier) instead of the cage")
    MirrorSnap: FloatProperty(name="Mirror snap", default=0.0005, min=0.0, unit='LENGTH', description="Points with |x| below this are moved onto x = 0")
    NamePrefix: StringProperty(name="Name prefix", default="Loop")

    def execute(self, Context):
        Object = Context.active_object
        if Object is None or Object.type != 'MESH':
            self.report({'ERROR'}, "Active object is not a mesh")
            return {'CANCELLED'}
        WasEditing = Object.mode == 'EDIT'
        if WasEditing:
            bpy.ops.object.mode_set(mode='OBJECT')                                              # Flush the edit selection
        try:
            Cage = bmesh.new()
            Cage.from_mesh(Object.data)
            Cage.verts.ensure_lookup_table()
            Chains = CollectChains(Cage)
            if not Chains:
                raise ValueError("no selected edges")
            Matrix  = Object.matrix_world
            Scale   = Context.scene.unit_settings.scale_length                                  # Blender unit → metres
            Points  = {Vertex.index: Matrix @ Vertex.co for Vertex in Cage.verts}
            Between = None
            Levels  = max([Modifier.levels for Modifier in Object.modifiers if Modifier.type == 'SUBSURF' and Modifier.show_viewport] or [0])
            if self.LimitPoints and Levels > 0:
                # 📝 Assumption (checked below): the Subdivision modifier keeps the cage vertices first in the evaluated mesh.
                Evaluated = Object.evaluated_get(Context.evaluated_depsgraph_get()).to_mesh()
                Adjacency = [[] for _ in Evaluated.vertices]
                for Edge in Evaluated.edges:
                    Adjacency[Edge.vertices[0]].append(Edge.vertices[1])
                    Adjacency[Edge.vertices[1]].append(Edge.vertices[0])
                Limit = [Matrix @ Vertex.co for Vertex in Evaluated.vertices]
                if len(Limit) < len(Cage.verts):
                    raise ValueError("evaluated mesh has fewer vertices than the cage (unexpected modifier stack)")
                Points  = {Index: Limit[Index] for Index in Points}
                Between = (Adjacency, Limit, 2 ** Levels)
            Curves = []
            for Ordinal, (Chain, Closed) in enumerate(Chains, start=1):
                Ring = Chain + [Chain[0]] if Closed else Chain
                Positions = [Points[Ring[0]]]
                for Start, End in zip(Ring, Ring[1:]):
                    if Between:
                        Path = SubdividedPath(Between[0], Start, End, Between[2])
                        if Path is None:
                            raise ValueError(f"cage edge {Start}–{End} has no subdivided path (vertex order assumption failed)")
                        Positions += [Between[1][Vertex] for Vertex in Path[1:]]
                    else:
                        Positions.append(Points[End])
                if Closed:
                    Positions.pop()                                                             # The closed flag carries the wrap
                Snapped = []
                for Position in Positions:
                    X, Y, Z = (Coordinate * Scale for Coordinate in Position)
                    Snapped.append((0.0 if abs(X) <= self.MirrorSnap else X, Y, Z))
                Curves.append((f"{self.NamePrefix}{Ordinal}", Closed, Snapped))
            with open(self.filepath, "w", encoding="utf-8") as Stream:
                Stream.write(f'# Exported by ExportVehicleCurves.py from "{Object.name}"\nFormat = "VehicleCurves"\nUnits  = "m"\n')
                Stream.write(f'Source = "Blender {bpy.app.version_string} · {Object.name} · {"limit" if Between else "cage"} points"\n\n')
                for Name, Closed, Positions in Curves:
                    Stream.write(f'[[Curves]]\nName   = "{Name}"\nClosed = {"true" if Closed else "false"}\nPoints = [\n')
                    for X, Y, Z in Positions:
                        Stream.write(f"    [{X!r}, {Y!r}, {Z!r}],\n")
                    Stream.write("]\n\n")
            self.report({'INFO'}, f"Exported {len(Curves)} curve(s) to {self.filepath}")
            return {'FINISHED'}
        except ValueError as Failure:
            self.report({'ERROR'}, str(Failure))
            return {'CANCELLED'}
        finally:
            if WasEditing:
                bpy.ops.object.mode_set(mode='EDIT')

#-----------------------------------------------------------------------------------------------------------------------
#                                                    REGISTRATION
#-----------------------------------------------------------------------------------------------------------------------

def DrawMenuEntry(self, Context):
    self.layout.operator(ExportVehicleCurves.bl_idname, text="Vehicle Curves (.toml)")


def register():
    bpy.utils.register_class(ExportVehicleCurves)
    bpy.types.TOPBAR_MT_file_export.append(DrawMenuEntry)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(DrawMenuEntry)
    bpy.utils.unregister_class(ExportVehicleCurves)


if __name__ == "__main__":
    register()
