/* hai_chat_core: the FSM chat engine. Pure file in, file out; used by hai, hai_chat and the window.
 * State and learning are never held in memory: they are replayed from the chat ledger on every call.
 * Ledger rows:
 *   TURN|unix|input|concept|reply|state_after|action|typed_name
 *   GRADE|unix|turn_no|concept|reply|grade|reward|punish|day
 * Weight of a concept or reply = (reward+1)/(reward+punish+2)  (Laplace; 0.5 with no grades). */
#ifndef HAI_CHAT_CORE_H
#define HAI_CHAT_CORE_H
#include <stddef.h>

/* Returns 0 to keep going, 1 when the chat is done (bye), 2 bad setup, 3 ledger write failed. */
int hai_chat_turn(const char *datadir, const char *ledger, const char *input, char *out, size_t out_len);
/* Prints the replayed state and name into out, e.g. "state=idle name=bo". */
int hai_chat_state(const char *ledger, char *out, size_t out_len);
/* Grade the last reply (0 bad, 1 good, 2 to 4 better). 0 ok, 2 setup, 4 bad grade, 5 no reply yet,
 * 6 daily cap reached, 7 this reply already graded, 3 ledger write failed. */
int hai_chat_grade(const char *datadir, const char *ledger, int grade, long now, char *out, size_t out_len);
/* Prints one line per learned item: "concept <id> reward=R punish=P weight=W" and "reply ...". */
int hai_chat_stats(const char *datadir, const char *ledger, char *out, size_t out_len);
/* Calibration of stated confidence against grades (see hai_chat_core.c). 0 ok, 2 setup missing. */
int hai_chat_calibration(const char *datadir, const char *ledger, char *out, size_t out_len);

/* Mark a word "high" attention (asked about first, counts more when recognised) or "normal". 0 ok, 3 write failed, 4 bad word or level. */
int hai_chat_mark(const char *datadir, const char *ledger, const char *word, const char *level, long now, char *out, size_t out_len);

/* The teaching levers: define | alias | say | forget | pack (see hai_chat_core.c). 0 ok, 2 setup, 3 write failed, 4 bad input. */
int hai_chat_cmd(const char *datadir, const char *ledger, const char *verb, const char *arg1, const char *arg2, long now, char *out, size_t out_len);

/* The bank (what this agent knows): CONCEPT / TAUGHT / ALIAS / SAY lines, see hai_chat_core.c. 0 ok, 2 setup. */
int hai_chat_bank(const char *datadir, const char *ledger, char *out, size_t out_len);
/* The dashboard data, one line per point, replayed from the ledger (the window draws text bars from it, the web demo draws SVG):
 *   CONF|turn|confidence|concept   KNOW|turn|taught|aliases|says|marked (after that turn)   TRUST|id|weight|reward|punish   CAL|lo|hi|n|good|rate|mean_conf. 0 ok, 2 setup. */
int hai_chat_viz(const char *datadir, const char *ledger, char *out, size_t out_len);
/* Everything about ONE item, for the dashboard drill-down. item = "concept:<id>" or "taught:<word>". Lines: ITEM|concept|id|keywords|reply|trust or ITEM|taught|word|meaning|marked|trust,
 *   ALIAS|word (words aliased onto a concept)  SAY|text (the answer you set for a taught word)  USE|turn|confidence|input (every turn that used it)  HIST|n|turn|grade|trust-after (every grade that moved it)  SUM|uses|grades|reward|punish.
 * 0 ok, 2 setup, 4 no such item. */
int hai_chat_detail(const char *datadir, const char *ledger, const char *item, char *out, size_t out_len);

#endif
