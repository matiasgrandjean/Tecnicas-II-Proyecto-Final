#ifndef NOCANONICO_H
#define NOCANONICO_H

#include <termios.h>

int setup_nocanonico_bloq(struct termios *orig_t);
int setup_nocanonico_nobloq(struct termios *orig_t, int *orig_flags);
void restaurarTerminal(const struct termios *orig_t, int orig_flags);

#endif
