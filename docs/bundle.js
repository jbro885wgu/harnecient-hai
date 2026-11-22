/* hai bundle, JS port of haibot/hai_bundle.c (E2). Same rules, same output text.
 * exportPdl(ledgerText), exportCsv(ledgerText), check(text) -> {code, msg, rows}, importText(text) -> ledgerText. */
(function (root) {
  "use strict";
  var MAXF = 16;
  function splitPipe(line) { return line.split("|"); }
  function splitCsv(line) {
    var out = [], i = 0, cell = "";
    while (true) {
      cell = "";
      if (line.charAt(i) === '"') {
        i++;
        while (i < line.length) {
          if (line.charAt(i) === '"' && line.charAt(i + 1) === '"') { cell += '"'; i += 2; }
          else if (line.charAt(i) === '"') { i++; break; }
          else { cell += line.charAt(i); i++; }
        }
        out.push(cell);
        if (line.charAt(i) === ",") { i++; continue; }
        break;
      }
      while (i < line.length && line.charAt(i) !== ",") { cell += line.charAt(i); i++; }
      out.push(cell);
      if (line.charAt(i) === ",") { i++; continue; }
      break;
    }
    return out;
  }
  function csvCell(s) { return /[,"]/.test(s) ? '"' + s.split('"').join('""') + '"' : s; }
  function allowed(kind, n) { return (kind === "TURN" && n === 8) || (kind === "GRADE" && n === 9); }
  function isLedgerRow(line) { return line.indexOf("TURN|") === 0 || line.indexOf("GRADE|") === 0; }

  function exportPdl(ledger) {
    var out = "# bundle v1\n", rows = 0;
    ledger.split("\n").forEach(function (l) { if (isLedgerRow(l)) { out += l + "\n"; rows++; } });
    return { text: out, rows: rows };
  }
  function exportCsv(ledger) {
    var out = "kind,n,f1,f2,f3,f4,f5,f6,f7,f8,f9\n", rows = 0;
    ledger.split("\n").forEach(function (l) {
      if (!isLedgerRow(l)) return;
      var f = splitPipe(l);
      out += f[0] + "," + f.length + "," + f.slice(1).map(csvCell).join(",") + "\n";
      rows++;
    });
    return { text: out, rows: rows };
  }
  function check(text) {
    var lines = text.split("\n"), csv = false, rows = 0;
    for (var i = 0; i < lines.length; i++) {
      var line = lines[i].replace(/\r$/, ""), ln = i + 1;
      if (ln === 1) {
        if (line.indexOf("kind,n,") === 0) { csv = true; continue; }
        if (line === "# bundle v1") continue;
        return { code: 4, msg: "ERR line 1: not a bundle header", rows: 0 };
      }
      if (line === "" || line.charAt(0) === "#") continue;
      var kind, n;
      if (csv) {
        var cells = splitCsv(line);
        if (cells.length < 2) return { code: 4, msg: "ERR line " + ln + ": short csv row", rows: 0 };
        kind = cells[0]; n = parseInt(cells[1], 10);
        if (!(n >= 1 && n <= MAXF) || cells.length !== n + 1) return { code: 4, msg: "ERR line " + ln + ": n=" + n + " does not match " + cells.length + " cells", rows: 0 };
      } else {
        var f = splitPipe(line); kind = f[0]; n = f.length;
      }
      if (!allowed(kind, n)) return { code: 4, msg: "ERR line " + ln + ": row kind " + kind + " with " + n + " fields is not allowed in a bundle", rows: 0 };
      rows++;
    }
    return { code: 0, msg: "OK check rows=" + rows, rows: rows };
  }
  function importText(text) {
    var c = check(text);
    if (c.code) return { code: c.code, msg: c.msg, ledger: "" };
    var csv = text.split("\n")[0].indexOf("kind,n,") === 0, out = "";
    text.split("\n").forEach(function (raw, i) {
      if (i === 0) return;
      var line = raw.replace(/\r$/, "");
      if (line === "" || line.charAt(0) === "#") return;
      if (!csv) { out += line + "\n"; return; }
      var cells = splitCsv(line), n = parseInt(cells[1], 10);
      out += [cells[0]].concat(cells.slice(2, n + 1)).join("|") + "\n";
    });
    return { code: 0, msg: "OK import rows=" + c.rows, ledger: out };
  }
  var api = { exportPdl: exportPdl, exportCsv: exportCsv, check: check, importText: importText };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.HaiBundle = api;
})(typeof window !== "undefined" ? window : globalThis);
