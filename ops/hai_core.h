/* hai_core: pure reply logic for M1. No I/O here, so a test can call it.
 * Numbered menu (nav standard): 1 = help, 2 = quit. Replies are fixed rows for now;
 * M3 moves them into replies.pdl. Returns 0 to keep going, 1 to quit. */
#ifndef HAI_CORE_H
#define HAI_CORE_H
#include <stddef.h>

const char *hai_menu(void);
int hai_reply(const char *line, char *out, size_t out_len);

#endif
