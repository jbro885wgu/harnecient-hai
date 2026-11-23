/* hai_session - the agent and session controls of the hai-cli window (a plain <item action=...>: the renderer appends <package_dir> <house_root>;
 * hai_send also runs it for the /agent and /session typed commands).
 * argv: <request> <package_dir> [house_root]
 *   NEW | SWITCH <session> | DELETE <session>       sessions of the ACTIVE agent
 *   AGENT <id>                                       make <id> the active agent
 *   NEWAGENT <id> [persona]                          a new agent: a copy of the base data + a personality preset, its own first session
 *   DELETEAGENT <id>                                 remove an agent folder and its sessions ("hai" is never removed)
 * NEW makes a session with an EMPTY transcript that INHERITS what the person taught (DEFINE ALIAS SAY FORGET MARK rows, and the answers they
 * gave to hai's questions as DEFINE rows): a ledger is the agent's whole mind, and a fresh one would forget every word. Grades and turns are not inherited.
 * A persona (chat/personas/<name>.pdl) is applied ONCE at birth: RULE rows replace numbers in the agent's chat_rules.pdl, REPLY rows replace
 * answers in its replies.pdl, PACK rows load a starter pack into its first session. After that the agent's files are plain data to edit.
 * Build: gcc -std=gnu11 -Wall -Wextra -O2 -I../ops -o bin/hai_session hai_session.c ../ops/hai_chat_core.c */
#define _GNU_SOURCE
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "hai_chat_core.h"
#include "hai_active.h"

static int write_pointer(const char *dir, const char *file, const char *name) {
    char path[1300], tmp[1400]; mkdir(dir, 0755);
    snprintf(path, sizeof path, "%s/%s", dir, file); snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w"); if (!f) return 3;
    fprintf(f, "%s\n", name); fclose(f);
    return rename(tmp, path) == 0 ? 0 : 3;
}
static void mkdirs(const char *path) {                       /* mkdir -p, no shell */
    char t[1400]; snprintf(t, sizeof t, "%s", path);
    for (char *p = t + 1; *p; p++) if (*p == '/') { *p = 0; mkdir(t, 0755); *p = '/'; }
    mkdir(t, 0755);
}
static int copy_file(const char *a, const char *b) {
    FILE *in = fopen(a, "rb"), *out = fopen(b, "wb"); if (!in || !out) { if (in) fclose(in); if (out) fclose(out); return 3; }
    char buf[4096]; size_t n; while ((n = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, n, out);
    fclose(in); fclose(out); return 0;
}
static int copy_tree(const char *src, const char *dst, int depth) {   /* files and sub-folders, depth-limited */
    mkdirs(dst); DIR *d = opendir(src); if (!d) return 3; struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        char a[1400], b[1400]; snprintf(a, sizeof a, "%s/%s", src, e->d_name); snprintf(b, sizeof b, "%s/%s", dst, e->d_name);
        struct stat st; if (stat(a, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) { if (depth > 0) copy_tree(a, b, depth - 1); } else copy_file(a, b);
    }
    closedir(d); return 0;
}
/* Replace (or append) the row of file whose prefix up to the second '|' equals key, e.g. "RULE | attn_high_mult |". */
static void set_row(const char *file, const char *key, const char *row) {
    char tmp[1400]; snprintf(tmp, sizeof tmp, "%s.tmp", file);
    FILE *in = fopen(file, "r"), *out = fopen(tmp, "w"); if (!out) { if (in) fclose(in); return; }
    char line[2048]; int done = 0; size_t kl = strlen(key);
    if (in) while (fgets(line, sizeof line, in)) { if (strncmp(line, key, kl) == 0) { if (!done) fprintf(out, "%s\n", row); done = 1; } else fputs(line, out); }
    if (!done) fprintf(out, "%s\n", row);
    if (in) fclose(in); fclose(out); rename(tmp, file);
}
static void trim(char *s) { size_t n = strlen(s); while (n && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ')) s[--n] = 0; }

static int has_turn(const char *ledger) {
    FILE *f = fopen(ledger, "r"); if (!f) return 0; char l[512]; int yes = 0;
    while (fgets(l, sizeof l, f)) if (strncmp(l, "TURN|", 5) == 0) { yes = 1; break; }
    fclose(f); return yes;
}

static int inherit(const char *src, const char *dst) {
    FILE *in = fopen(src, "r"), *out = fopen(dst, "w");
    if (!out) { if (in) fclose(in); return 3; }
    if (in) {
        char line[2048];
        while (fgets(line, sizeof line, in)) {
            if (!strncmp(line, "DEFINE|", 7) || !strncmp(line, "ALIAS|", 6) || !strncmp(line, "SAY|", 4) || !strncmp(line, "FORGET|", 7) || !strncmp(line, "MARK|", 5)) { fputs(line, out); continue; }
            if (strncmp(line, "TURN|", 5) != 0) continue;
            char *f[9]; int n = 0; char *p = line; f[n++] = p;           /* TURN|ts|input|concept|reply|state_after|action|typed|conf */
            while (*p && n < 9) { if (*p == '|') { *p = 0; f[n++] = p + 1; } p++; }
            if (n >= 8 && strcmp(f[6], "store_meaning") == 0 && f[7][0]) fprintf(out, "DEFINE|%s|%s|%s|inherit\n", f[1], f[7], f[2]);
        }
        fclose(in);
    }
    fclose(out); return 0;
}

static int new_agent(const char *pkg, const char *id, const char *persona) {
    char dst[1400], base[1400], pf[1400], line[2048], pers_name[64] = "plain";
    if (!hai_name_ok(id) || strcmp(id, "hai") == 0) { fprintf(stderr, "an agent id is letters, digits, _ and - (and not hai)\n"); return 4; }
    hai_data_of(pkg, id, dst, sizeof dst); hai_data_of(pkg, "hai", base, sizeof base);
    char chk[1400]; snprintf(chk, sizeof chk, "%s/concepts.pdl", dst); if (access(chk, F_OK) == 0) { fprintf(stderr, "agent %s already exists\n", id); return 4; }
    if (persona && persona[0]) {                 /* validate BEFORE copying, so a failed birth leaves nothing behind */
        char pchk[1400]; if (!hai_name_ok(persona)) return 4;
        snprintf(pchk, sizeof pchk, "%s/../chat/personas/%s.pdl", pkg, persona);
        if (access(pchk, F_OK) != 0) { fprintf(stderr, "no persona called %s\n", persona); return 4; }
    }
    if (copy_tree(base, dst, 2) != 0) return 3;
    char packs[16][64]; int np = 0;
    if (persona && persona[0]) {
        if (!hai_name_ok(persona)) return 4;
        snprintf(pf, sizeof pf, "%s/../chat/personas/%s.pdl", pkg, persona);
        FILE *f = fopen(pf, "r"); if (!f) { fprintf(stderr, "no persona called %s\n", persona); return 4; }
        snprintf(pers_name, sizeof pers_name, "%s", persona);
        char rules[1400], reps[1400]; snprintf(rules, sizeof rules, "%s/chat_rules.pdl", dst); snprintf(reps, sizeof reps, "%s/replies.pdl", dst);
        while (fgets(line, sizeof line, f)) {
            trim(line);
            if (!strncmp(line, "RULE |", 6)) { char k[96]; if (sscanf(line, "RULE | %95[^| ]", k) == 1) { char key[128]; snprintf(key, sizeof key, "RULE | %s |", k); set_row(rules, key, line); } }
            else if (!strncmp(line, "REPLY |", 7)) { char k[96]; if (sscanf(line, "REPLY | %95[^| ]", k) == 1) { char key[128]; snprintf(key, sizeof key, "REPLY | %s |", k); set_row(reps, key, line); } }
            else if (!strncmp(line, "PACK |", 6) && np < 16) { char k[64]; if (sscanf(line, "PACK | %63s", k) == 1 && hai_name_ok(k)) snprintf(packs[np++], 64, "%s", k); }
        }
        fclose(f);
    }
    char card[1400]; snprintf(card, sizeof card, "%s/../agent.pdl", dst);        /* <agent>/agent.pdl: who this agent is */
    FILE *c = fopen(card, "w"); if (c) { fprintf(c, "AGENT | %s | %s | %ld\n", id, pers_name, (long)time(NULL)); fclose(c); }
    char root[1400]; hai_root(pkg, id, root, sizeof root); mkdirs(root);
    char led[1500]; hai_session_ledger(pkg, id, "main", led, sizeof led);
    FILE *l = fopen(led, "a"); if (l) fclose(l);                                  /* its first session exists, empty */
    for (int i = 0; i < np; i++) { char out[300]; hai_chat_cmd(dst, led, "pack", packs[i], "", (long)time(NULL), out, sizeof out); }
    char ptr[1400]; snprintf(ptr, sizeof ptr, "%s/state", pkg);
    return write_pointer(ptr, "active_agent.txt", id);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: hai_session <NEW|SWITCH s|DELETE s|AGENT a|NEWAGENT a [persona]|DELETEAGENT a> <package_dir> [house_root]\n"); return 2; }
    const char *req = argv[1], *pkg = argv[2];
    char agent[64], root[1300], sdir[1400], src[1500], dst[1500], name[128];
    hai_agent_active(pkg, agent, sizeof agent); hai_root(pkg, agent, root, sizeof root); snprintf(sdir, sizeof sdir, "%s/sessions", root);
    if (strcmp(req, "NEW") == 0) {
        mkdirs(sdir);
        char cur[64]; hai_session_active(pkg, agent, cur, sizeof cur);
        hai_session_ledger(pkg, agent, cur, src, sizeof src);
        if (strcmp(cur, "main") != 0 && !has_turn(src)) return 0;           /* the active session is still empty: reuse it (a double click, or a second "New") */
        snprintf(name, sizeof name, "session_%ld", (long)time(NULL));
        hai_session_ledger(pkg, agent, name, dst, sizeof dst);
        for (int k = 2; access(dst, F_OK) == 0 && k < 100; k++) {              /* NEVER overwrite a session: inherit() truncates its target, and two clicks in one second share a name */
            snprintf(name, sizeof name, "session_%ld_%d", (long)time(NULL), k); hai_session_ledger(pkg, agent, name, dst, sizeof dst);
        }
        if (access(dst, F_OK) == 0) return 3;
        if (inherit(src, dst) != 0) return 3;
        return write_pointer(sdir, "active.txt", name);
    }
    if (strncmp(req, "SWITCH ", 7) == 0 && hai_name_ok(req + 7)) {
        snprintf(name, sizeof name, "%s", req + 7); hai_session_ledger(pkg, agent, name, dst, sizeof dst);
        if (strcmp(name, "main") != 0 && access(dst, F_OK) != 0) return 4;
        return write_pointer(sdir, "active.txt", name);
    }
    if (strncmp(req, "DELETE ", 7) == 0 && hai_name_ok(req + 7)) {
        snprintf(name, sizeof name, "%s", req + 7);
        if (strcmp(name, "main") == 0) return 4;                          /* main is the agent's first ledger: never deleted from the window */
        char act[64]; hai_session_active(pkg, agent, act, sizeof act);
        hai_session_ledger(pkg, agent, name, dst, sizeof dst); unlink(dst);
        return strcmp(act, name) == 0 ? write_pointer(sdir, "active.txt", "main") : 0;
    }
    char st[1300]; snprintf(st, sizeof st, "%s/state", pkg);
    if (strncmp(req, "AGENT ", 6) == 0 && hai_name_ok(req + 6)) {
        if (strcmp(req + 6, "hai") != 0) { hai_data_of(pkg, req + 6, dst, sizeof dst); char chk[1500]; snprintf(chk, sizeof chk, "%s/concepts.pdl", dst); if (access(chk, F_OK) != 0) return 4; }
        return write_pointer(st, "active_agent.txt", req + 6);
    }
    if (strncmp(req, "NEWAGENT ", 9) == 0) {
        char id[64] = "", persona[64] = ""; sscanf(req + 9, "%63s %63s", id, persona);
        return new_agent(pkg, id, persona);
    }
    if (strncmp(req, "DELETEAGENT ", 12) == 0 && hai_name_ok(req + 12) && strcmp(req + 12, "hai") != 0) {
        char r2[1300], d2[1400], act[64]; hai_root(pkg, req + 12, r2, sizeof r2); hai_data_of(pkg, req + 12, d2, sizeof d2);
        char top[1400]; snprintf(top, sizeof top, "%s/../chat/agents/%s", pkg, req + 12);   /* the agent folder, computed BEFORE its data is removed */
        char cmd1[1500]; (void)cmd1;
        /* remove the files we know about: data files + packs, sessions, pointer files (no recursive delete of anything else) */
        char ddir[1500], pdir[1500]; snprintf(ddir, sizeof ddir, "%s", d2); snprintf(pdir, sizeof pdir, "%s/packs", d2);
        DIR *d = opendir(pdir); struct dirent *e; if (d) { while ((e = readdir(d))) { if (e->d_name[0] == '.') continue; char f[1700]; snprintf(f, sizeof f, "%s/%s", pdir, e->d_name); unlink(f); } closedir(d); rmdir(pdir); }
        d = opendir(ddir); if (d) { while ((e = readdir(d))) { if (e->d_name[0] == '.') continue; char f[1700]; snprintf(f, sizeof f, "%s/%s", ddir, e->d_name); unlink(f); } closedir(d); rmdir(ddir); }
        char card[1500]; snprintf(card, sizeof card, "%s/agent.pdl", top); unlink(card); rmdir(top);
        char sd[1500]; snprintf(sd, sizeof sd, "%s/sessions", r2);
        d = opendir(sd); if (d) { while ((e = readdir(d))) { if (e->d_name[0] == '.') continue; char f[1700]; snprintf(f, sizeof f, "%s/%s", sd, e->d_name); unlink(f); } closedir(d); rmdir(sd); }
        char led[1500]; snprintf(led, sizeof led, "%s/chat.ledger", r2); unlink(led); rmdir(r2);
        hai_agent_active(pkg, act, sizeof act);
        return strcmp(act, req + 12) == 0 ? write_pointer(st, "active_agent.txt", "hai") : 0;
    }
    return 4;
}
