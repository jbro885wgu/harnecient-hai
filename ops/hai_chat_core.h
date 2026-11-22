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

#endif
