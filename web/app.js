// Demo page for the mini-gmp-plus-plus WebAssembly module.
// Serve the repo root over HTTP (the module is fetched from
// ../builds/wasm-release/bin/) e.g.:  python3 -m http.server
// then open /web/ in a browser.

const status = document.getElementById("status");
const app = document.getElementById("app");
const out = document.getElementById("out");

const { default: createMiniGmpPlusPlus } = await import(
  "../builds/wasm-release/bin/mini-gmp-plus-plus.js"
);
const MiniGmp = await createMiniGmpPlusPlus();

status.textContent = "module loaded";
app.hidden = false;

const ops = {
  add: (a, b) => a.add(b),
  sub: (a, b) => a.sub(b),
  mul: (a, b) => a.mul(b),
  div: (a, b) => a.div(b),
  pow: (a) => a.pow(64),
};

for (const button of app.querySelectorAll("button")) {
  button.addEventListener("click", () => {
    const a = new MiniGmp.MiniMPZ(document.getElementById("a").value.trim(), 10);
    const b = new MiniGmp.MiniMPZ(document.getElementById("b").value.trim(), 10);
    try {
      out.textContent = ops[button.dataset.op](a, b).toString(10);
    } catch (err) {
      out.textContent = String(err);
    }
  });
}
