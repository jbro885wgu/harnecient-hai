/* world.js - the JavaScript port of ops/hai_world.c (book 25 and 26, 2026-11-23). The same rules, the same ledger rows, the same words, and the same frame bytes (512x320 RGBA, the canvas format).
 * Case hai_world_parity runs the C op and this file on the same commands and requires the same words, exit codes, byte-identical ledgers and byte-identical frames; a mutant in either turns it red.
 * One authority: an append-only ledger. The agents and where they stand are what replaying the rows gives. Rows: SPAWN|name|page|x|y  MOVE|name|dx|dy  TELEPORT|name|page|x|y  REFUSED|verb|name|reason.
 * Browser: window.HaiWorld (apply, replay, frame, dump, pages); node: require('./world.js') or the command line, identical to the C op:
 *   node world.js do <ledger> VERB args... | dump <ledger> | pages <ledger> | frame <ledger> <page> <out_base> [active]
 * Exit 0 ok, 3 refused (a REFUSED row is appended), 2 usage. NOT here: permissions and the real-folder sandbox (not built). */
(function (root) {
  'use strict';
  var COLS = 16, ROWS = 10, CELL = 32, MAXA = 32;
  var PAL = [[230, 90, 90], [240, 170, 60], [235, 220, 80], [110, 200, 110], [80, 200, 220], [100, 140, 240], [180, 120, 230], [240, 130, 190]];
  function atoi(s) { var n = parseInt(s, 10); return isNaN(n) ? 0 : n; }
  function validName(s) { if (s.length < 1 || s.length > 16) return false; for (var i = 0; i < s.length; i++) { var c = s[i]; if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c === '_' || c === '-')) return false; } return true; }
  function isWall(page, x, y) { if (x < 1 || y < 1 || x > COLS - 2 || y > ROWS - 2) return true; if (page === 'home' && y === 4 && (x === 7 || x === 8)) return true; return false; }
  /* like the C split: at most max fields; the field after the last bar kept is cut at its next bar */
  function split(line, max) { var out = [], p = 0; while (out.length < max) { var q = line.indexOf('|', p); if (q < 0) { out.push(line.slice(p)); break; } out.push(line.slice(p, q)); p = q + 1; } return out; }
  function newWorld() { return { a: [] }; }
  function find(w, name) { for (var i = 0; i < w.a.length; i++) if (w.a[i].name === name) return w.a[i]; return null; }
  function at(w, page, x, y) { for (var i = 0; i < w.a.length; i++) if (w.a[i].page === page && w.a[i].x === x && w.a[i].y === y) return w.a[i]; return null; }
  /* the one rule set: null when allowed (and applied), else the reason. Replay and new rows both come through here, so a hand-edited ledger cannot cheat. */
  function apply(w, verb, f) {
    var nf = f.length, a;
    if (verb === 'SPAWN' || verb === 'TELEPORT') {
      if (nf < 4) return 'missing arguments';
      var name = f[0], page = f[1], x = atoi(f[2]), y = atoi(f[3]);
      if (!validName(name)) return 'bad name';
      if (!validName(page)) return 'bad page name';
      if (isWall(page, x, y)) return 'that cell is a wall';
      if (at(w, page, x, y)) return 'that cell is taken';
      if (verb === 'SPAWN') {
        if (find(w, name)) return 'name already used';
        if (w.a.length >= MAXA) return 'too many agents';
        w.a.push({ name: name, page: page, x: x, y: y }); return null;
      }
      a = find(w, name); if (!a) return 'no such agent';
      a.page = page; a.x = x; a.y = y; return null;
    }
    if (verb === 'MOVE') {
      if (nf < 3) return 'missing arguments';
      a = find(w, f[0]); if (!a) return 'no such agent';
      var dx = atoi(f[1]), dy = atoi(f[2]);
      if ((dx === 0 && dy === 0) || dx < -1 || dx > 1 || dy < -1 || dy > 1) return 'one step only';
      var nx = a.x + dx, ny = a.y + dy;
      if (isWall(a.page, nx, ny)) return 'a wall is in the way';
      if (at(w, a.page, nx, ny)) return 'another agent is in the way';
      a.x = nx; a.y = ny; return null;
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
  function dump(w) { var s = ''; for (var i = 0; i < w.a.length; i++) s += w.a[i].name + ' ' + w.a[i].page + ' ' + w.a[i].x + ' ' + w.a[i].y + '\n'; return s; }
  function pages(w) {
    var out = ['home'];
    for (var i = 0; i < w.a.length; i++) { var seen = w.a[i].page === 'home'; for (var j = 0; j < i && !seen; j++) if (w.a[j].page === w.a[i].page) seen = true; if (!seen) out.push(w.a[i].page); }
    return out.join('\n') + '\n';
  }
  function colourOf(name) { var h = 5381; for (var i = 0; i < name.length; i++) h = (Math.imul(h, 33) + name.charCodeAt(i)) >>> 0; return PAL[h % 8]; }
  /* the canvas frame: COLS*CELL by ROWS*CELL RGBA, byte for byte what the C op writes */
  function frame(w, page, active) {
    var W = COLS * CELL, H = ROWS * CELL, fr = new Uint8Array(W * H * 4);
    function px(x, y, r, g, b) { if (x < 0 || y < 0 || x >= W || y >= H) return; var o = (y * W + x) * 4; fr[o] = r; fr[o + 1] = g; fr[o + 2] = b; fr[o + 3] = 255; }
    var x, y, k, t;
    for (y = 0; y < H; y++) for (x = 0; x < W; x++) {
      var cx = Math.floor(x / CELL), cy = Math.floor(y / CELL);
      if (isWall(page, cx, cy)) px(x, y, 92, 98, 112); else { var d = (cx + cy) & 1; px(x, y, d ? 38 : 44, d ? 42 : 48, d ? 50 : 56); }
    }
    for (var i = 0; i < w.a.length; i++) {
      if (w.a[i].page !== page) continue;
      var c = colourOf(w.a[i].name), x0 = w.a[i].x * CELL, y0 = w.a[i].y * CELL;
      for (y = 6; y < CELL - 6; y++) for (x = 6; x < CELL - 6; x++) px(x0 + x, y0 + y, c[0], c[1], c[2]);
      if (active && active === w.a[i].name)
        for (k = 0; k < CELL; k++) for (t = 0; t < 2; t++) { px(x0 + k, y0 + t, 255, 255, 255); px(x0 + k, y0 + CELL - 1 - t, 255, 255, 255); px(x0 + t, y0 + k, 255, 255, 255); px(x0 + CELL - 1 - t, y0 + k, 255, 255, 255); }
    }
    return fr;
  }
  var api = { apply: apply, replay: replay, newWorld: newWorld, find: find, dump: dump, pages: pages, frame: frame, COLS: COLS, ROWS: ROWS, CELL: CELL };
  if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.HaiWorld = api;

  /* ---- command line, the same as the C op ---- */
  if (typeof require !== 'undefined' && typeof module !== 'undefined' && require.main === module) {
    var fs = require('fs'), argv = process.argv.slice(2), out = function (s) { process.stdout.write(s); };
    if (argv.length < 2) { process.stderr.write('usage: world.js do|dump|frame|pages <ledger> ...\n'); process.exit(2); }
    var cmd = argv[0], ledger = argv[1], w = newWorld();
    var read = function () { try { return fs.readFileSync(ledger, 'utf8'); } catch (e) { return ''; } };
    if (cmd === 'do') {
      if (argv.length < 3) { process.stderr.write('usage: world.js do <ledger> VERB args...\n'); process.exit(2); }
      replay(w, read());
      var verb = argv[2], f = argv.slice(3, 11);
      for (var i = 0; i < f.length; i++) if (f[i].indexOf('|') >= 0 || f[i].indexOf('\n') >= 0) { process.stderr.write('bad character in an argument\n'); process.exit(2); }
      var why = apply(w, verb, f);
      if (why) { fs.appendFileSync(ledger, 'REFUSED|' + verb + '|' + (f.length ? f[0] : '-') + '|' + why + '\n'); out('REFUSED ' + verb + ': ' + why + '\n'); process.exit(3); }
      fs.appendFileSync(ledger, verb + (f.length ? '|' + f.join('|') : '') + '\n'); out('ok ' + verb + '\n'); process.exit(0);
    }
    replay(w, read());
    if (cmd === 'dump') { out(dump(w)); process.exit(0); }
    if (cmd === 'pages') { out(pages(w)); process.exit(0); }
    if (cmd === 'frame') {
      if (argv.length < 4) { process.stderr.write('usage: world.js frame <ledger> <page> <out_base> [active]\n'); process.exit(2); }
      var fr = frame(w, argv[2], argv.length > 4 ? argv[4] : '');
      fs.writeFileSync(argv[3] + '.raw', Buffer.from(fr.buffer));
      fs.writeFileSync(argv[3] + '.receipt.txt', 'frame_w=' + COLS * CELL + '\nframe_h=' + ROWS * CELL + '\n'); process.exit(0);
    }
    process.stderr.write('unknown command ' + cmd + '\n'); process.exit(2);
  }
})(typeof window !== 'undefined' ? window : this);
