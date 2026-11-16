#!/usr/bin/env python3
"""Serve the standalone card prototypes and the two viewers from the repository root (default port 8097).

    python3 Documentation/Prototypes/ServePrototypes.py 8097

Routes:
    /                                   index of the card prototypes
    /Documentation/Prototypes/<Name>/   a card prototype (index.html is served for the folder)
    /Experimental/ComponentViewer/      the live Component Viewer and Asset Viewer files

Everything is served from the repository root so the absolute asset paths the cards use
(/Documentation/Prototypes/_shared/...) resolve exactly as they will once the cards are integrated.
"""
import functools
import http.server
import pathlib
import sys

CARDS = [
    ('Placement Frame', 'PlacementFrame',
     'placement, facing, heft, reach, and coupling for one record'),
    ('Lattice Reach', 'LatticeReach',
     'face budget, piece counts, and the size spread of one shape'),
]

INDEX = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<title>Frontier — card prototypes</title>
<style>
    @font-face {{ font-family: 'DM Sans'; src: url('{base}/_shared/fonts/dm-sans-300.woff2') format('woff2');
                  font-weight: 300; font-display: swap; }}
    @font-face {{ font-family: 'DM Sans'; src: url('{base}/_shared/fonts/dm-sans-400.woff2') format('woff2');
                  font-weight: 400; font-display: swap; }}
    body {{ margin: 0; min-height: 100vh; background: #090A0B; color: #E8EAE5;
            font-family: 'DM Sans', system-ui, sans-serif; display: flex; align-items: center;
            justify-content: center; }}
    main {{ width: 470px; padding: 32px 0; }}
    h1 {{ margin: 0 0 4px; font-size: 26px; font-weight: 300; letter-spacing: -0.6px; }}
    p {{ margin: 0 0 26px; font-size: 11.5px; color: #6E747A; }}
    a.card {{ display: block; text-decoration: none; color: inherit; border-radius: 16px; padding: 15px 17px;
              background: #141618; border: 1px solid rgba(255,255,255,0.06); margin-bottom: 10px;
              transition: border-color 140ms, background 140ms; }}
    a.card:hover {{ background: #171A1C; border-color: rgba(216,238,102,0.3); }}
    a.card b {{ display: block; font-size: 13.5px; font-weight: 400; }}
    a.card small {{ display: block; font-size: 10.5px; color: #6E747A; margin-top: 3px; }}
    .foot {{ font-size: 10px; color: #4E545A; margin-top: 22px; line-height: 1.7; }}
    .foot code {{ color: #8A9096; }}
</style>
</head>
<body>
<main>
    <h1>Card prototypes</h1>
    <p>Standalone. Nothing here is wired into the viewers yet.</p>
    {cards}
    <div class="foot">
        Asset Viewer &middot; <code>/Experimental/ComponentViewer/AssetViewer.html</code><br>
        Component Viewer &middot; <code>/Experimental/ComponentViewer/index.html</code>
    </div>
</main>
</body>
</html>
"""


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        '.js': 'text/javascript',
        '.mjs': 'text/javascript',
        '.woff2': 'font/woff2',
        '.facet': 'application/octet-stream',
    }

    def send_index(self):
        base = '/Documentation/Prototypes'
        cards = ''.join(
            f'<a class="card" href="{base}/{folder}/"><b>{title}</b><small>{blurb}</small></a>'
            for title, folder, blurb in CARDS
        )
        body = INDEX.format(base=base, cards=cards).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            self.send_index()
            return
        if self.path.rstrip('/').startswith('/Documentation/Prototypes/') and not self.path.endswith('/'):
            # let a folder path resolve to its index.html
            candidate = pathlib.Path(self.directory) / self.path.lstrip('/')
            if candidate.is_dir():
                self.send_response(302)
                self.send_header('Location', self.path + '/')
                self.end_headers()
                return
        super().do_GET()

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def log_message(self, fmt, *args):
        sys.stderr.write('  %s\n' % (fmt % args))


if __name__ == '__main__':
    root = pathlib.Path(__file__).resolve().parents[2]
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8097
    server = http.server.ThreadingHTTPServer(
        ('0.0.0.0', port), functools.partial(Handler, directory=str(root))
    )
    print(f'Card prototypes on http://0.0.0.0:{port}/', flush=True)
    server.serve_forever()
