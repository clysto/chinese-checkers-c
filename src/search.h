#ifndef _SEARCH_H
#define _SEARCH_H

#include "checkers.h"

int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move);

#endif  // _SEARCH_H