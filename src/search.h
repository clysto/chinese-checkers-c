#ifndef _SEARCH_H
#define _SEARCH_H

#include "checkers.h"

struct search_result_t {
  struct move_t best_move;
  int searched_nodes;
};

int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move);

#endif  // _SEARCH_H