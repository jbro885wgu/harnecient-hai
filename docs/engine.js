/* hai engine, browser port of ops/hai_chat_core.c. Same rules, same ledger rows:
 *   TURN|unix|input|concept|reply|state_after|action|typed_name
 *   GRADE|unix|turn_no|concept|reply|grade|reward|punish|day
 * Weight = (reward+1)/(reward+punish+2), 0.5 with no grades. Weights break ties in matching and
 * order the fallback options. Replies are never rewritten by learning.
 * Data: the same .pdl files as chat/data/ (copies in docs/data/; a parity check keeps them equal).
 * Works in a browser (window.HaiEngine) and in node (module.exports). */
(function (root) {
  "use strict";

  function splitKeep(line) {
    return line.replace(/[\r\n]+$/, "").split("|").map(function (f) { return f.replace(/^ +/, "").replace(/ +$/, ""); });
  }

  function parseData(files) {
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
    var seeds = [];
    (files.seeds || "").split("\n").forEach(function (line) {
      if (line[0] === "#" || line.indexOf("SEED") !== 0) return;
      var f = splitKeep(line);
      if (f.length !== 4) return;
      seeds.push({ id: f[1], r: parseFloat(f[2]), p: parseFloat(f[3]) });
    });
    files.rules.split("\n").forEach(function (line) {
      var m = line.match(/^RULE \| ([^|]+?) \| (\d+)/);
      if (m) rules[m[1].trim()] = parseInt(m[2], 10);
    });
    if (!rules.input_max || !rules.options_max) throw new Error("setup missing rules");
    seeds.forEach(function (sd) { if (!concepts.some(function (c) { return c.id === sd.id; })) throw new Error("seed refers to an unknown concept: " + sd.id); });
    return { concepts: concepts, replies: replies, next: next, rules: rules, seeds: seeds };
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

  // One pass over the ledger, like replay() in C.
  function replay(ledger, today, seeds) {
    var st = { state: "start", name: "", turns: 0, gradesToday: 0, lastConcept: "", lastReply: "",
               lastGradedTurn: -1, concepts: {}, replies: {} };
    function add(map, id, rw, pu) {
      if (!map[id]) map[id] = { r: 0, p: 0 };
      map[id].r += rw; map[id].p += pu;
    }
    (seeds || []).forEach(function (sd) { st.concepts[sd.id] = { r: sd.r, p: sd.p }; });
    ledger.forEach(function (line) {
      if (line.indexOf("TURN|") === 0) {
        var f = splitKeep(line);
        if (f.length < 8) return;
        st.turns++;
        st.state = f[5];
        st.lastConcept = f[3];
        st.lastReply = f[4];
        if (f[6] === "store_name") st.name = f[7];
      } else if (line.indexOf("GRADE|") === 0) {
        var g = splitKeep(line);
        if (g.length < 9) return;
        if (parseInt(g[8], 10) === today) st.gradesToday++;
        st.lastGradedTurn = parseInt(g[2], 10);
        var rw = parseFloat(g[6]), pu = parseFloat(g[7]);
        add(st.concepts, g[3], rw, pu);
        add(st.replies, g[4], rw, pu);
      }
    });
    return st;
  }

  function weightOf(map, id) {
    var c = map[id];
    return c ? (c.r + 1) / (c.r + c.p + 2) : 0.5;
  }

  function turn(data, ledger, input, now) {
    var clean = String(input == null ? "" : input).replace(/[|\r\n]/g, " ");
    if (clean.length > data.rules.input_max) clean = clean.slice(0, data.rules.input_max);
    var st = replay(ledger, 0, data.seeds);
    var toks = tokens(clean), best = -1, bestHits = 0, bestW = -1;
    data.concepts.forEach(function (c, i) {
      var hits = toks.filter(function (t) { return c.words.indexOf(t) >= 0; }).length;
      var w = weightOf(st.concepts, c.id);
      if (hits > bestHits || (hits === bestHits && hits > 0 && w > bestW)) { bestHits = hits; best = i; bestW = w; }
    });
    var concept = best >= 0 ? data.concepts[best].id : "none";
    var conf = (toks.length > 0 && bestHits > 0) ? Math.floor((100 * bestHits / toks.length) * bestW + 0.5) : 0;
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
      var order = data.concepts.map(function (c, k) { return { k: k, w: weightOf(st.concepts, c.id) }; });
      order.sort(function (a, b) { return (b.w - a.w) || (a.k - b.k); });
      var opts = order.slice(0, data.rules.options_max).map(function (o, n) { return "[" + (n + 1) + "] " + data.concepts[o.k].label; });
      text = text + " Did you mean: " + opts.join(" ") + "?";
    }
    text = text + " (confidence " + conf + "%)";
    var row = "TURN|" + now + "|" + clean + "|" + concept + "|" + replyId + "|" + to + "|" + action + "|" + typed + "|" + conf;
    ledger.push(row);
    var code = (to === "done" && st.state !== "done") ? 1 : 0;
    return { text: text, code: code, row: row };
  }

  // Same checks, same order, same exit codes as hai_chat_grade() in C.
  function grade(data, ledger, g, now) {
    if (!(g >= 0 && g <= 4)) return { code: 4, text: "grade must be 0 to 4" };
    var cap = data.rules.grade_daily_cap, rw = data.rules["grade_" + g + "_reward"], pu = data.rules["grade_" + g + "_punish"];
    if (!cap || rw === undefined || pu === undefined) return { code: 2, text: "setup missing rules" };
    var day = Math.floor(now / 86400);
    var st = replay(ledger, day, data.seeds);
    if (st.turns === 0 || st.lastConcept === "") return { code: 5, text: "no reply to grade yet" };
    if (st.lastGradedTurn === st.turns) return { code: 7, text: "this reply is already graded" };
    if (st.gradesToday >= cap) return { code: 6, text: "daily grade limit reached (" + cap + ")" };
    ledger.push("GRADE|" + now + "|" + st.turns + "|" + st.lastConcept + "|" + st.lastReply + "|" + g + "|" + rw + "|" + pu + "|" + day);
    var after = replay(ledger, day, data.seeds);
    var w = weightOf(after.replies, st.lastReply);
    return { code: 0, text: "OK grade " + g + " for " + st.lastReply + " weight=" + w.toFixed(4) };
  }

  function stats(data, ledger) {
    var st = replay(ledger, 0, data.seeds);
    return data.concepts.map(function (c) {
      var k = st.concepts[c.id] || { r: 0, p: 0 };
      return "concept " + c.id + " reward=" + k.r + " punish=" + k.p + " weight=" + ((k.r + 1) / (k.r + k.p + 2)).toFixed(4);
    }).join("\n") + "\n";
  }

  // Calibration: JS port of hai_chat_calibration in C (same bins, same rule rows, same output text).
  function calibration(data, ledger) {
    var width = data.rules.calib_bin_width, goodMin = data.rules.good_min_grade;
    if (!width || width > 100) return { code: 2, text: "setup missing or bad rule calib_bin_width" };
    if (goodMin === undefined) return { code: 2, text: "setup missing rule good_min_grade" };
    var conf = [], turns = 0, bn = [], bg = [], bsum = [], b, i;
    for (b = 0; b < 101; b++) { bn.push(0); bg.push(0); bsum.push(0); }
    ledger.forEach(function (line) {
      if (line.indexOf("TURN|") === 0) {
        var f = splitKeep(line);
        if (f.length < 9) return;
        conf[turns] = parseInt(f[8], 10); turns++;
      } else if (line.indexOf("GRADE|") === 0) {
        var g = splitKeep(line);
        if (g.length < 9) return;
        var t = parseInt(g[2], 10), gr = parseInt(g[5], 10);
        if (t < 1 || t > 4096 || conf[t - 1] === undefined) return;
        var c = conf[t - 1];
        var bin = Math.min(100, Math.floor(c / width));
        bn[bin]++; bsum[bin] += c; if (gr >= goodMin) bg[bin]++;
      }
    });
    var total = 0; for (i = 0; i < 101; i++) total += bn[i];
    var ece = 0, out = "";
    for (b = 0; b < 101; b++) {
      if (bn[b] === 0) continue;
      var lo = b * width, hi = Math.min(100, lo + width - 1);
      var rate = bg[b] / bn[b], meanc = bsum[b] / bn[b] / 100;
      ece += (bn[b] / total) * (rate > meanc ? rate - meanc : meanc - rate);
      out += "BIN lo=" + lo + " hi=" + hi + " n=" + bn[b] + " good=" + bg[b] + " rate=" + rate.toFixed(4) + " mean_conf=" + meanc.toFixed(4) + "\n";
    }
    out += "ECE=" + ece.toFixed(4) + " graded=" + total + "\n";
    return { code: 0, text: out };
  }

  var api = { parseData: parseData, turn: turn, grade: grade, stats: stats, calibration: calibration, replay: replay, tokens: tokens };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.HaiEngine = api;
})(typeof window !== "undefined" ? window : globalThis);
