// Smoke test for the mini-gmp-plus-plus WebAssembly module.
//
// Usage (Node >= 18, from anywhere):
//   node Wasm/test/smoke-test.mjs
//
// Exercises the embind bindings against JS BigInt as the reference
// implementation. Exits non-zero on the first mismatch.

import path from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const moduleJs = path.join(repoRoot, "builds/wasm-release/bin/mini-gmp-plus-plus.js");

const { default: createMiniGmpPlusPlus } = await import(`file://${moduleJs}`);
const MiniGmp = await createMiniGmpPlusPlus();

let failures = 0;
function check(name, actual, expected) {
  const a = String(actual);
  const e = String(expected);
  if (a === e) {
    console.log(`ok   ${name}`);
  } else {
    failures++;
    console.error(`FAIL ${name}\n  wasm: ${a}\n  want: ${e}`);
  }
}

// --- MiniMPZ: 100! computed in WASM must match BigInt -----------------------
function bigFactorial(n) {
  let f = 1n;
  for (let i = 2n; i <= n; i++) f *= i;
  return f;
}

let f = new MiniGmp.MiniMPZ("1", 10);
for (let i = 2n; i <= 100n; i++) {
  f = f.mul(new MiniGmp.MiniMPZ(i.toString(), 10));
}
check("100!", f.toString(10), bigFactorial(100n).toString());

// --- MiniMPZ: pow, div, mod, comparisons ------------------------------------
const two = new MiniGmp.MiniMPZ("2", 10);
check("2^256", two.pow(256).toString(10), (2n ** 256n).toString());

const big = new MiniGmp.MiniMPZ("123456789012345678901234567890", 10);
const d = new MiniGmp.MiniMPZ("987654321", 10);
check("div", big.div(d).toString(10), (123456789012345678901234567890n / 987654321n).toString());
check("mod", big.mod(d).toString(10), (123456789012345678901234567890n % 987654321n).toString());
check("gt", big.gt(d), true);
check("lt", d.lt(big), true);
check("ge self", big.ge(new MiniGmp.MiniMPZ("123456789012345678901234567890", 10)), true);
check("equals", d.equals(new MiniGmp.MiniMPZ("987654321", 10)), true);
check("hex round-trip", new MiniGmp.MiniMPZ("deadbeef", 16).toString(16), "deadbeef");
check("neg/abs", new MiniGmp.MiniMPZ("-42", 10).abs().toString(10), "42");
check("sign", new MiniGmp.MiniMPZ("-42", 10).sign(), -1);
check("isOdd", new MiniGmp.MiniMPZ("7", 10).isOdd(), true);
check("sqrt", new MiniGmp.MiniMPZ("1000000000000", 10).sqrt().toString(10), "1000000");
check("toDouble", new MiniGmp.MiniMPZ("255", 10).toDouble(), 255);

// --- MiniMPF: mantissa * 2^exponent -----------------------------------------
const half = new MiniGmp.MiniMPF(0.5);
const quarter = new MiniGmp.MiniMPF(0.25);
check("mpf add", half.add(quarter).estimate(), 0.75);
check("mpf mul", half.mul(quarter).estimate(), 0.125);
check("mpf cmp", half.cmp(quarter), 1);
check("mpf isZero", new MiniGmp.MiniMPF(0).isZero(), true);
check("mpf visu", half.visu().includes("* 2^"), true);
check("mpf from mantissa", new MiniGmp.MiniMPF(new MiniGmp.MiniMPZ("3", 10), 4).estimate(), 48);

if (failures > 0) {
  console.error(`\n${failures} check(s) failed`);
  process.exit(1);
}
console.log("\nall WASM smoke checks passed");
