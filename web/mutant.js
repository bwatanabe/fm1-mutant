// SPDX-License-Identifier: GPL-3.0-only
// FM-1 Mutant installer. Reuses GATOR's known-good Felucca/SLOOP update transport.
import { Updater } from "./fm1ota.js";
import { productOf, logicalImage, sha256hex } from "./fm1pkg.js";

const $ = id => document.getElementById(id);
const state = { hosted: null, local: null, writing: false };
const LABELS = {
  start: "Connecting to FM-1…", verify: "Validating update…",
  loader: "Switching to updater…", write: "Writing firmware: do not disconnect!",
  reboot: "Restarting device…", done: "Installation complete.",
  stopped: "Update interrupted. Reconnect and try the same package again."
};
function status(message, mode = "") {
  $("status").textContent = message;
  $("status").className = mode;
}
function log(line) {
  $("log").textContent += new Date().toLocaleTimeString() + " " + line + "\n";
}
function updateButtons() {
  $("installHosted").disabled = state.writing || !state.hosted;
  $("installLocal").disabled = state.writing || !state.local;
}
function unpack(raw) {
  const product = productOf(raw);
  // Deliberately accept only the existing Felucca family update loader/identity.
  if (!/^FM-1_9\d\d$/.test(product))
    throw new Error("Not a recognized Felucca-platform FM-1 package: " + product);
  const text = new TextDecoder("latin1").decode(raw);
  if (!text.includes("FELUCCA-LOADER-1"))
    throw new Error("Required Felucca update loader is absent.");
  return { product, image: logicalImage(raw) };
}
async function getHosted() {
  if (!state.hosted) throw new Error("No published firmware has been built yet.");
  const p = state.hosted;
  const response = await fetch("firmware/" + p.file, { cache: "no-cache" });
  if (!response.ok) throw new Error("Firmware download: HTTP " + response.status);
  const raw = new Uint8Array(await response.arrayBuffer());
  if (await sha256hex(raw) !== p.sha256)
    throw new Error("SHA-256 mismatch: firmware download failed verification.");
  return unpack(raw);
}
function progress(total) {
  return (kind, amount) => {
    log(kind + " " + (amount ?? ""));
    const scale = {
      start: 2, verify: 8, loader: 11, reboot: 98, done: 100
    };
    let pct = scale[kind] ?? $("progress").value;
    if (kind === "write") pct = 12 + Math.min(86, (amount / total) * 86);
    $("progress").value = pct;
    if (LABELS[kind]) status(LABELS[kind], kind === "done" ? "good" : "");
  };
}
async function install(pkg) {
  if (!navigator.requestMIDIAccess) throw new Error("Chrome or Edge with Web MIDI is required.");
  const midi = await navigator.requestMIDIAccess({ sysex: true });
  const wake = await navigator.wakeLock?.request("screen").catch(() => null);
  state.writing = true;
  updateButtons();
  $("progress").value = 0;
  const updater = new Updater(midi, log);
  const report = progress(Math.ceil(pkg.image.length / 512));
  try {
    const ota = await updater.find(id => /^ota-/i.test(id.model));
    if (ota) {
      ota.link.close();
      const finished = await updater.resume(pkg.image, report);
      status(finished ? "Update complete. FM-1 restarting." : LABELS.stopped, finished ? "good" : "bad");
    } else {
      await updater.install(pkg.image, pkg.product, report);
      status("Installed. FM-1 should restart with the selected firmware.", "good");
    }
    $("progress").value = 100;
  } finally {
    await wake?.release?.().catch(() => null);
    state.writing = false;
    updateButtons();
  }
}
async function onInstall(kind) {
  try {
    status("Reading and validating firmware…");
    const pkg = kind === "hosted" ? await getHosted() : unpack(state.local);
    log("Verified package: " + pkg.product + ", " + pkg.image.length + " bytes");
    await install(pkg);
  } catch (err) {
    status("Install stopped: " + (err?.message || String(err)), "bad");
    log(String(err?.stack || err));
  }
}
async function onLocal(event) {
  state.local = null;
  try {
    const f = event.target.files?.[0];
    if (f) {
      if (f.size > 12_000_000) throw new Error("Unexpectedly large firmware package.");
      const raw = new Uint8Array(await f.arrayBuffer());
      unpack(raw); // Validate immediately, before enabling installation.
      state.local = raw;
      status("Local file validated. Use Install selected file only if you trust its source.");
    }
  } catch (err) {
    status("File not accepted: " + err.message, "bad");
  }
  updateButtons();
}
async function loadBuild() {
  try {
    const response = await fetch("firmware/manifest.json", { cache: "no-store" });
    if (!response.ok) throw new Error("no build available");
    const manifest = await response.json();
    const pkg = manifest.packages?.find(p => p.id === "mutant");
    if (!pkg || !pkg.sha256 || !pkg.file) throw new Error("no Mutant image published");
    state.hosted = pkg;
    $("buildInfo").textContent = pkg.title + " " + pkg.version + " · " + (pkg.size / 1024).toFixed(0) + " KB · SHA-256 verified before update.";
    $("firmwareDownload").href = "firmware/" + pkg.file;
    $("firmwareDownload").removeAttribute("hidden");
    updateButtons();
  } catch (_) {
    $("buildInfo").textContent = "No published Mutant package yet. You can still preview a locally compiled .fwsc in the emulator and install a trusted local file.";
  }
}
$("local").addEventListener("change", onLocal);
$("installLocal").addEventListener("click", () => onInstall("local"));
$("installHosted").addEventListener("click", () => onInstall("hosted"));
window.addEventListener("beforeunload", event => {
  if (state.writing) { event.preventDefault(); event.returnValue = "Firmware update in progress"; }
});
loadBuild();