/* hai event interpreter, JS port of haibot/hai_event.c (E1). Same row set, same operands, same output.
 * Row set: PAGE | name · WHEN | event · IF | left | op | right · THEN | APPEND | fields... · THEN | SET | key | value · THEN | STOP
 * Operands: $name = rule, @name = fact (values set earlier in this run win), anything else = literal.
 * Anything outside the set is refused with its line number. Returns {code, lines, error}. */
(function (root) {
  "use strict";
  var OPS = ["eq", "ne", "ge", "gt", "le", "lt"];

  function split(line) {
    return line.split("|").map(function (s) { return s.replace(/^[ \t]+/, "").replace(/[ \t\r\n]+$/, ""); });
  }
  function kvRows(text, isRule) {
    var out = [], ln = 0;
    text.split("\n").forEach(function (line) {
      ln++;
      if (line[0] === "#" || line === "") return;
      var f = split(line);
      if (isRule) {
        if (f.length !== 3 || f[0] !== "RULE") throw { line: ln, msg: "rules rows are RULE | key | value" };
        out.push({ k: f[1], v: f[2] });
      } else {
        if (f.length !== 2) throw { line: ln, msg: "facts rows are key | value" };
        out.push({ k: f[0], v: f[1] });
      }
    });
    return out;
  }
  function lookup(arr, k) {
    for (var i = arr.length - 1; i >= 0; i--) if (arr[i].k === k) return arr[i].v;
    return null;
  }
  function isNum(s) { return /^-?[0-9]+$/.test(s); }
  function compare(op, l, r) {
    var cmp = (isNum(l) && isNum(r)) ? (parseInt(l, 10) - parseInt(r, 10)) : (l < r ? -1 : (l > r ? 1 : 0));
    if (cmp < 0) cmp = -1; else if (cmp > 0) cmp = 1;
    switch (op) {
      case "eq": return cmp === 0; case "ne": return cmp !== 0;
      case "ge": return cmp >= 0;  case "gt": return cmp > 0;
      case "le": return cmp <= 0;  case "lt": return cmp < 0;
    }
    return null;
  }

  function parsePages(text) {
    var pages = [], ln = 0;
    text.split("\n").forEach(function (line) {
      ln++;
      if (line[0] === "#" || line === "") return;
      var f = split(line), n = f.length, cur = pages[pages.length - 1];
      if (f[0] === "PAGE") {
        if (n !== 2) throw { line: ln, msg: "PAGE takes one name" };
        pages.push({ name: f[1], when: "", ifs: [], thens: [] });
      } else if (f[0] === "WHEN") {
        if (!cur || n !== 2) throw { line: ln, msg: "WHEN needs a PAGE above it and one event" };
        cur.when = f[1];
      } else if (f[0] === "IF") {
        if (!cur || n !== 4) throw { line: ln, msg: "IF is: IF | left | op | right" };
        if (OPS.indexOf(f[2]) < 0) throw { line: ln, msg: "unknown IF operator " + f[2] };
        cur.ifs.push({ a: f[1], op: f[2], b: f[3], line: ln });
      } else if (f[0] === "THEN") {
        if (!cur || n < 2) throw { line: ln, msg: "THEN needs a PAGE above it" };
        if (f[1] === "APPEND") cur.thens.push({ verb: "APPEND", f: f.slice(2, 10), line: ln });
        else if (f[1] === "SET" && n === 4) cur.thens.push({ verb: "SET", f: [f[2], f[3]], line: ln });
        else if (f[1] === "STOP" && n === 2) cur.thens.push({ verb: "STOP", f: [], line: ln });
        else throw { line: ln, msg: "THEN verbs are APPEND | fields..., SET | key | value, STOP" };
      } else {
        throw { line: ln, msg: "unknown row kind (refused, not guessed): " + f[0] };
      }
    });
    return pages;
  }

  function runEvent(pageText, rulesText, factsText, event) {
    try {
      var rules = kvRows(rulesText, true), facts = kvRows(factsText, false), local = [], lines = [];
      var pages = parsePages(pageText), ran = 0;
      function resolve(tok) {
        if (tok.charAt(0) === "$") {
          var rv = lookup(rules, tok.slice(1));
          if (rv === null) throw { code: 3, msg: "missing rule " + tok.slice(1) };
          return rv;
        }
        if (tok.charAt(0) === "@") {
          var fv = lookup(local, tok.slice(1));
          if (fv === null) fv = lookup(facts, tok.slice(1));
          if (fv === null) throw { code: 3, msg: "missing fact " + tok.slice(1) };
          return fv;
        }
        return tok;
      }
      pages.forEach(function (p) {
        if (p.when !== event) return;
        ran++;
        var pass = true;
        for (var i = 0; i < p.ifs.length && pass; i++) {
          var c = p.ifs[i], l = resolve(c.a), r = resolve(c.b);
          if (!compare(c.op, l, r)) pass = false;
        }
        if (!pass) return;
        for (var t = 0; t < p.thens.length; t++) {
          var th = p.thens[t];
          if (th.verb === "STOP") break;
          if (th.verb === "APPEND") {
            lines.push("ROW " + th.f.map(resolve).join("|"));
          } else {
            var v = resolve(th.f[1]);
            local.push({ k: th.f[0], v: v });
            lines.push("FACT " + th.f[0] + "=" + v);
          }
        }
      });
      lines.push("DONE n=" + ran);
      return { code: 0, lines: lines, error: null };
    } catch (e) {
      if (e && e.code) return { code: e.code, lines: [], error: e.msg };
      if (e && e.line) return { code: 2, lines: [], error: "line " + e.line + ": " + e.msg };
      throw e;
    }
  }

  var api = { runEvent: runEvent };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.HaiEvent = api;
})(typeof window !== "undefined" ? window : globalThis);
