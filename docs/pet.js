/* pet.js - the JavaScript port of ops/hai_pet.c (book 26, 2026-11-23). The same rules, the same ledger rows, the same text. Case hai_pet_parity runs the C op and this file on the same commands and
 * requires the same words and byte-identical ledgers; a mutant in either turns it red. One ledger is the only authority: needs, remainders, weights and the inventory are what replaying the rows gives.
 * Browser: <script src="pet.js"></script> gives window.HaiPet; node: require('./pet.js'), or the command line, identical to the C op:
 *   node pet.js <data_dir> <ledger> born <name> <type> | give <name> <item> <n> | tick <name> <minutes> | use <name> <item> | weight <name> <need> <tenths> | status <name>
 * Exit 0 ok, 3 refused (reason on stdout, a REFUSED row appended), 2 usage. Body data: body_<type>.pdl rows BODY|type  RATE|need|points|per_minutes  USE|item|verb|need|delta (the house needs.pdl shapes). */
(function (root) {
  'use strict';
  var MAXN = 8, MAXU = 16, MAXI = 16, MAXP = 16;
  function okName(s, dots) { if (s.length < 1 || s.length > 24) return false; for (var i = 0; i < s.length; i++) { var c = s[i]; if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c === '_' || c === '-' || (dots && c === '.'))) return false; } return true; }
  function clampv(v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
  function atol(s) { var n = parseInt(s, 10); return isNaN(n) ? 0 : n; }
  /* like the C split: at most max fields, the rest stays joined in the last one */
  function split(line, max) { var out = [], p = 0; while (out.length < max) { var q = line.indexOf('|', p); if (q < 0 || out.length === max - 1) { out.push(line.slice(p)); return out; } out.push(line.slice(p, q)); p = q + 1; } return out; }

  function parseBody(text, type) {
    var b = { type: type, r: [], u: [] }, lines = text.split('\n');
    for (var i = 0; i < lines.length; i++) {
      var line = lines[i].replace(/[\r\n]+$/, ''); if (!line || line[0] === '#') continue;
      var f = split(line, 6);
      if (f[0] === 'RATE' && f.length >= 4 && b.r.length < MAXN && okName(f[1], false)) { var per = atol(f[3]); b.r.push({ need: f[1], pts: atol(f[2]), per: per <= 0 ? 60 : per }); }
      else if (f[0] === 'USE' && f.length >= 5 && b.u.length < MAXU && okName(f[1], true)) b.u.push({ item: f[1], verb: f[2], need: f[3], delta: atol(f[4]) });
    }
    return b.r.length > 0 ? b : null;
  }
  /* loadBody(type) returns the text of body_<type>.pdl or null; the world never touches a file itself */
  function newWorld(loadBody) { return { pets: [], loadBody: loadBody }; }
  function find(w, name) { for (var i = 0; i < w.pets.length; i++) if (w.pets[i].name === name) return w.pets[i]; return null; }
  function needIx(p, need) { for (var i = 0; i < p.b.r.length; i++) if (p.b.r[i].need === need) return i; return -1; }
  function invIx(p, item, create) { for (var i = 0; i < p.inv.length; i++) if (p.inv[i].item === item) return i; if (!create || p.inv.length >= MAXI) return -1; p.inv.push({ item: item, cnt: 0 }); return p.inv.length - 1; }

  /* the one rule set: null when allowed (and applied), else the reason; replay and new rows both come through here */
  function apply(w, verb, f) {
    var nf = f.length, p, i;
    if (verb === 'BORN') {
      if (nf < 2) return 'missing arguments';
      if (!okName(f[0], false) || !okName(f[1], false)) return 'bad name';
      if (find(w, f[0])) return 'name already used';
      if (w.pets.length >= MAXP) return 'too many pets';
      var txt = w.loadBody(f[1]), b = txt === null || txt === undefined ? null : parseBody(txt, f[1]);
      if (!b) return 'no such body type';
      p = { name: f[0], b: b, val: [], rem: [], w: [], inv: [] };
      for (i = 0; i < MAXN; i++) { p.val.push(0); p.rem.push(0); p.w.push(10); }
      w.pets.push(p); return null;
    }
    p = nf ? find(w, f[0]) : null; if (!p) return 'no such pet';
    if (verb === 'GIVE') {
      if (nf < 3) return 'missing arguments';
      if (!okName(f[1], true)) return 'bad item name';
      var n = atol(f[2]); if (n < 1 || n > 99) return 'count must be 1..99';
      i = invIx(p, f[1], true); if (i < 0) return 'inventory full';
      p.inv[i].cnt += n; return null;
    }
    if (verb === 'TICK') {
      if (nf < 2) return 'missing arguments';
      var m = atol(f[1]); if (m < 1 || m > 100000) return 'minutes must be 1..100000';
      for (i = 0; i < p.b.r.length; i++) { var denom = p.b.r[i].per * 10, tot = p.rem[i] + m * p.b.r[i].pts * p.w[i]; p.val[i] = clampv(p.val[i] + Math.trunc(tot / denom)); p.rem[i] = tot % denom; }
      return null;
    }
    if (verb === 'USE') {
      if (nf < 2) return 'missing arguments';
      var u = null; for (i = 0; i < p.b.u.length; i++) if (p.b.u[i].item === f[1]) u = p.b.u[i];
      if (!u) return 'this body cannot use that item';
      var ni = needIx(p, u.need); if (ni < 0) return 'unknown need';
      var ii = invIx(p, f[1], false); if (ii < 0 || p.inv[ii].cnt < 1) return 'holds none of that item';
      p.inv[ii].cnt--; p.val[ni] = clampv(p.val[ni] + u.delta); return null;
    }
    if (verb === 'WEIGHT') {
      if (nf < 3) return 'missing arguments';
      var wi = needIx(p, f[1]); if (wi < 0) return 'unknown need';
      var t = atol(f[2]); if (t < 0 || t > 40) return 'weight must be 0..40 tenths';
      p.w[wi] = t; return null;
    }
    return 'unknown verb';
  }
  function replay(w, text) {
    var lines = text.split('\n');
    for (var i = 0; i < lines.length; i++) {
      var line = lines[i].replace(/[\r\n]+$/, ''); if (!line || line[0] === '#') continue;
      var f = split(line, 8); if (f[0] === 'REFUSED') continue;
      apply(w, f[0], f.slice(1));
    }
  }
  function statusText(p) {
    var s = 'NEEDS ' + p.name, i; for (i = 0; i < p.b.r.length; i++) s += ' ' + p.b.r[i].need + '=' + p.val[i];
    s += '\nBODY ' + p.b.type + ' weights'; for (i = 0; i < p.b.r.length; i++) s += ' ' + p.b.r[i].need + '=' + p.w[i];
    s += '\nINVENTORY'; for (i = 0; i < p.inv.length; i++) if (p.inv[i].cnt > 0) s += ' ' + p.inv[i].item + ':' + p.inv[i].cnt;
    return s + '\n';
  }
  var VERBS = { born: 'BORN', give: 'GIVE', tick: 'TICK', use: 'USE', weight: 'WEIGHT' };
  var api = { apply: apply, replay: replay, newWorld: newWorld, find: find, statusText: statusText, parseBody: parseBody, VERBS: VERBS };
  if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.HaiPet = api;

  /* ---- command line, the same as the C op ---- */
  if (typeof require !== 'undefined' && typeof module !== 'undefined' && require.main === module) {
    var fs = require('fs'), argv = process.argv.slice(2), out = function (s) { process.stdout.write(s); };
    if (argv.length < 4) { process.stderr.write('usage: pet.js <data_dir> <ledger> born|give|tick|use|weight|status <name> ...\n'); process.exit(2); }
    var data = argv[0], ledger = argv[1], cmd = argv[2];
    var w = newWorld(function (type) { try { return fs.readFileSync(data + '/body_' + type + '.pdl', 'utf8'); } catch (e) { return null; } });
    try { replay(w, fs.readFileSync(ledger, 'utf8')); } catch (e) { /* no ledger yet is an empty world */ }
    if (cmd === 'run') {
      var sc; try { sc = fs.readFileSync(argv[3], 'utf8'); } catch (e) { process.stderr.write('cannot read the script\n'); process.exit(2); }
      var lo = '', nok = 0, nref = 0, sl = sc.split('\n');
      for (var si = 0; si < sl.length; si++) {
        var ln = sl[si].replace(/[\r\n]+$/, ''); if (!ln || ln[0] === '#') continue;
        var sf = split(ln, 8), sv = sf[0], sa = sf.slice(1), sw = apply(w, sv, sa);
        if (sw) { lo += 'REFUSED|' + sv + '|' + (sa.length ? sa[0] : '-') + '|' + sw + '\n'; nref++; } else { lo += ln + '\n'; nok++; }
      }
      fs.appendFileSync(ledger, lo); out('ran ok=' + nok + ' refused=' + nref + '\n');
      if (argv.length > 4) fs.writeFileSync(argv[4], w.pets.map(statusText).join(''));
      process.exit(0);
    }
    if (cmd === 'status') {
      var sp = find(w, argv[3]); if (!sp) { out('REFUSED: no such pet\n'); process.exit(3); }
      out(statusText(sp)); process.exit(0);
    }
    var verb = VERBS[cmd]; if (!verb) { process.stderr.write('unknown command ' + cmd + '\n'); process.exit(2); }
    var f = argv.slice(3, 9);
    for (var k = 0; k < f.length; k++) if (f[k].indexOf('|') >= 0 || f[k].indexOf('\n') >= 0) { process.stderr.write('bad character\n'); process.exit(2); }
    var why = apply(w, verb, f);
    if (why) { fs.appendFileSync(ledger, 'REFUSED|' + verb + '|' + (f.length ? f[0] : '-') + '|' + why + '\n'); out('REFUSED ' + verb + ': ' + why + '\n'); process.exit(3); }
    fs.appendFileSync(ledger, verb + (f.length ? '|' + f.join('|') : '') + '\n'); out('ok ' + verb + '\n'); process.exit(0);
  }
})(typeof window !== 'undefined' ? window : this);
