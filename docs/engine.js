/* hai engine, browser port of ops/hai_chat_core.c. Same rules, same ledger rows (2026-11-23: the teach loop, WHY and the levers were ported):
 *   TURN|unix|input|concept|reply|state_after|action|typed|confidence     SAID|unix|turn|text     WHY|unix|turn|reasoning
 *   GRADE|unix|turn_no|concept|reply|grade|reward|punish|day              MARK|unix|word|level
 *   DEFINE|unix|word|meaning|origin   ALIAS|unix|word|concept|origin   SAY|unix|word|text|origin   FORGET|unix|word   PACK|unix|name
 * Weight = (reward+1)/(reward+punish+2), 0.5 with no grades. Weights break ties in matching and order the fallback options.
 * A word hai does not know is ASKED about (state await_meaning) and the next line is stored as its meaning.
 * Replies are never rewritten by learning. Everything is replayed from the ledger rows; nothing lives in memory.
 * Data: the same .pdl files as chat/data/ (copies in docs/data/; case hai_web_parity runs the C engine and this one on the same inputs
 * and requires identical output). Works in a browser (window.HaiEngine) and in node (module.exports). Command line (node docs/engine.js):
 *   turn <datadir> <ledger> <input...> | grade <datadir> <ledger> <0-4> | state <ledger> | stats <datadir> <ledger> | calib <datadir> <ledger>
 *   bank <datadir> <ledger> | why <ledger> [turn|last] | cmd <datadir> <ledger> <define|alias|say|forget|pack> <arg1> [arg2] | mark <datadir> <ledger> <word> [high|normal]
 * (the same verbs, output and exit codes as the C tool hai_chat; HAI_NOW=<unix> fixes the clock for a test). */
(function (root) {
  "use strict";
  var TMAX = 64;

  function splitKeep(line) {
    return line.replace(/[\r\n]+$/, "").split("|").map(function (f) { return f.replace(/^ +/, "").replace(/ +$/, ""); });
  }

  // printf("%.<d>f") for a double: JS toFixed rounds an exact tie up, C rounds it to even. Only dyadic ties can be exact (k/8, k/32 ...).
  function fmt(x, d) {
    var s = Math.pow(10, d) * x;
    if (Math.abs(x * 1048576 - Math.round(x * 1048576)) < 1e-9 && Math.abs(s - Math.floor(s) - 0.5) < 1e-9) {
      var fl = Math.floor(s), even = (fl % 2 === 0) ? fl : fl + 1;
      return (even / Math.pow(10, d)).toFixed(d);
    }
    return x.toFixed(d);
  }

  function parseData(files) {
    var concepts = [], replies = [], next = [], rules = {}, stops = {};
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
      var m = line.match(/^RULE \| ([^|]+?) \| (-?\d+)/);
      if (m) rules[m[1].trim()] = parseInt(m[2], 10);
    });
    (files.stopwords || "").split("\n").forEach(function (line) {
      if (line.indexOf("STOP") !== 0) return;
      var f = splitKeep(line);
      if (f.length >= 2) stops[f[1]] = true;
    });
    if (!rules.input_max || !rules.options_max) throw new Error("setup missing rules");
    seeds.forEach(function (sd) { if (!concepts.some(function (c) { return c.id === sd.id; })) throw new Error("seed refers to an unknown concept: " + sd.id); });
    return { concepts: concepts, replies: replies, next: next, rules: rules, seeds: seeds, stops: stops, packs: files.packs || {} };
  }

  function tokens(s) {
    var out = [], cur = "";
    for (var i = 0; i <= s.length; i++) {
      var c = i < s.length ? s[i] : "";
      if (c && /[A-Za-z0-9]/.test(c)) cur += c.toLowerCase();
      else { if (cur.length && out.length < 64) out.push(cur.slice(0, 63)); cur = ""; }
    }
    return out;
  }

  function idx(arr, key, val) { var k = -1; for (var i = 0; i < arr.length; i++) if (arr[i][key] === val) k = i; return k; }
  function swapRemove(arr, i) { arr[i] = arr[arr.length - 1]; arr.pop(); }

  // One pass over the ledger, like replay() in C (the order of taught, alias, say and marked entries matters: a forget moves the last one into the hole).
  function replay(ledger, today, seeds) {
    var st = { state: "start", name: "", turns: 0, gradesToday: 0, lastConcept: "", lastReply: "", lastGradedTurn: -1,
               concepts: {}, replies: {}, pending: "", taught: [], marked: [], alias: [], say: [] };
    function add(map, id, rw, pu) { if (!map[id]) map[id] = { r: 0, p: 0 }; map[id].r += rw; map[id].p += pu; }
    function define(word, meaning) {
      var k = idx(st.taught, "word", word);
      if (k < 0 && st.taught.length < TMAX) { st.taught.push({ word: word, meaning: meaning }); return; }
      if (k >= 0) st.taught[k] = { word: word, meaning: meaning };
    }
    (seeds || []).forEach(function (sd) { st.concepts[sd.id] = { r: sd.r, p: sd.p }; });
    ledger.forEach(function (line) {
      var f, k;
      if (line.indexOf("TURN|") === 0) {
        f = splitKeep(line);
        if (f.length < 8) return;
        st.turns++; st.state = f[5]; st.lastConcept = f[3]; st.lastReply = f[4];
        if (f[6] === "store_name") st.name = f[7];
        if (f[6] === "ask_word") st.pending = f[7]; else st.pending = "";
        if (f[6] === "store_meaning" && f[7]) define(f[7], f[2]);
      } else if (line.indexOf("DEFINE|") === 0) {
        f = splitKeep(line); if (f.length >= 4) define(f[2], f[3]);
      } else if (line.indexOf("ALIAS|") === 0) {
        f = splitKeep(line);
        if (f.length >= 4) { k = idx(st.alias, "word", f[2]); if (k < 0 && st.alias.length < TMAX) st.alias.push({ word: f[2], concept: f[3] }); else if (k >= 0) st.alias[k] = { word: f[2], concept: f[3] }; }
      } else if (line.indexOf("SAY|") === 0) {
        f = splitKeep(line);
        if (f.length >= 4) {
          k = idx(st.say, "word", f[2]);
          if (k < 0 && st.say.length < TMAX) st.say.push({ word: f[2], text: f[3] }); else if (k >= 0) st.say[k] = { word: f[2], text: f[3] };
          if (idx(st.taught, "word", f[2]) < 0) define(f[2], "");
        }
      } else if (line.indexOf("FORGET|") === 0) {
        f = splitKeep(line);
        if (f.length >= 3) {
          k = idx(st.taught, "word", f[2]); if (k >= 0) swapRemove(st.taught, k);
          k = idx(st.alias, "word", f[2]); if (k >= 0) swapRemove(st.alias, k);
          k = idx(st.say, "word", f[2]); if (k >= 0) swapRemove(st.say, k);
          k = st.marked.indexOf(f[2]); if (k >= 0) swapRemove(st.marked, k);
        }
      } else if (line.indexOf("MARK|") === 0) {
        f = splitKeep(line);
        if (f.length < 4) return;
        k = st.marked.indexOf(f[2]);
        if (f[3] === "high") { if (k < 0 && st.marked.length < TMAX) st.marked.push(f[2]); }
        else if (k >= 0) st.marked.splice(k, 1);
      } else if (line.indexOf("GRADE|") === 0) {
        f = splitKeep(line);
        if (f.length < 9) return;
        if (parseInt(f[8], 10) === today) st.gradesToday++;
        st.lastGradedTurn = parseInt(f[2], 10);
        var rw = parseFloat(f[6]), pu = parseFloat(f[7]);
        add(st.concepts, f[3], rw, pu);
        add(st.replies, f[4], rw, pu);
      }
    });
    return st;
  }

  function weightOf(map, id) { var c = map[id]; return c ? (c.r + 1) / (c.r + c.p + 2) : 0.5; }
  function rule(data, key, dflt) { return data.rules[key] !== undefined ? data.rules[key] : dflt; }
  function clean1(s) { return String(s == null ? "" : s).replace(/[|\r\n]/g, " "); }
  function slashes(s) { return s.replace(/[|\n]/g, "/"); }

  function turn(data, ledger, input, now) {
    var clean = clean1(input);
    if (clean.length > data.rules.input_max) clean = clean.slice(0, data.rules.input_max);
    var st = replay(ledger, 0, data.seeds);
    var toks = tokens(clean), nt = toks.length, best = -1, bestHits = 0, bestW = -1, i, t, c, k;
    function aliased(cid, tok) { return st.alias.some(function (a) { return a.concept === cid && a.word === tok; }); }
    for (c = 0; c < data.concepts.length; c++) {
      var cc = data.concepts[c], hits = 0;
      for (t = 0; t < nt; t++) if (cc.words.indexOf(toks[t]) >= 0) hits++;
      for (t = 0; t < nt; t++) if (aliased(cc.id, toks[t])) hits++;
      var w = weightOf(st.concepts, cc.id);
      if (hits > bestHits || (hits === bestHits && hits > 0 && w > bestW)) { bestHits = hits; best = c; bestW = w; }
    }
    var concept = best >= 0 ? data.concepts[best].id : "none";
    var conf = (nt > 0 && bestHits > 0) ? Math.floor((100 * bestHits / nt) * bestW + 0.5) : 0;
    var why = "";
    if (best >= 0) {
      var m = "";
      for (t = 0; t < nt; t++) {
        var hit = data.concepts[best].words.indexOf(toks[t]) >= 0 || aliased(data.concepts[best].id, toks[t]);
        if (hit && m.length + toks[t].length + 6 < 240) m += (m ? ", " : "") + '"' + toks[t] + '"';
      }
      why = "matched " + m + " as " + data.concepts[best].id + ": " + bestHits + " of " + nt + " words x trust " + fmt(bestW, 2) + " = " + conf + "%";
    } else why = "no word matched any meaning I know";

    var to = "", action = "none";
    for (i = 0; i < data.next.length; i++) {
      var r = data.next[i];
      if (r.from !== st.state) continue;
      if (r.concept !== "*" && r.concept !== concept) continue;
      to = r.to; action = r.action; break;
    }
    if (!to) to = st.state;
    var typed = (action === "store_name" && nt > 0) ? toks[0] : "";
    var replyId = best >= 0 ? data.concepts[best].reply : "r_fallback";
    if (st.state === "await_name" && action === "store_name") replyId = "r_named";
    if (st.state === "done") replyId = "r_done";

    var tword = "", tmean = "";
    if (rule(data, "teach_enabled", 0) > 0 && st.state !== "await_name" && st.state !== "done") {
      var minlen = rule(data, "teach_min_len", 3); if (minlen < 1) minlen = 3;
      var mult = rule(data, "attn_high_mult", 1); if (mult < 1) mult = 1;
      if (st.state === "await_meaning" && st.pending) {
        tword = st.pending; concept = "teach"; to = "idle";
        if (nt === 1 && toks[0] === "skip") { replyId = "r_skipped"; action = "none"; typed = ""; conf = 0; }
        else { replyId = "r_learned"; action = "store_meaning"; typed = tword; tmean = clean; conf = 100; }
        if (action === "store_meaning") why = 'I asked what "' + tword + '" means, so I stored your whole answer as its meaning (certain: you told me)';
        else why = 'I asked what "' + tword + '" means and you passed, so nothing was stored';
      } else {
        var ncontent = 0; for (t = 0; t < nt; t++) if (!data.stops[toks[t]]) ncontent++;
        var hitIdx = -1, hits2 = 0, markedHit = false;
        for (t = 0; t < nt; t++) for (k = 0; k < st.taught.length; k++) if (toks[t] === st.taught[k].word) {
          hits2++; if (hitIdx < 0) hitIdx = k;
          if (st.marked.indexOf(toks[t]) >= 0) markedHit = true;
        }
        if (hitIdx >= 0 && hits2 >= bestHits) {
          tword = st.taught[hitIdx].word; tmean = st.taught[hitIdx].meaning;
          concept = "taught:" + tword; replyId = "r_taught"; to = st.state;
          for (k = 0; k < st.say.length; k++) if (st.say[k].word === tword) { replyId = "r_said"; tmean = st.say[k].text; }
          action = "none"; typed = "";
          var tw = weightOf(st.concepts, concept);
          var cv = 100 * hits2 * (markedHit ? mult : 1) / (ncontent > 0 ? ncontent : 1) * tw;
          conf = cv > 100 ? 100 : Math.floor(cv + 0.5);
          why = 'you taught me "' + tword + '"' + (tmean ? "" : " (no meaning, only an answer)") + ": " + hits2 + " hit of " + (ncontent > 0 ? ncontent : 1) + " content words" +
                (markedHit ? " (marked: counts x2 or more)" : "") + " x trust " + fmt(tw, 2) + " = " + conf + "%";
        } else if (best < 0) {
          var pick = -1, pickMarked = false, pickLen = 0;
          for (t = 0; t < nt; t++) {
            if (toks[t].length < minlen || data.stops[toks[t]]) continue;
            var known = data.concepts.some(function (cn) { return cn.words.indexOf(toks[t]) >= 0; }) || st.alias.some(function (a) { return a.word === toks[t]; });
            if (known) continue;
            var mk = st.marked.indexOf(toks[t]) >= 0;
            if (pick < 0 || (mk && !pickMarked) || (mk === pickMarked && toks[t].length > pickLen)) { pick = t; pickMarked = mk; pickLen = toks[t].length; }
          }
          if (pick >= 0) {
            tword = toks[pick]; concept = "ask"; replyId = "r_ask_meaning"; to = "await_meaning"; action = "ask_word"; typed = tword; conf = 0;
            why = '"' + tword + '" is a content word that is not a keyword, an alias or a taught word, so I ask what it means';
          }
        }
      }
    }

    var text = "I don't know that yet.";
    data.replies.some(function (rp) { if (rp.id === replyId) { text = rp.text; return true; } return false; });
    if (tword) { text = text.split("{word}").join(tword); text = text.split("{meaning}").join(tmean); text = text.split("{say}").join(tmean); }
    if (text.indexOf("{name}") >= 0) {
      var who = action === "store_name" ? typed : st.name;
      text = text.replace("{name}", function () { return who; });
    }
    if (replyId === "r_fallback" && why.indexOf("no word matched") === 0) why = "no word matched any meaning I know and there is no new content word to ask about, so I offer my top options";
    if (replyId === "r_fallback") {
      var order = data.concepts.map(function (cn, kk) { return { k: kk, w: weightOf(st.concepts, cn.id) }; });
      order.sort(function (a, b) { return (b.w - a.w) || (a.k - b.k); });
      var opts = order.slice(0, data.rules.options_max).map(function (o, n) { return "[" + (n + 1) + "] " + data.concepts[o.k].label; });
      text = text + " Did you mean: " + opts.join(" ") + "?";
    }
    var row = "TURN|" + now + "|" + clean + "|" + concept + "|" + replyId + "|" + to + "|" + action + "|" + typed + "|" + conf;
    ledger.push(row);
    ledger.push("SAID|" + now + "|" + (st.turns + 1) + "|" + slashes(text));
    ledger.push("WHY|" + now + "|" + (st.turns + 1) + "|" + slashes(why));
    var code = (to === "done" && st.state !== "done") ? 1 : 0;
    return { text: text + " (confidence " + conf + "%)", code: code, row: row };
  }

  // Same checks, same order, same exit codes as hai_chat_grade() in C.
  function grade(data, ledger, g, now) {
    if (!(g >= 0 && g <= 4)) return { code: 4, text: "grade must be 0 to 4" };
    var cap = data.rules.grade_daily_cap, rw = data.rules["grade_" + g + "_reward"], pu = data.rules["grade_" + g + "_punish"];
    if (!cap || cap <= 0) return { code: 2, text: "setup missing rule grade_daily_cap" };
    if (rw === undefined) return { code: 2, text: "setup missing rule grade_" + g + "_reward" };
    if (pu === undefined) return { code: 2, text: "setup missing rule grade_" + g + "_punish" };
    var day = Math.floor(now / 86400);
    var st = replay(ledger, day, data.seeds);
    if (st.turns === 0 || st.lastConcept === "") return { code: 5, text: "no reply to grade yet" };
    if (st.lastGradedTurn === st.turns) return { code: 7, text: "this reply is already graded" };
    if (st.gradesToday >= cap) return { code: 6, text: "daily grade limit reached (" + cap + ")" };
    ledger.push("GRADE|" + now + "|" + st.turns + "|" + st.lastConcept + "|" + st.lastReply + "|" + g + "|" + rw + "|" + pu + "|" + day);
    var after = replay(ledger, day, data.seeds);
    return { code: 0, text: "OK grade " + g + " for " + st.lastReply + " weight=" + fmt(weightOf(after.replies, st.lastReply), 4) };
  }

  function stats(data, ledger) {
    var st = replay(ledger, 0, data.seeds);
    return data.concepts.map(function (c) {
      var k = st.concepts[c.id] || { r: 0, p: 0 };
      return "concept " + c.id + " reward=" + k.r + " punish=" + k.p + " weight=" + fmt((k.r + 1) / (k.r + k.p + 2), 4);
    }).join("\n") + "\n";
  }

  function cleanWord(s) { return (s && /^[A-Za-z0-9]+$/.test(s)) ? s.toLowerCase().slice(0, 63) : ""; }

  // Mark a word as deserving more attention. 0 ok, 4 bad word or level (same texts as C).
  function mark(data, ledger, word, level, now) {
    if (!word || (level !== "high" && level !== "normal")) return { code: 4, text: "usage: /mark <word> [high|normal]" };
    if (!/^[A-Za-z0-9]+$/.test(word)) return { code: 4, text: "a marked word is letters and digits only" };
    var w = word.toLowerCase().slice(0, 63);
    ledger.push("MARK|" + now + "|" + w + "|" + level);
    return { code: 0, text: "OK marked " + w + " " + level };
  }

  // The teaching levers: define | alias | say | forget | pack. Every lever appends rows.
  function cmd(data, ledger, verb, arg1, arg2, now) {
    var turns = replay(ledger, 0, data.seeds).turns, w, t, msg, name;
    function said(m) { ledger.push("SAID|" + now + "|" + turns + "|" + slashes(m)); }
    if (verb === "pack") {
      name = cleanWord(arg1);
      if (!name) return { code: 4, text: "usage: /pack <name>" };
      var body = data.packs[name];
      if (body === undefined) return { code: 4, text: "no pack called " + name };
      ledger.push("PACK|" + now + "|" + name);
      var rows = 0;
      body.split("\n").forEach(function (line) {
        if (line.indexOf("DEFINE") !== 0 && line.indexOf("ALIAS") !== 0 && line.indexOf("SAY") !== 0) return;
        var f = splitKeep(line);
        if (f.length < 3) return;
        var pw = cleanWord(f[1]); if (!pw) return;
        ledger.push(f[0] + "|" + now + "|" + pw + "|" + clean1(f[2]).slice(0, 219) + "|pack:" + name); rows++;
      });
      var pm = "Loaded the " + name + " pack: " + rows + " things taught.";
      said(pm);
      return { code: 0, text: pm };
    }
    w = cleanWord(arg1);
    if (!w) return { code: 4, text: "a word is letters and digits only" };
    if (verb === "define" || verb === "say") {
      t = clean1(arg2).slice(0, 219);
      if (!t) return { code: 4, text: "usage: /" + (verb === "define" ? "teach" : "say") + " <word> = <text>" };
      ledger.push((verb === "define" ? "DEFINE" : "SAY") + "|" + now + "|" + w + "|" + t + "|you");
      msg = verb === "define" ? 'Thank you. I will remember what you said about "' + w + '": ' + t : 'OK. When you mention "' + w + '" I will say: ' + t;
    } else if (verb === "alias") {
      var cid = cleanWord(arg2);
      if (!cid) return { code: 4, text: "usage: /alias <word> = <concept>" };
      if (!data.concepts.some(function (c) { return c.id === cid; })) return { code: 4, text: "I know these meanings to alias to: " + data.concepts.map(function (c) { return c.id; }).join(", ") };
      ledger.push("ALIAS|" + now + "|" + w + "|" + cid + "|you");
      msg = 'OK. "' + w + '" now means the same as ' + cid + " to me.";
    } else if (verb === "forget") {
      ledger.push("FORGET|" + now + "|" + w);
      msg = 'OK. I forgot everything you taught me about "' + w + '".';
    } else return { code: 4, text: "unknown lever " + verb };
    said(msg);
    return { code: 0, text: msg };
  }

  // The bank: everything this agent knows, one line per item (CONCEPT / TAUGHT / ALIAS / SAY), replayed from the ledger.
  function bank(data, ledger) {
    var st = replay(ledger, 0, data.seeds), out = "";
    data.concepts.forEach(function (c) { out += "CONCEPT|" + c.id + "|" + c.words.join(",") + "|" + c.reply + "|" + fmt(weightOf(st.concepts, c.id), 2) + "\n"; });
    st.taught.forEach(function (tt) { out += "TAUGHT|" + tt.word + "|" + tt.meaning + "|" + (st.marked.indexOf(tt.word) >= 0 ? 1 : 0) + "|" + fmt(weightOf(st.concepts, "taught:" + tt.word), 2) + "\n"; });
    st.alias.forEach(function (a) { out += "ALIAS|" + a.word + "|" + a.concept + "\n"; });
    st.say.forEach(function (s) { out += "SAY|" + s.word + "|" + s.text + "\n"; });
    return out;
  }

  // hai_chat why: what you said and why hai answered as it did (turn number, or -1 for the last).
  function why(ledger, want) {
    var turnNo = 0, inp = "", w = "", found = false;
    for (var i = 0; i < ledger.length; i++) {
      var line = ledger[i].replace(/[\r\n]+$/, "");
      if (line.indexOf("TURN|") === 0) { turnNo++; var p = line.indexOf("|", 5); if (p >= 0) { var e = line.indexOf("|", p + 1); inp = line.slice(p + 1, e >= 0 ? e : undefined); } }
      else if (line.indexOf("WHY|") === 0 && (want < 0 || turnNo === want)) {
        var p1 = line.indexOf("|", 4), p2 = p1 >= 0 ? line.indexOf("|", p1 + 1) : -1;
        w = p2 >= 0 ? line.slice(p2 + 1) : ""; found = true; if (want >= 0) break;
      }
    }
    if (!found) return { code: 1, text: "no WHY row (older ledger, or no such turn)" };
    return { code: 0, text: "you said: " + inp + "\nwhy: " + w };
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
      out += "BIN lo=" + lo + " hi=" + hi + " n=" + bn[b] + " good=" + bg[b] + " rate=" + fmt(rate, 4) + " mean_conf=" + fmt(meanc, 4) + "\n";
    }
    out += "ECE=" + fmt(ece, 4) + " graded=" + total + "\n";
    return { code: 0, text: out };
  }

  // One typed line, as the window's hai_send does it: slash levers, /grade, /mark, otherwise one chat turn. Returns {text, code}.
  function send(data, ledger, line, now) {
    var t = String(line == null ? "" : line).replace(/^\s+|\s+$/g, "");
    var m;
    if ((m = t.match(/^\/grade (.*)$/))) { if (!/^-?\d+$/.test(m[1])) return { code: 4, text: "grade must be 0 to 4" }; return grade(data, ledger, parseInt(m[1], 10), now); }
    var levers = [["/teach ", "define"], ["/alias ", "alias"], ["/say ", "say"], ["/forget ", "forget"], ["/pack ", "pack"]];
    for (var i = 0; i < levers.length; i++) {
      if (t.indexOf(levers[i][0]) !== 0) continue;
      var rest = t.slice(levers[i][0].length), eq = rest.indexOf(" = "), a1 = rest, a2 = "";
      if (eq >= 0) { a1 = rest.slice(0, eq); a2 = rest.slice(eq + 3); }
      return cmd(data, ledger, levers[i][1], a1, a2, now);
    }
    if ((m = t.match(/^\/mark (\S+)(?: (\S+))?/))) return mark(data, ledger, m[1], m[2] || "high", now);
    var r = turn(data, ledger, t, now);
    return { text: r.text, code: r.code === 1 ? 0 : r.code };
  }

  var api = { parseData: parseData, turn: turn, grade: grade, stats: stats, calibration: calibration, replay: replay, tokens: tokens,
              mark: mark, cmd: cmd, bank: bank, why: why, send: send, fmt: fmt };
  if (typeof module !== "undefined" && module.exports) {
    module.exports = api;
    if (require.main === module) {
      var fs = require("fs"), a = process.argv.slice(2), v = a[0];
      var now = process.env.HAI_NOW ? parseInt(process.env.HAI_NOW, 10) : Math.floor(Date.now() / 1000);
      var rd = function (p) { try { return fs.readFileSync(p, "utf8"); } catch (e) { return ""; } };
      var load = function (dir) {
        var packs = {}; try { fs.readdirSync(dir + "/packs").forEach(function (n) { if (/\.pdl$/.test(n)) packs[n.slice(0, -4)] = rd(dir + "/packs/" + n); }); } catch (e) {}
        return parseData({ concepts: rd(dir + "/concepts.pdl"), replies: rd(dir + "/replies.pdl"), states: rd(dir + "/states.pdl"), rules: rd(dir + "/chat_rules.pdl"), seeds: rd(dir + "/seeds.pdl"), stopwords: rd(dir + "/stopwords.pdl"), packs: packs });
      };
      var led = function (p) { var s = rd(p); return s ? s.split("\n").filter(function (l) { return l.length; }) : []; };
      var save = function (p, before, after) { if (after.length > before) fs.appendFileSync(p, after.slice(before).join("\n") + "\n"); };
      var done = function (r) { process.stdout.write(r.text + (/\n$/.test(r.text) ? "" : "\n")); process.exit(r.code); };
      var run = function (f) { try { f(); } catch (e) { process.stdout.write("setup missing: " + e.message + "\n"); process.exit(2); } };
      run(function () {
        if (v === "turn" && a.length >= 4) { var d = load(a[1]), l = led(a[2]), n = l.length, r = turn(d, l, a.slice(3).join(" "), now); save(a[2], n, l); done(r); }
        else if (v === "grade" && a.length === 4) { var d2 = load(a[1]), l2 = led(a[2]), n2 = l2.length, r2 = grade(d2, l2, /^\d+$/.test(a[3]) ? parseInt(a[3], 10) : -1, now); save(a[2], n2, l2); done(r2); }
        else if (v === "state" && a.length === 2) { var s = replay(led(a[1]), 0, []); done({ code: 0, text: "state=" + s.state + " name=" + s.name }); }
        else if (v === "stats" && a.length === 3) done({ code: 0, text: stats(load(a[1]), led(a[2])) });
        else if (v === "calib" && a.length === 3) done(calibration(load(a[1]), led(a[2])));
        else if (v === "bank" && a.length === 3) done({ code: 0, text: bank(load(a[1]), led(a[2])) });
        else if (v === "why" && a.length >= 2) done(why(led(a[1]), (a[2] && a[2] !== "last") ? parseInt(a[2], 10) : -1));
        else if (v === "cmd" && a.length >= 5) { var d3 = load(a[1]), l3 = led(a[2]), n3 = l3.length, r3 = cmd(d3, l3, a[3], a[4], a[5] || "", now); save(a[2], n3, l3); done(r3); }
        else if (v === "mark" && a.length >= 4) { var d4 = load(a[1]), l4 = led(a[2]), n4 = l4.length, r4 = mark(d4, l4, a[3], a[4] || "high", now); save(a[2], n4, l4); done(r4); }
        else { process.stderr.write("usage: engine.js turn|grade|state|stats|calib|bank|why|cmd|mark ...\n"); process.exit(8); }
      });
    }
  } else root.HaiEngine = api;
})(typeof window !== "undefined" ? window : globalThis);
