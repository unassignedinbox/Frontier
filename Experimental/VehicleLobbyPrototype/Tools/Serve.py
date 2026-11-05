#!/usr/bin/env python3
# ====================================================================================================================================
#                                                             SERVE.PY
# ====================================================================================================================================
# 🧩 Preview server for the car configurator lobby prototype: the restored VehicleHMI renderer plus the lobby folder.
#
# 📝 Routes: /lobby/... -> Lobby/ (this prototype). /ReferenceApps/... -> the restored ReferenceApps folder. Everything else ->
#    the restored renderer root. "/" redirects to the studio lobby. Responses are not cached, so an edit shows on reload.
#    Binds 0.0.0.0 so the sandbox preview can reach it.
#
# Usage: python3 Experimental/VehicleLobbyPrototype/Tools/Serve.py [--port 8080]
#        (run Automation/RestoreVehicleHmiRenderer.py first)

import argparse
import mimetypes
import os
import sys
from functools import partial
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlsplit, unquote

RepositoryRoot = Path(__file__).resolve().parents[3]
PrototypeRoot = Path(__file__).resolve().parents[1]
RendererRoot = RepositoryRoot / "ExternalPackages" / "VehicleHmiRenderer" / "Experimental" / "VehicleHmiPrototype"
ReferenceAppsRoot = RepositoryRoot / "ExternalPackages" / "VehicleHmiRenderer" / "ReferenceApps"
LobbyRoot = PrototypeRoot / "Lobby"

mimetypes.add_type("text/javascript", ".mjs")
mimetypes.add_type("text/plain", ".wgsl")
mimetypes.add_type("model/gltf-binary", ".glb")


class LobbyRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *Arguments, **Keywords):
        super().__init__(*Arguments, directory=str(RendererRoot), **Keywords)

    def do_GET(self):
        Path_ = urlsplit(self.path).path
        if Path_ in ("/", ""):
            self.send_response(302)
            self.send_header("Location", "/lobby/index.html?scene=studio")
            self.end_headers()
            return
        super().do_GET()

    def translate_path(self, path):
        Clean = unquote(urlsplit(path).path)
        for Prefix, Base in (("/lobby/", LobbyRoot), ("/ReferenceApps/", ReferenceAppsRoot)):
            if Clean.startswith(Prefix):
                Relative = Clean[len(Prefix):]
                Target = (Base / Relative).resolve()
                if Base.resolve() not in Target.parents and Target != Base.resolve():
                    return str(Base / "__refused__")
                if Target.is_dir():
                    Target = Target / "index.html"
                return str(Target)
        return str(RendererRoot / Clean.lstrip("/"))

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, Format, *Arguments):
        sys.stderr.write("%s - %s\n" % (self.address_string(), Format % Arguments))


def Main():
    Parser = argparse.ArgumentParser(description="Car configurator lobby preview server.")
    Parser.add_argument("--port", type=int, default=8080)
    Arguments = Parser.parse_args()
    for Required in (RendererRoot / "index.html", ReferenceAppsRoot):
        if not Required.exists():
            sys.exit(f"Missing {Required}. Run Automation/RestoreVehicleHmiRenderer.py first.")
    Server = ThreadingHTTPServer(("0.0.0.0", Arguments.port), LobbyRequestHandler)
    print(f"Car configurator lobby preview on 0.0.0.0:{Arguments.port}", flush=True)
    Server.serve_forever()


if __name__ == "__main__":
    Main()
