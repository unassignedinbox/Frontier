"""Preview server for the exhibit: serves the shell and never lets a cached JS layout meet newer WGSL bindings.

    python3 Tools/Serve.py            http://0.0.0.0:8080/            (PORT overrides the port)
"""
import os
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

Root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *Args, **Keywords):
        super().__init__(*Args, directory=Root, **Keywords)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def log_message(self, Format, *Args):
        pass


Port = int(os.environ.get('PORT', '8080'))
print(f'serving {Root} on http://0.0.0.0:{Port}/', flush=True)
ThreadingHTTPServer(('0.0.0.0', Port), Handler).serve_forever()
