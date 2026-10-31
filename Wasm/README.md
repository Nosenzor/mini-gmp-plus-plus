# mini-gmp-plus-plus WebAssembly build

Cross-compiles the library to WebAssembly and wraps the C++ classes `MiniMPZ`
(arbitrary-precision integers) and `MiniMPF` (mantissa \* 2^exponent floats) in
a TypeScript-friendly ES6 module via Emscripten's embind. The unmodified C core
(`mini-gmp.c`, 64-bit limbs) runs in the browser/worker.

An interactive demo page lives in [`web/`](../web/). A Node smoke test for the
module API is [`test/smoke-test.mjs`](test/smoke-test.mjs).

## Build

Prerequisites: [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
(activate it so `emcc` is on `PATH` or `EMSDK` is set), CMake >= 3.21, and Ninja.
No other dependency is needed: unlike the desktop build, the WASM build uses
the portable non-SIMD path and links the library statically into the module.

```bash
source <your-emsdk>/emsdk_env.sh
cmake --preset wasm-release
cmake --build --preset wasm-release
```

The preset drives the build through [`cmake/emscripten/emscripten-toolchain.cmake`](../cmake/emscripten/emscripten-toolchain.cmake),
a thin wrapper that locates `Emscripten.cmake` from `EMSCRIPTEN_ROOT`, `EMSDK`,
or `emcc` on `PATH`.

Artifacts land in `builds/wasm-release/bin/`:

| File                        | Purpose                                       |
|-----------------------------|-----------------------------------------------|
| `mini-gmp-plus-plus.js`     | ES6 module loader (default export = factory)  |
| `mini-gmp-plus-plus.wasm`   | compiled library + bindings                   |
| `types/mini-gmp-plus-plus.d.ts` | TypeScript declarations for the module  |

The module is built for `web`, `worker` and `node` environments, so the same
artifacts can be smoke-tested under Node against JS BigInt as reference:

```bash
node Wasm/test/smoke-test.mjs
```

## Usage (JavaScript / TypeScript)

```js
import createMiniGmpPlusPlus from "./builds/wasm-release/bin/mini-gmp-plus-plus.js";

const MiniGmp = await createMiniGmpPlusPlus();

// Big values are constructed from strings; JS numbers cannot hold them.
const z = new MiniGmp.MiniMPZ("123456789012345678901234567890", 10);
console.log(z.mul(z).toString(10));       // 15241578753238836750495351562536198787501905199875019052100
console.log(new MiniGmp.MiniMPZ("2", 10).pow(256).toString(10)); // 2^256

const f = new MiniGmp.MiniMPF(0.5);        // 1 * 2^-1
console.log(f.add(new MiniGmp.MiniMPF(0.25)).estimate()); // 0.75
```

See [`types/mini-gmp-plus-plus.d.ts`](types/mini-gmp-plus-plus.d.ts) for the
full API. Notes:

- the `.js` loader locates the `.wasm` next to itself (`import.meta.url`), so
  both files must be served/copied together
- on wasm32, `toLong()` truncates like a 32-bit `long` and `pow(exp)` takes a
  32-bit exponent; prefer `toString(base)` for big values

## Demo page (GitHub Pages)

[`web/`](../web/) is a small big-integer calculator built on the module. The
[`pages.yml`](../.github/workflows/pages.yml) workflow builds the module,
copies the artifacts next to the page, and deploys it to GitHub Pages on
changes to `web/`, `Wasm/` or the WASM build files (one-time setup: Settings ->
Pages -> Source = "GitHub Actions").

## npm package

Release tags (`v*.*.*`) and manual dispatches of
[`release-wasm.yml`](../.github/workflows/release-wasm.yml) build the module
and run `npm pack` on [`package.json`](package.json), producing
`mini-gmp-plus-plus-wasm-<version>.tgz` (attached to the GitHub release on
tags; published to npm when the `NPM_TOKEN` secret is set):

```bash
npm install mini-gmp-plus-plus-wasm
```

```js
import createMiniGmpPlusPlus from "mini-gmp-plus-plus-wasm";
const MiniGmp = await createMiniGmpPlusPlus();
console.log(new MiniGmp.MiniMPZ("2", 10).pow(256).toString(10));
```

- `MiniMPZ.toString(base)` takes an explicit base (10 or 16).
- `MiniMPZ.toLong()` truncates to a 32-bit `long` on wasm32; use `toString`
  for values beyond int32.
- `MiniMPZ.pow(exp)` takes a wasm32 32-bit exponent.
- `MiniMPF` values are exact `mantissa * 2^exponent` numbers; `estimate()`
  converts to the nearest double.
