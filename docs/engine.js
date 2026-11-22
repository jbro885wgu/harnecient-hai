/* hai engine, browser port of ops/hai_chat_core.c. Same rules, same ledger row format:
 *   TURN|unix|input|concept|reply|state_after|action|typed_name
 * Data is the same .pdl files as chat/data/ (copies in docs/data/; a parity check keeps them equal).
 * Works in a browser (window.HaiEngine) and in node (module.exports). */
(function (root) {
  "use strict";

  function splitKeep(line) {
    // Keeps EMPTY fields, like split_keep() in C. Trims spaces and line ends per field.
    return line.replace(/[\r\n]+$/, "").split("|").map(function (f) { return f.replace(/^ +/, "").replace(/[ ]+$/, ""); });
  }

  function parseData(files) {
    // files: {concepts, replies, states, rules} = text of each .pdl
    var concepts = [], replies = [], next = [], rules = {};
    files.concepts.split("\n").forEach(function (line) {
      if (line[0] === "#" || line.indexOf("CONCEPT") !== 0) return;
      var f = splitKeep(line);
      if (f.length < 5) return;
      concepts.push({ id: f[1], words: f[2].split(",").map(function (w) { return w.replace(/^ +/, ""); }).filter(function (w) { return w.length; }).slice(0, 16), reply: f[3], label: f[4] });
    });
    files.replies.split("\n").forEach(function (line) {
      if (line[0] === "#" || line.indexOf("REPLY") !== 0) return;
      var f = splitKeep(line);
      if (f.length < 3) return;
      replies.push({ id: f[1], text: f.slice(2).join("|") });
    });
    files.states.split("\n").forEach(function (line) {
      if (line[0] === "#" || line.indexOf("NEXT") !== 0) return;
      var f = splitKeep(line);
      if (f.length < 5) return;
      next.push({ from: f[1], concept: f[2], to: f[3], action: f[4] });
    });
    files.rules.split("\n").forEach(function (line) {
      var m = line.match(/^RULE \| ([^|]+?) \| (\d+)/);
      if (m) rules[m[1].trim()] = parseInt(m[2], 10);
    });
    if (!rules.input_max || !rules.options_max) throw new Error("setup missing rules");
    return { concepts: concepts, replies: replies, next: next, rules: rules };
  }

  function tokens(s) {
    var out = [], cur = "";
    for (var i = 0; i <= s.length; i++) {
      var c = i < s.length ? s[i] : "";
      if (c && /[A-Za-z0-9]/.test(c)) cur += c.toLowerCase();
      else { if (cur.length) out.push(cur.slice(0, 63)); cur = ""; }
    }
    return out;
  }

  function replay(ledger) {
    var state = "start", name = "";
    ledger.forEach(function (line) {
      if (line.indexOf("TURN|") !== 0) return;
      var f = splitKeep(line);
      if (f.length < 8) return;
      state = f[5];
      if (f[6] === "store_name") name = f[7];
    });
    return { state: state, name: name };
  }

  function turn(data, ledger, input, now) {
    var clean = String(input == null ? "" : input).replace(/[|\r\n]/g, " ");
    if (clean.length > data.rules.input_max) clean = clean.slice(0, data.rules.input_max);
    var st = replay(ledger);
    var toks = tokens(clean), best = -1, bestHits = 0;
    data.concepts.forEach(function (c, i) {
      // C: count the tokens that match any word of this concept.
      var matched = toks.filter(function (t) { return c.words.indexOf(t) >= 0; }).length;
      if (matched > bestHits) { bestHits = matched; best = i; }
    });
    var concept = best >= 0 ? data.concepts[best].id : "none";
    var to = "", action = "none";
    for (var i = 0; i < data.next.length; i++) {
      var r = data.next[i];
      if (r.from !== st.state) continue;
      if (r.concept !== "*" && r.concept !== concept) continue;
      to = r.to; action = r.action; break;
    }
    if (!to) to = st.state;
    var typed = (action === "store_name" && toks.length) ? toks[0] : "";
    var replyId = best >= 0 ? data.concepts[best].reply : "r_fallback";
    if (st.state === "await_name" && action === "store_name") replyId = "r_named";
    if (st.state === "done") replyId = "r_done";
    var text = "I don't know that yet.";
    data.replies.some(function (r) { if (r.id === replyId) { text = r.text; return true; } return false; });
    if (text.indexOf("{name}") >= 0) {
      var who = action === "store_name" ? typed : st.name;
      text = text.split("{name}").join(who);
    }
    if (replyId === "r_fallback") {
      var opts = data.concepts.slice(0, data.rules.options_max).map(function (c, k) { return "[" + (k + 1) + "] " + c.label; });
      text = text + " Did you mean: " + opts.join(" ") + "?";
    }
    var row = "TURN|" + now + "|" + clean + "|" + concept + "|" + replyId + "|" + to + "|" + action + "|" + typed;
    ledger.push(row);
    var code = 0;
    if (to === "done" && st.state !== "done") code = 1;
    return { text: text, code: code, row: row };
  }

  var api = { parseData: parseData, turn: turn, replay: replay, tokens: tokens };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.HaiEngine = api;
})(typeof window !== "undefined" ? window : globalThis);
