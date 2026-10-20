# Frontier

Recovered editor UIs (fracture / fluid-gas / wind / particle) live in [`editors/`](editors/),
with a launcher at [`index.html`](index.html). See [`FOUND.md`](FOUND.md) for where each one was
found (repo × branch × commit), why the fracture editor seemed to be missing, and how to rebuild.

Run everything:

```bash
python3 -m http.server 8080 --bind 0.0.0.0
# open http://localhost:8080/
```

Start with **Project-Zero**: `editors/ProjectZeroEditor/index.html`
(geometry object → Fracture card → Enable fracture → ↗).
