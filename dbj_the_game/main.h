// dbj_the_game -- what the program says it is when asked.
#ifndef DBJ_THE_GAME_MAIN_H
#define DBJ_THE_GAME_MAIN_H

#include "milestone_iteration.inc"

// Two steps, because the argument of a one-step stringifier is not expanded:
// DBJ_STR_ would yield "DBJ_THE_GAME_MILESTONE", not "1".
#define DBJ_STR_(text) #text
#define DBJ_STR(text) DBJ_STR_(text)

// One string, two consumers: the --version reply and the clintro banner.
#define DBJ_THE_GAME_VERSION           \
	"milestone:" DBJ_STR(DBJ_THE_GAME_MILESTONE) \
	" iteration:" DBJ_STR(DBJ_THE_GAME_ITERATION)

#endif  // DBJ_THE_GAME_MAIN_H
