#!/usr/bin/env python3
# ==================================================================================================================================
#                                                     EXCHANGECROSSCHECK.PY
# ==================================================================================================================================
# 🧩 Independent read-back of the exchange files written by ExchangeValidation --files: STL through trimesh (closure, winding,
#    Euler number, volume against the kernel's exact mass properties) and a shaded gallery rendered from what trimesh read.
#
#    python3 ExchangeCrossCheck.py --files <folder> --report ExchangeValidation.md --markdown <out.md> --gallery <out.png>

import argparse
import os
import re
import sys

import numpy
import trimesh

# ------------------------------------------------------------------------------------------------------------------------
#                                                    STL
# ------------------------------------------------------------------------------------------------------------------------

def ReadExactVolumes(ReportPath):
    # EX2 rows: "| 🟢 | EX2 | Block | V facets 24 vs exact 24: …"
    Volumes = {}
    for Line in open(ReportPath, encoding='utf-8'):
        Match = re.match(r'\| \S+ \| EX2 \| (\w+) \| V facets [\d.e+-]+ vs exact ([\d.e+-]+):', Line)
        if Match:
            Volumes[Match.group(1)] = float(Match.group(2))
    return Volumes

def CrossCheckStl(Folder, Volumes):
    Rows = []
    for Name in sorted(Volumes):
        Path = os.path.join(Folder, Name + '.stl')
        if not os.path.exists(Path):
            Rows.append((Name, None))
            continue
        Model = trimesh.load(Path, file_type='stl', process=True)
        Components = len(Model.split(only_watertight=False))
        Rows.append((Name, {
            'Faces': len(Model.faces), 'Vertices': len(Model.vertices), 'Watertight': Model.is_watertight, 'Winding': Model.is_winding_consistent,
            'Euler': Model.euler_number, 'Components': Components, 'Volume': Model.volume, 'Exact': Volumes[Name], 'Area': Model.area, 'Model': Model }))
    return Rows

# ------------------------------------------------------------------------------------------------------------------------
#                                                    GALLERY
# ------------------------------------------------------------------------------------------------------------------------

def RenderGallery(Rows, Names, Path):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as Pyplot
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    Chosen = [(Name, Values) for Name, Values in Rows if Name in Names and Values]
    Columns = 4
    Lines = (len(Chosen) + Columns - 1) // Columns
    Figure = Pyplot.figure(figsize=(4.0 * Columns, 4.0 * Lines), dpi=110)
    Light = numpy.array([0.45, -0.35, 0.82])
    Light /= numpy.linalg.norm(Light)
    for Index, (Name, Values) in enumerate(Chosen):
        Model = Values['Model']
        Axes = Figure.add_subplot(Lines, Columns, Index + 1, projection='3d')
        Shade = numpy.clip(Model.face_normals @ Light, 0.0, 1.0) * 0.65 + 0.3
        Colours = numpy.stack([Shade * 0.62, Shade * 0.70, Shade * 0.80, numpy.ones_like(Shade)], axis=1)
        Collection = Poly3DCollection(Model.triangles, facecolors=Colours, edgecolors=(0.1, 0.1, 0.1, 0.12), linewidths=0.15)
        Axes.add_collection3d(Collection)
        Low, High = Model.bounds
        Centre, Half = (Low + High) / 2.0, (High - Low).max() / 2.0
        Axes.set_xlim(Centre[0] - Half, Centre[0] + Half)
        Axes.set_ylim(Centre[1] - Half, Centre[1] + Half)
        Axes.set_zlim(Centre[2] - Half, Centre[2] + Half)
        Axes.set_box_aspect((1, 1, 1))
        Axes.view_init(elev=28, azim=-58)
        Axes.set_axis_off()
        Axes.set_title('%s\n%d facets · χ %d · %s' % (Name, Values['Faces'], Values['Euler'], 'watertight' if Values['Watertight'] else 'OPEN'), fontsize=10)
    Figure.suptitle('STL files read back by trimesh %s (shaded from the facets it read)' % trimesh.__version__, fontsize=12)
    Figure.tight_layout()
    Figure.savefig(Path)

# ------------------------------------------------------------------------------------------------------------------------
#                                                    MAIN
# ------------------------------------------------------------------------------------------------------------------------

def Main():
    Parser = argparse.ArgumentParser()
    Parser.add_argument('--files', required=True)
    Parser.add_argument('--report', required=True)
    Parser.add_argument('--markdown', required=True)
    Parser.add_argument('--gallery')
    Arguments = Parser.parse_args()

    Volumes = ReadExactVolumes(Arguments.report)
    Rows = CrossCheckStl(Arguments.files, Volumes)
    Passed = True
    Table = ['| | Specimen | Facets | Vertices | Watertight | Winding | Euler | Components | V trimesh | V exact | ΔV / V |', '|---|---|---|---|---|---|---|---|---|---|---|']
    for Name, Values in Rows:
        if not Values:
            Table.append('| 🔴 | %s | missing | | | | | | | | |' % Name)
            Passed = False
            continue
        Relative = abs(Values['Volume'] - Values['Exact']) / Values['Exact']
        Sound = Values['Watertight'] and Values['Winding'] and Values['Volume'] > 0.0 and Relative < 0.03
        Passed = Passed and Sound
        Table.append('| %s | %s | %d | %d | %s | %s | %d | %d | %.9g | %.9g | %.2e |' % ('🟢' if Sound else '🔴', Name, Values['Faces'], Values['Vertices'], Values['Watertight'], Values['Winding'],
                                                                                   Values['Euler'], Values['Components'], Values['Volume'], Values['Exact'], Relative))
    with open(Arguments.markdown, 'w', encoding='utf-8') as Stream:
        Stream.write('# Exchange cross-check\n\n%s %d STL files written by ExchangeValidation, read back by trimesh %s (vertices merged by trimesh itself, positive outward volume, '
                     'closure and winding by its own edge test). Exact volumes are the kernel\'s MeasureBodyMass values from ExchangeValidation.md; '
                     'the facet volume sits inside them by at most sag · area (EX2).\n\n' % ('🟢' if Passed else '🔴', len(Rows), trimesh.__version__))
        Stream.write('## STL\n\n' + '\n'.join(Table) + '\n')
    if Arguments.gallery:
        RenderGallery(Rows, ['Torus', 'PerforatedBlock', 'HollowBlock', 'SweptRing', 'PipeBend', 'MirroredCross', 'BoxAllFillet', 'HoleRimFillet'], Arguments.gallery)
    print('\n'.join(Table))
    return 0 if Passed else 1

if __name__ == '__main__':
    sys.exit(Main())
