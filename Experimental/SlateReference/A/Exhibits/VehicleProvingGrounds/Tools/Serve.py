"""High-concurrency preview server: HTTP/1.1 keep-alive, 256-slot TCP backlog, single-request shader bundle."""
import json
import os
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class HighConcurrencyHTTPServer(ThreadingHTTPServer):
    request_queue_size = 256
    allow_reuse_address = True
    daemon_threads = True


class Handler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def end_headers(self):
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate")
        super().end_headers()

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path == "/api/shaders.json":
            self._serve_shader_bundle()
            return
        try:
            super().do_GET()
        except (BrokenPipeError, ConnectionResetError):
            pass

    def _serve_shader_bundle(self):
        shader_dir = os.path.join(self.directory, "Shader")
        bundle = {}
        try:
            for name in sorted(os.listdir(shader_dir)):
                if name.endswith(".wgsl"):
                    with open(os.path.join(shader_dir, name), "r", encoding="utf-8") as fh:
                        bundle[name] = fh.read()
        except OSError as exc:
            body = json.dumps({"error": str(exc)}).encode("utf-8")
            self.send_response(500)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        body = json.dumps(bundle).encode("utf-8")
        try:
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def handle_one_request(self):
        try:
            super().handle_one_request()
        except (BrokenPipeError, ConnectionResetError, TimeoutError):
            self.close_connection = True


if __name__ == "__main__":
    HighConcurrencyHTTPServer(("0.0.0.0", 8080), Handler).serve_forever()
