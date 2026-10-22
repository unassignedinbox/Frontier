// Validates the injected surface-shader chunk without glslangValidator: extracts
// fragmentHead + fragmentBody from src/surface-shader.js, stubs the context Three.js
// provides (varyings, camera uniforms, main() variables), and parses the result as
// GLSL. Catches syntax errors and undeclared identifiers in our own chunk.
// Usage: node scripts/check-shader-parse.mjs   (needs devDependencies installed)
import { parser } from '@shaderfrog/glsl-parser';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { jointSetsGLSL } from '../src/joint-sets.js';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const src = readFileSync(join(root, 'src', 'surface-shader.js'), 'utf8');
// the head interpolates the shared joint-set GLSL at runtime; substitute the same source
const head = src.match(/const fragmentHead = \/\* glsl \*\/`([\s\S]*?)`;/)[1].replace('${jointSetsGLSL}', jointSetsGLSL);
const body = src.match(/const fragmentBody = \/\* glsl \*\/`([\s\S]*?)`;/)[1];

// Every uXxx the chunk uses must be declared in it (or be a Three.js-provided builtin).
const declared = new Set(
  [...head.matchAll(/uniform\s+\w+\s+([^;]+);/g)]
    .flatMap((m) => m[1].split(',').map((s) => s.trim().split(/[\s\[]/)[0])),
);
const threeBuiltins = new Set(['vColor']);
const used = new Set([...(head + body).matchAll(/\bu[A-Z]\w*/g)].map((m) => m[0]));
const missing = [...used].filter((x) => !declared.has(x) && !threeBuiltins.has(x));
if (missing.length) {
  console.error(`undeclared uniforms used: ${missing.join(', ')}`);
  process.exit(1);
}

const stubs = /* glsl */`
precision highp float;
uniform mat4 viewMatrix; uniform vec3 cameraPosition;
uniform vec3 diffuse; uniform float opacity; uniform float roughness; uniform float metalness;
vec3 diffuseColor; float roughnessFactor; float metalnessFactor;
vec3 inverseTransformDirection( vec3 d, mat4 m ) { return d; }
float cgAO = 1.0; vec3 cgEmissive = vec3( 0.0 );
`;
const chunk = `${stubs}\n${head}\nvoid __testMain() {\n  vec3 normal = vec3( 0.0, 1.0, 0.0 );\n${body}\n}\n`;

// Same-block redeclaration gate: the GLSL parser checks syntax, not symbol tables,
// so `float bedMix` twice in main() parses fine and then blanks the user's screen
// (R5 foam bug). Walk the braces; a name declared twice in one block is an error.
{
  const code = chunk.replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/\/\/[^\n]*/g, '');
  const n = code.length;
  const paren = new Int32Array(n + 1);
  let pd = 0;
  for (let i = 0; i < n; i++) {
    const c = code[i];
    if (c === '(') pd++;
    else if (c === ')') pd = Math.max(0, pd - 1);
    paren[i + 1] = pd;
  }
  const events = [];
  for (let i = 0; i < n; i++) {
    if (code[i] === '{') events.push([i, 'push']);
    else if (code[i] === '}') events.push([i, 'pop']);
  }
  const declRe = /(^|[;{}])\s*(float|int|uint|bool|vec2|vec3|vec4|mat3|mat4|ivec2|sampler2D)\s+([A-Za-z_]\w*)/g;
  let m;
  while ((m = declRe.exec(code)) !== null) {
    const at = m.index + m[0].lastIndexOf(m[3]);
    if (paren[at] > 0) continue; // for-init / params / call args live in parens, not blocks
    events.push([at, 'decl', m[3]]);
  }
  events.sort((a, b) => a[0] - b[0]);
  const stack = [new Map()];
  const dups = [];
  for (const [, kind, name] of events) {
    if (kind === 'push') stack.push(new Map());
    else if (kind === 'pop') stack.pop();
    else {
      const top = stack[stack.length - 1];
      if (top.has(name)) dups.push(name);
      else top.set(name, 1);
    }
  }
  if (dups.length) {
    console.error(`surface-shader chunk: REDECLARATION in one block: ${[...new Set(dups)].join(', ')}`);
    process.exit(1);
  }
}
try {
  parser.parse(chunk);
  console.log(`surface-shader chunk: PARSE OK (${(chunk.length / 1024).toFixed(0)} KB, ${used.size} uniforms all declared)`);
} catch (e) {
  console.error('surface-shader chunk: PARSE FAILED');
  console.error(String(e.message).split('\n').slice(0, 25).join('\n'));
  process.exit(1);
}
