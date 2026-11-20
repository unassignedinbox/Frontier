#!/usr/bin/env python3
"""Label the actual C++ render outputs; never synthesize replacement pictures."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def construct_sheet(folder: Path, columns: list[tuple[str, str]]) -> None:
    with tempfile.TemporaryDirectory(prefix="frontier-gi-sheet-") as temporary:
        rows = []
        for view in ("front", "rear"):
            tiles = []
            for index, (suffix, label) in enumerate(columns):
                source = folder / f"{view}_{suffix}.png"
                if not source.is_file():
                    raise FileNotFoundError(source)
                tile = Path(temporary) / f"{view}_{index}.png"
                subprocess.run([
                    "convert", str(source), "-resize", "640x360", "-background", "#111827",
                    "-gravity", "south", "-splice", "0x40", "-font", "DejaVu-Sans",
                    "-pointsize", "16", "-fill", "white", "-annotate", "+0+11",
                    f"{view.upper()} / {label}", str(tile),
                ], check=True)
                tiles.append(str(tile))
            row = Path(temporary) / f"{view}.png"
            subprocess.run(["convert", *tiles, "+append", str(row)], check=True)
            rows.append(str(row))
        subprocess.run(["convert", *rows, "-append", str(folder / "00_comparison.png")], check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("gi", "shadows"))
    arguments = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    if arguments.mode == "gi":
        construct_sheet(root / "Exhibits/ControlVehicleGiMethods", [
            ("direct_only", "DIRECT SOLAR ONLY"),
            ("stokes_contour_gi", "STOKES CONTOUR GI"),
            ("surfel_cascade_gi", "SURFEL CASCADE GI"),
            ("path_trace_64spp", "PATH TRACE / 64 SAMPLES"),
        ])
    else:
        construct_sheet(root / "Exhibits/ControlVehicleShadowMethods", [
            ("bvh4_shadow_surfel_gi", "BVH4 SOLAR + SURFEL GI"),
            ("surfel_shadow_surfel_gi", "SURFEL SOLAR + SURFEL GI"),
            ("bvh4_visibility", "BVH4 VISIBILITY"),
            ("surfel_visibility", "SURFEL VISIBILITY"),
        ])


if __name__ == "__main__":
    main()
