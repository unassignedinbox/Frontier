#====================================================================================================================================
#                                                 PREPAREDRAFTINGDEPENDENCIES.PY
#====================================================================================================================================
# 🧩 Restores the pinned third-party sources of DraftingWorkspace into the ignored ExternalPackages/ folder (sha256-checked)

"""
Run:  python Experimental/DraftingWorkspace/Tooling/PrepareDraftingDependencies.py [--verify]

The pins for imgui, tomlpp, glfw, vulkan-headers and stb are the ones in SultanAladin/Frontier
Automation/UIDependencies.json, so the workspace lifts into that project without a second copy of any dependency.
volk is the Vulkan meta-loader (no link-time vulkan-1 / libvulkan dependency; the loader or a driver is found at run time).
Configure never downloads: CMake only checks that every witness file exists.
"""

import argparse
import hashlib
import io
import shutil
import sys
import tarfile
import urllib.request
from pathlib import Path

#------------------------------------------------------------------------------------------------------------------------
#                                                    PINS
#------------------------------------------------------------------------------------------------------------------------

RepositoryRoot = Path(__file__).resolve().parents[3]
PackageRoot    = RepositoryRoot / "ExternalPackages"

Packages = [
    { "Name": "imgui",          "Revision": "3bae66c735670619baf51391eba7f3d90a25d125",
      "Url": "https://codeload.github.com/ocornut/imgui/tar.gz/3bae66c735670619baf51391eba7f3d90a25d125",
      "Sha256": "0b99c7baed551f76058bc6fe83d806dc55c912280e9d2d268879a1c9751cef20", "Witness": "imgui.cpp" },
    { "Name": "tomlpp",         "Revision": "1e8829b793b66ad17011732a146b8077d379b011",
      "Url": "https://codeload.github.com/marzer/tomlplusplus/tar.gz/1e8829b793b66ad17011732a146b8077d379b011",
      "Sha256": "8f6131576dbfd77c72d3073ad3969f2bda086961e15937ddb6dc9b4b4238ae01", "Witness": "include/toml++/toml.hpp" },
    { "Name": "glfw",           "Revision": "a74efa0d5628b74adc0426af4c5710e287fa7c2c",
      "Url": "https://codeload.github.com/glfw/glfw/tar.gz/a74efa0d5628b74adc0426af4c5710e287fa7c2c",
      "Sha256": "7e85e39ade1efdda00734d323a0230b5c938074e9fb2ddbe453bdbb281064d2c", "Witness": "include/GLFW/glfw3.h" },
    { "Name": "vulkan-headers", "Revision": "v1.4.321",
      "Url": "https://codeload.github.com/KhronosGroup/Vulkan-Headers/tar.gz/refs/tags/v1.4.321",
      "Sha256": "7c1874f9186d9e7d5245e279cdccaa77a66ed8f52871b428630aeb771dc7623c", "Witness": "include/vulkan/vulkan.h" },
    { "Name": "stb",            "Revision": "2c980bb59875b0d32144a71867fbdebb2f77cd20",
      "Url": "https://codeload.github.com/nothings/stb/tar.gz/2c980bb59875b0d32144a71867fbdebb2f77cd20",
      "Sha256": "9a955b1b49a4410088a2e0ee2a9c057c3c907d0c1d75454144cb980aca0ba515", "Witness": "stb_image.h" },
    { "Name": "volk",           "Revision": "a8da8ef3368482b0ee9b0ec0c6079a16a89c6924",
      "Url": "https://codeload.github.com/zeux/volk/tar.gz/a8da8ef3368482b0ee9b0ec0c6079a16a89c6924",
      "Sha256": "9ded768eb21498868b6c2e0ebd8006b658f40460202f8b80d827c848b4aa8ece", "Witness": "volk.h" },
]

#------------------------------------------------------------------------------------------------------------------------
#                                                    FUNCTIONS
#------------------------------------------------------------------------------------------------------------------------

def RestorePackage(Package: dict) -> None:
    """① download ② check sha256 ③ unpack below a staging folder ④ strip the archive's top folder ⑤ swap into place."""
    Destination = PackageRoot / Package["Name"]
    Stamp       = Destination / ".frontier-revision"
    if (Destination / Package["Witness"]).exists() and Stamp.exists() and Stamp.read_text().strip() == Package["Revision"]:
        print(f"{Package['Name']:<16} present  {Package['Revision']}")
        return
    with urllib.request.urlopen(Package["Url"], timeout=120) as Response:
        Archive = Response.read()
    Digest = hashlib.sha256(Archive).hexdigest()
    if Digest != Package["Sha256"]:
        raise SystemExit(f"{Package['Name']}: sha256 {Digest} differs from the pin {Package['Sha256']}")
    Staging = PackageRoot / (Package["Name"] + ".staging")
    if Staging.exists(): shutil.rmtree(Staging)
    Staging.mkdir(parents=True)
    with tarfile.open(fileobj=io.BytesIO(Archive), mode="r:gz") as Bundle:
        for Member in Bundle.getmembers():
            Parts = Path(Member.name).parts[1:]
            if not Parts or not (Member.isfile() or Member.isdir()): continue
            Output = Staging.joinpath(*Parts)
            if not Output.resolve().is_relative_to(Staging.resolve()): raise SystemExit(f"{Package['Name']}: unsafe path {Member.name}")
            if Member.isdir(): Output.mkdir(parents=True, exist_ok=True); continue
            Output.parent.mkdir(parents=True, exist_ok=True)
            Output.write_bytes(Bundle.extractfile(Member).read())
    (Staging / ".frontier-revision").write_text(Package["Revision"] + "\n")
    if Destination.exists(): shutil.rmtree(Destination)
    Staging.rename(Destination)
    print(f"{Package['Name']:<16} restored {Package['Revision']}  sha256 {Digest}")


def main() -> int:
    Parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    Parser.add_argument("--verify", action="store_true", help="only check that every pinned package is present")
    Arguments = Parser.parse_args()
    if Arguments.verify:
        Missing = [Package["Name"] for Package in Packages if not (PackageRoot / Package["Name"] / Package["Witness"]).exists()]
        if Missing: print("missing: " + ", ".join(Missing) + " — run PrepareDraftingDependencies.py"); return 1
        print("all pinned packages present"); return 0
    PackageRoot.mkdir(exist_ok=True)
    for Package in Packages: RestorePackage(Package)
    return 0


if __name__ == "__main__":
    sys.exit(main())
