/* hai_world.c - the world of hai-pal (2026-11-23, DESIGN book 25, first slice). One authority: an append-only ledger of rows. The map, the agents and where they stand are
 * whatever replaying the rows gives; nothing else is kept. The frame the window shows (RGBA, the canvas format of the house) is a projection and is rewritten from the ledger.
 *
 *   SPAWN|name|page|x|y        an agent appears on a page (the cell must be free)
 *   MOVE|name|dx|dy            one step (dx, dy in -1..1, not both 0); a wall or another agent refuses it
 *   TELEPORT|name|page|x|y     the only way to change page
 *   REORDER|name|slot          move an agent to a 1-based slot of the party; slot 1 is the MASTER (the role is the order); `hai_world master <ledger>` prints it, `hai_world party <ledger>` prints the whole order on one line
 *   REFUSED|verb|name|reason   written when a verb was not allowed; it changes nothing (kept so a refusal can be shown and audited)
 *
 * Rules (each can fail in case hai_world): names are [a-z0-9_-], 1 to 16 characters and unique; cells 1..14 by 1..8 are floor, the border is wall, page "home" has a wall block at
 * (7,4) and (8,4). NOT BUILT: permissions (hard rules, biases: book 25 section 4); every verb is allowed here if the rules above pass.
 * No disk folders yet: the sandbox for real folders is step 1 of the build order and is NOT built; this op never touches a path except the ledger and the frame it is told.
 *
 * Usage:
 *   hai_world do <ledger> SPAWN|MOVE|TELEPORT <args...>   append one row if the rules allow it (exit 0), else append REFUSED and exit 3
 *   hai_world dump <ledger>                                text: one line per agent  name page x y
 *   hai_world frame <ledger> <page> <out_base> [active]    write <out_base>.raw (512x320 RGBA) and <out_base>.receipt.txt (frame_w, frame_h); active gets a white outline
 *   hai_world pages <ledger>                               the page names that exist (home plus any teleported to), one per line
 *   hai_world desk <ledger> <page> <desk_dir> <atlas.rgba> the editor desk of a page (cells, figure tokens), a projection for rpg-edit (2026-11-24)
 * Build: gcc -std=gnu11 -Wall -Wextra -O2 -o hai_world hai_world.c */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define COLS 16
#define ROWS 10
#define CELL 32
#define MAXA 32
typedef struct { char name[20]; char page[20]; int x, y, on; } agent_t;
typedef struct { agent_t a[MAXA]; int n; } world_t;

static int valid_name(const char *s) {
    size_t n = strlen(s); if (n < 1 || n > 16) return 0;
    for (size_t i = 0; i < n; i++) { char c = s[i]; if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0; }
    return 1;
}
static int is_wall(const char *page, int x, int y) {
    if (x < 1 || y < 1 || x > COLS - 2 || y > ROWS - 2) return 1;
    if (strcmp(page, "home") == 0 && y == 4 && (x == 7 || x == 8)) return 1;
    return 0;
}
static agent_t *find(world_t *w, const char *name) { for (int i = 0; i < w->n; i++) if (strcmp(w->a[i].name, name) == 0) return &w->a[i]; return NULL; }
static agent_t *at(world_t *w, const char *page, int x, int y) { for (int i = 0; i < w->n; i++) if (strcmp(w->a[i].page, page) == 0 && w->a[i].x == x && w->a[i].y == y) return &w->a[i]; return NULL; }

/* the one rule set: returns NULL when the verb is allowed (and applies it), else the reason. Used for replay and for new rows alike, so a hand-edited ledger cannot cheat. */
static const char *apply(world_t *w, const char *verb, char **f, int nf) {
    if (strcmp(verb, "SPAWN") == 0 || strcmp(verb, "TELEPORT") == 0) {
        if (nf < 4) return "missing arguments";
        const char *name = f[0], *page = f[1]; int x = atoi(f[2]), y = atoi(f[3]);
        if (!valid_name(name)) return "bad name";
        if (!valid_name(page)) return "bad page name";
        if (is_wall(page, x, y)) return "that cell is a wall";
        if (at(w, page, x, y)) return "that cell is taken";
        if (strcmp(verb, "SPAWN") == 0) {
            if (find(w, name)) return "name already used";
            if (w->n >= MAXA) return "too many agents";
            agent_t *a = &w->a[w->n++]; memset(a, 0, sizeof *a);
            snprintf(a->name, sizeof a->name, "%s", name); snprintf(a->page, sizeof a->page, "%s", page); a->x = x; a->y = y; a->on = 1; return NULL;
        }
        agent_t *a = find(w, name); if (!a) return "no such agent";
        snprintf(a->page, sizeof a->page, "%s", page); a->x = x; a->y = y; return NULL;
    }
    if (strcmp(verb, "MOVE") == 0) {
        if (nf < 3) return "missing arguments";
        agent_t *a = find(w, f[0]); if (!a) return "no such agent";
        int dx = atoi(f[1]), dy = atoi(f[2]);
        if ((dx == 0 && dy == 0) || dx < -1 || dx > 1 || dy < -1 || dy > 1) return "one step only";
        int nx = a->x + dx, ny = a->y + dy;
        if (is_wall(a->page, nx, ny)) return "a wall is in the way";
        if (at(w, a->page, nx, ny)) return "another agent is in the way";
        a->x = nx; a->y = ny; return NULL;
    }
    if (strcmp(verb, "REORDER") == 0) { /* the party order is the role: slot 1 is the master (book 27) */
        if (nf < 2) return "missing arguments";
        int from = -1; for (int i = 0; i < w->n; i++) if (strcmp(w->a[i].name, f[0]) == 0) from = i;
        if (from < 0) return "no such agent";
        int to = atoi(f[1]); if (to < 1 || to > w->n) return "slot out of range";
        to--; if (to == from) return "already in that slot";
        agent_t t = w->a[from];
        if (to < from) for (int i = from; i > to; i--) w->a[i] = w->a[i - 1];
        else for (int i = from; i < to; i++) w->a[i] = w->a[i + 1];
        w->a[to] = t; return NULL;
    }
    return "unknown verb";
}
static int split(char *line, char **f, int max) { int n = 0; char *p = line; while (n < max) { f[n++] = p; char *q = strchr(p, '|'); if (!q) break; *q = 0; p = q + 1; } return n; }

static int replay(const char *ledger, world_t *w) {
    memset(w, 0, sizeof *w);
    FILE *fp = fopen(ledger, "r"); if (!fp) return 0;     /* no ledger yet is an empty world */
    char line[512];
    while (fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\r\n")] = 0; if (!line[0] || line[0] == '#') continue;
        char *f[8]; int nf = split(line, f, 8); if (nf < 1 || strcmp(f[0], "REFUSED") == 0) continue;
        apply(w, f[0], f + 1, nf - 1);                     /* a row that no longer passes the rules is simply skipped: the rules are the authority */
    }
    fclose(fp); return 1;
}
static void colour_of(const char *name, unsigned char *r, unsigned char *g, unsigned char *b) {
    static const unsigned char pal[8][3] = { {230, 90, 90}, {240, 170, 60}, {235, 220, 80}, {110, 200, 110}, {80, 200, 220}, {100, 140, 240}, {180, 120, 230}, {240, 130, 190} };
    unsigned h = 5381; for (const char *p = name; *p; p++) h = h * 33 + (unsigned char)*p;
    *r = pal[h % 8][0]; *g = pal[h % 8][1]; *b = pal[h % 8][2];
}
static void px(unsigned char *fr, int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= COLS * CELL || y >= ROWS * CELL) return; unsigned char *p = fr + ((size_t)y * COLS * CELL + x) * 4; p[0] = (unsigned char)r; p[1] = (unsigned char)g; p[2] = (unsigned char)b; p[3] = 255; }

static int write_atomic(const char *path, const char *text) { char tmp[1200]; snprintf(tmp, sizeof tmp, "%s.tmp", path); FILE *o = fopen(tmp, "w"); if (!o) return -1; fputs(text, o); if (fclose(o) != 0) return -1; return rename(tmp, path); }

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: hai_world do|dump|frame|pages <ledger> ...\n"); return 2; }
    const char *cmd = argv[1], *ledger = argv[2]; world_t w;
    if (strcmp(cmd, "do") == 0) {
        if (argc < 4) { fprintf(stderr, "usage: hai_world do <ledger> VERB args...\n"); return 2; }
        replay(ledger, &w);
        const char *verb = argv[3]; char *f[8]; int nf = 0; for (int i = 4; i < argc && nf < 8; i++) f[nf++] = argv[i];
        for (int i = 0; i < nf; i++) if (strchr(f[i], '|') || strchr(f[i], '\n')) { fprintf(stderr, "bad character in an argument\n"); return 2; }
        const char *why = apply(&w, verb, f, nf);
        FILE *o = fopen(ledger, "a"); if (!o) { perror("ledger"); return 2; }
        if (why) { fprintf(o, "REFUSED|%s|%s|%s\n", verb, nf ? f[0] : "-", why); fclose(o); printf("REFUSED %s: %s\n", verb, why); return 3; }
        fprintf(o, "%s", verb); for (int i = 0; i < nf; i++) fprintf(o, "|%s", f[i]); fprintf(o, "\n"); fclose(o);
        printf("ok %s\n", verb); return 0;
    }
    replay(ledger, &w);
    if (strcmp(cmd, "master") == 0) { if (w.n < 1) { printf("none\n"); return 0; } printf("%s\n", w.a[0].name); return 0; }
    if (strcmp(cmd, "party") == 0) { for (int i = 0; i < w.n; i++) printf("%s%s", i ? " " : "", w.a[i].name); printf("\n"); return 0; }
    if (strcmp(cmd, "dump") == 0) { for (int i = 0; i < w.n; i++) printf("%s %s %d %d\n", w.a[i].name, w.a[i].page, w.a[i].x, w.a[i].y); return 0; }
    if (strcmp(cmd, "pages") == 0) {
        printf("home\n");
        for (int i = 0; i < w.n; i++) { int seen = strcmp(w.a[i].page, "home") == 0; for (int j = 0; j < i && !seen; j++) if (strcmp(w.a[j].page, w.a[i].page) == 0) seen = 1; if (!seen) printf("%s\n", w.a[i].page); }
        return 0;
    }
    if (strcmp(cmd, "frame") == 0) {
        if (argc < 5) { fprintf(stderr, "usage: hai_world frame <ledger> <page> <out_base> [active]\n"); return 2; }
        const char *page = argv[3], *base = argv[4], *active = argc > 5 ? argv[5] : "";
        size_t bytes = (size_t)COLS * CELL * ROWS * CELL * 4; unsigned char *fr = malloc(bytes); if (!fr) return 2;
        for (int y = 0; y < ROWS * CELL; y++) for (int x = 0; x < COLS * CELL; x++) {
            int cx = x / CELL, cy = y / CELL;
            if (is_wall(page, cx, cy)) px(fr, x, y, 92, 98, 112); else { int d = (cx + cy) & 1; px(fr, x, y, d ? 38 : 44, d ? 42 : 48, d ? 50 : 56); }
        }
        for (int i = 0; i < w.n; i++) {
            if (strcmp(w.a[i].page, page) != 0) continue;
            unsigned char r, g, b; colour_of(w.a[i].name, &r, &g, &b);
            int x0 = w.a[i].x * CELL, y0 = w.a[i].y * CELL;
            for (int y = 6; y < CELL - 6; y++) for (int x = 6; x < CELL - 6; x++) px(fr, x0 + x, y0 + y, r, g, b);
            if (active[0] && strcmp(active, w.a[i].name) == 0)
                for (int k = 0; k < CELL; k++) for (int t = 0; t < 2; t++) { px(fr, x0 + k, y0 + t, 255, 255, 255); px(fr, x0 + k, y0 + CELL - 1 - t, 255, 255, 255); px(fr, x0 + t, y0 + k, 255, 255, 255); px(fr, x0 + CELL - 1 - t, y0 + k, 255, 255, 255); }
        }
        char p[1024], tmp[1100]; snprintf(p, sizeof p, "%s.raw", base); snprintf(tmp, sizeof tmp, "%s.tmp", p);
        FILE *o = fopen(tmp, "wb"); if (!o) { perror("frame"); free(fr); return 2; } fwrite(fr, 1, bytes, o); fclose(o); free(fr);
        if (rename(tmp, p) != 0) { perror("rename"); return 2; }
        snprintf(p, sizeof p, "%s.receipt.txt", base); o = fopen(p, "w"); if (!o) return 2; fprintf(o, "frame_w=%d\nframe_h=%d\n", COLS * CELL, ROWS * CELL); fclose(o);
        return 0;
    }
    if (strcmp(cmd, "desk") == 0) {
        /* desk <ledger> <page> <desk_dir> <atlas.rgba>: the editor desk of one page, a PROJECTION of the ledger (safe to delete, rewritten byte for byte): cells.txt (tiles: floor 39 or 42, wall 36), cells.marker (grows by one
         * byte per rewrite, the house reload rule), map.txt (dots), events.txt (one figure token per agent: x y r g b People1 index 2 1 letter), and skin.txt, extrusion.pdl, cells.rgba written only if missing. */
        if (argc < 6) { fprintf(stderr, "usage: hai_world desk <ledger> <page> <desk_dir> <atlas.rgba>\n"); return 2; }
        const char *page = argv[3], *dd = argv[4], *atlas = argv[5]; struct stat sb;
        if (stat(dd, &sb) != 0 || !S_ISDIR(sb.st_mode)) { fprintf(stderr, "desk: the desk folder does not exist (make it first)\n"); return 2; }
        static char buf[COLS * ROWS * 14 + 512]; size_t n = 0; char q[1100];
        n += (size_t)snprintf(buf + n, sizeof buf - n, "width=%d\nheight=%d\ntile_px=24\natlas_cols=16\natlas_tiles=43\nrock_slot=36\ncells\n", COLS, ROWS);
        for (int y = 0; y < ROWS; y++) { for (int x = 0; x < COLS; x++) { int t = is_wall(page, x, y) ? 36 : (((x + y) & 1) ? 42 : 39); int z = is_wall(page, x, y) ? 2 : 1; n += (size_t)snprintf(buf + n, sizeof buf - n, "%s%d,0,%d", x ? " " : "", t, z); } buf[n++] = '\n'; }
        buf[n] = 0; snprintf(q, sizeof q, "%s/cells.txt", dd); if (write_atomic(q, buf) != 0) { fprintf(stderr, "desk: cannot write cells.txt\n"); return 2; }
        { char mk[1100]; struct stat ms; snprintf(mk, sizeof mk, "%s/cells.marker", dd); FILE *mf = fopen(mk, (!stat(mk, &ms) && ms.st_size >= 4096) ? "w" : "a"); if (mf) { fputc('1', mf); fclose(mf); } }
        n = 0; for (int y = 0; y < ROWS; y++) { for (int x = 0; x < COLS; x++) buf[n++] = '.'; buf[n++] = '\n'; } buf[n] = 0; snprintf(q, sizeof q, "%s/map.txt", dd); if (write_atomic(q, buf) != 0) return 2;
        n = 0; buf[0] = 0; int shown = 0;
        for (int i = 0; i < w.n && shown < 26; i++) { if (strcmp(w.a[i].page, page) != 0) continue; unsigned char r, g, b; colour_of(w.a[i].name, &r, &g, &b);
            unsigned hsh = 5381; for (const char *p = w.a[i].name; *p; p++) hsh = hsh * 33 + (unsigned char)*p;
            n += (size_t)snprintf(buf + n, sizeof buf - n, "%d %d %u %u %u People1 %u 2 1 %c\n", w.a[i].x, w.a[i].y, r, g, b, hsh % 8, (char)('A' + shown)); shown++; }
        snprintf(q, sizeof q, "%s/events.txt", dd); if (write_atomic(q, buf) != 0) return 2;
        snprintf(q, sizeof q, "%s/skin.txt", dd); if (stat(q, &sb) != 0) write_atomic(q, "rmmv\n");
        snprintf(q, sizeof q, "%s/extrusion.pdl", dd); if (stat(q, &sb) != 0) write_atomic(q, "SECTION      | KEY                | VALUE\n----------------------------------------------------------------------\nMETA         | map_id             | hai-pal\nMETA         | desk_id            | play\nMETA         | note               | a page of hai-pal drawn from its ledger by hai_world desk\n");
        snprintf(q, sizeof q, "%s/cells.rgba", dd);
        if (stat(q, &sb) != 0) { FILE *a = fopen(atlas, "rb"); if (!a) { fprintf(stderr, "desk: cannot read the atlas\n"); return 2; } FILE *o = fopen(q, "wb"); if (!o) { fclose(a); return 2; } char bb[8192]; size_t k; while ((k = fread(bb, 1, sizeof bb, a)) > 0) fwrite(bb, 1, k, o); fclose(a); fclose(o); }
        printf("DESK %s page=%s %dx%d figures=%d\n", dd, page, COLS, ROWS, shown); return 0;
    }
    fprintf(stderr, "unknown command %s\n", cmd); return 2;
}
