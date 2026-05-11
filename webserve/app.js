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

term.onData((data) => {
  term.write(data);

  for (let i = 0; i < data.length; i++) {
    inputQueue.push(data.charCodeAt(i));
  }
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
