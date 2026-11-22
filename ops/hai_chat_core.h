/* hai_chat_core: the FSM chat engine (M3). Pure file in, file out; used by hai and by hai_chat op.
 * State is never held in memory: it is replayed from the chat ledger (TURN rows) on every call. */
#ifndef HAI_CHAT_CORE_H
#define HAI_CHAT_CORE_H
#include <stddef.h>

/* Returns 0 to keep going, 1 when the chat is done (bye), 2 bad setup, 3 ledger write failed. */
int hai_chat_turn(const char *datadir, const char *ledger, const char *input, char *out, size_t out_len);
/* Prints the replayed state and name into out, e.g. "state=idle name=bo". */
int hai_chat_state(const char *ledger, char *out, size_t out_len);

#endif
