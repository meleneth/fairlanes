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

term.open(terminalEl);
term.resize(120, 40);
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
  term.resize(120, 40);
  updateTtySize();
  console.log("[app] fixed size ->", term.cols, "x", term.rows);
}

window.addEventListener("resize", fitTerminal);

term.onData((data) => {
  term.write(data);

  for (let i = 0; i < data.length; i++) {
    inputQueue.push(data.charCodeAt(i));
  }
});

window.Module = window.Module || {};

Object.assign(window.Module, {
  preRun: [
    function () {
      console.log("[app] preRun");

      FS.init(
        function stdin() {
          return inputQueue.length ? inputQueue.shift() : null;
        },
        function stdout(c) {
          if (c === null) {
            flushBytes(stdoutBytes, false);
            return;
          }

          stdoutBytes.push(c);
          if (c === 10) flushBytes(stdoutBytes, false);
        },
        function stderr(c) {
          if (c === null) {
            flushBytes(stderrBytes, true);
            return;
          }

          stderrBytes.push(c);
          if (c === 10) flushBytes(stderrBytes, true);
        }
      );
    },
  ],

  print(text) {
    term.writeln(text);
  },

  printErr(text) {
    term.writeln("\x1b[31m" + text + "\x1b[0m");
  },

  onRuntimeInitialized() {
    term.writeln("\r\n[Fairlanes ready]\r\n");
    fitTerminal();
  },
});

console.log("[app] crossOriginIsolated =", window.crossOriginIsolated);

fitTerminal();
