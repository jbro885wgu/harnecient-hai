/* hai_active.h - TEXT-INCLUDED (not linked) by hai_send.c, hai_manager.c and hai_session.c: where the active agent's data and the active
 * session's ledger are. House rule: pure helpers shared by 2+ files are text-included, never a header plus a link step.
 * Agents (2026-11-23): "hai" is the original agent: data = <pkg>/../chat/data, state root = <pkg>/state (so older ledgers and tests keep working).
 * Any other agent <id> is a self-contained folder: data = <pkg>/../chat/agents/<id>/data (its corpus, replies, rules, packs, stopwords),
 * state root = <pkg>/state/agents/<id>. Sessions live under a state root: "main" = <root>/chat.ledger, any other = <root>/sessions/<name>.ledger,
 * <root>/sessions/active.txt names the active session. <pkg>/state/active_agent.txt names the active agent. Both pointers are PREFERENCES:
 * they never change what an agent knows, only which ledger the window shows and writes. Names are letters, digits, _ and - only. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static __attribute__((unused)) int hai_name_ok(const char *n) {
    if (!n || !n[0] || strlen(n) > 60) return 0;
    for (const char *p = n; *p; p++) if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-') return 0;
    return 1;
}
static __attribute__((unused)) void hai_read_pointer(const char *path, const char *dflt, char *name, size_t cap) {
    snprintf(name, cap, "%s", dflt);
    FILE *f = fopen(path, "r"); if (!f) return;
    char line[128] = ""; if (fgets(line, sizeof line, f)) { line[strcspn(line, "\r\n")] = 0; if (hai_name_ok(line)) snprintf(name, cap, "%s", line); }
    fclose(f);
}
static __attribute__((unused)) void hai_agent_active(const char *pkg, char *name, size_t cap) {
    char p[1100]; snprintf(p, sizeof p, "%s/state/active_agent.txt", pkg); hai_read_pointer(p, "hai", name, cap);
}
static __attribute__((unused)) void hai_root(const char *pkg, const char *agent, char *out, size_t cap) {
    if (strcmp(agent, "hai") == 0) snprintf(out, cap, "%s/state", pkg); else snprintf(out, cap, "%s/state/agents/%s", pkg, agent);
}
static __attribute__((unused)) void hai_data_of(const char *pkg, const char *agent, char *out, size_t cap) {
    if (strcmp(agent, "hai") == 0) snprintf(out, cap, "%s/../chat/data", pkg); else snprintf(out, cap, "%s/../chat/agents/%s/data", pkg, agent);
}
static __attribute__((unused)) void hai_session_active(const char *pkg, const char *agent, char *name, size_t cap) {
    char root[1100], p[1200]; hai_root(pkg, agent, root, sizeof root);
    snprintf(p, sizeof p, "%s/sessions/active.txt", root); hai_read_pointer(p, "main", name, cap);
}
static __attribute__((unused)) void hai_session_ledger(const char *pkg, const char *agent, const char *session, char *out, size_t cap) {
    char root[1100]; hai_root(pkg, agent, root, sizeof root);
    if (strcmp(session, "main") == 0) snprintf(out, cap, "%s/chat.ledger", root); else snprintf(out, cap, "%s/sessions/%s.ledger", root, session);
}
static __attribute__((unused)) void hai_active_ledger(const char *pkg, char *out, size_t cap) {
    char a[64], s[64]; hai_agent_active(pkg, a, sizeof a); hai_session_active(pkg, a, s, sizeof s); hai_session_ledger(pkg, a, s, out, cap);
}
static __attribute__((unused)) void hai_active_data(const char *pkg, char *out, size_t cap) {
    char a[64]; hai_agent_active(pkg, a, sizeof a); hai_data_of(pkg, a, out, cap);
}
