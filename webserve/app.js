console.log("[app] boot");

const terminalEl = document.getElementById("terminal");
if (!terminalEl) {
  throw new Error("Missing #terminal element");
}

const term = new Terminal({
  cursorBlink: true,
  convertEol: true,
  theme: { background: "#111" },
});

const fitAddon = new FitAddon.FitAddon();

term.loadAddon(fitAddon);
term.open(terminalEl);
term.writeln("[app] terminal ready");

const inputQueue = [];
const decoder = new TextDecoder("utf-8");

const WebKeys = Object.freeze({
  Enter: 1000,
  Escape: 1001,
  Backspace: 1002,
  Tab: 1003,
  ArrowUp: 1004,
  ArrowDown: 1005,
  ArrowLeft: 1006,
  ArrowRight: 1007,
  PageUp: 1008,
  PageDown: 1009,
  Home: 1010,
  End: 1011,
});

const WebModifiers = Object.freeze({
  Shift: 1 << 0,
  Ctrl: 1 << 1,
  Alt: 1 << 2,
  Meta: 1 << 3,
});

const WebActions = Object.freeze({
  KeyDown: 1,
});

const stdoutBytes = [];
const stderrBytes = [];

function flushBytes(byteBuffer, isErr = false) {
  if (byteBuffer.length === 0) return;

  const text = decoder.decode(new Uint8Array(byteBuffer));
  byteBuffer.length = 0;

  term.write(isErr ? "\x1b[31m" + text + "\x1b[0m" : text);
}

function updateTtySize() {
  const mod = window.Module;
  if (!mod) return;

  if (mod.tty && typeof mod.tty.setWindowSize === "function") {
    mod.tty.setWindowSize(0, term.rows, term.cols);
    return;
  }

  if (typeof mod.setWindowSize === "function") {
    mod.setWindowSize(0, term.rows, term.cols);
    return;
  }

  console.log("[app] no tty resize hook found; terminal is", term.cols, "x", term.rows);
}

function fitTerminal() {
  if (!terminalEl.isConnected) return;

  const rect = terminalEl.getBoundingClientRect();
  if (rect.width <= 0 || rect.height <= 0) {
    console.log("[app] terminal has no layout yet; skipping fit");
    return;
  }

  try {
    fitAddon.fit();
    updateTtySize();
    console.log("[app] fit ->", term.cols, "x", term.rows);
  } catch (err) {
    console.error("[app] fit failed", err);
  }
}

function scheduleFit() {
  requestAnimationFrame(() => {
    requestAnimationFrame(fitTerminal);
  });
}

window.addEventListener("resize", scheduleFit);

function wasmKeyboardBridge() {
  const mod = window.Module;
  if (!mod) return null;

  if (typeof mod._fairlanes_web_key_event === "function") {
    return mod._fairlanes_web_key_event.bind(mod);
  }

  if (typeof mod.ccall === "function") {
    return (key, modifiers, action) => {
      mod.ccall(
        "fairlanes_web_key_event",
        null,
        ["number", "number", "number"],
        [key, modifiers, action]
      );
    };
  }

  return null;
}

function eventModifiers(event) {
  let modifiers = 0;
  if (event.shiftKey) modifiers |= WebModifiers.Shift;
  if (event.ctrlKey) modifiers |= WebModifiers.Ctrl;
  if (event.altKey) modifiers |= WebModifiers.Alt;
  if (event.metaKey) modifiers |= WebModifiers.Meta;
  return modifiers;
}

function fairlanesKeyFromEvent(event) {
  if (Object.prototype.hasOwnProperty.call(WebKeys, event.key)) {
    return WebKeys[event.key];
  }

  if (event.key === " " || event.code === "Space") {
    return " ".charCodeAt(0);
  }

  if (event.key && event.key.length === 1 && !event.ctrlKey && !event.altKey && !event.metaKey) {
    const code = event.key.charCodeAt(0);
    if (code >= 0x20 && code <= 0x7e) {
      return code;
    }
  }

  return null;
}

function sendFairlanesKeydown(event) {
  if (event.type !== "keydown" || event.isComposing) return false;

  const key = fairlanesKeyFromEvent(event);
  if (key === null) return false;

  const bridge = wasmKeyboardBridge();
  if (!bridge) return false;

  bridge(key, eventModifiers(event), WebActions.KeyDown);
  event.preventDefault();
  event.stopImmediatePropagation();
  return false;
}

terminalEl.addEventListener("keydown", sendFairlanesKeydown, true);
terminalEl.addEventListener("pointerdown", () => term.focus());

term.onData(() => {
  // Browser keydown is the sole input path for Fairlanes on the web build.
});

window.Module = window.Module || {};

window.Module.preRun = window.Module.preRun || [];
window.Module.preRun.push(function () {
  console.log("[app] preRun");

  FS.init(
    function stdin() {
      return inputQueue.length ? inputQueue.shift() : null;
    },
    function stdout(c) {
      if (c === null) return flushBytes(stdoutBytes, false);

      stdoutBytes.push(c);
      if (c === 10) flushBytes(stdoutBytes, false);
    },
    function stderr(c) {
      if (c === null) return flushBytes(stderrBytes, true);

      stderrBytes.push(c);
      if (c === 10) flushBytes(stderrBytes, true);
    }
  );
});

window.Module.onRuntimeInitialized = function () {
  term.writeln("\r\n[Fairlanes ready]\r\n");
  scheduleFit();
};


console.log("[app] crossOriginIsolated =", window.crossOriginIsolated);

scheduleFit();
