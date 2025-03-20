#include "search.h"

#include <stdio.h>

#include "list.h"
#include "stdlib.h"

#define free_moves(moves)                          \
  do {                                             \
    list_for_each_safe(pos, _n, moves) {           \
      move = list_entry(pos, struct move_t, list); \
      list_del(pos);                               \
      free(move);                                  \
    }                                              \
  } while (0)

int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move) {
  int score;
  struct list_head *pos, *_n;
  struct move_t *move;
  LIST_HEAD(moves);

  if (depth == 0) {
    return game_evaluate(game);
  }

  gen_moves(&(game->board),
            game->turn == RED ? game->board.red : game->board.green, &moves);
  list_for_each_safe(pos, _n, &moves) {
    move = list_entry(pos, struct move_t, list);
    game_apply_move(game, move);
    score = -alpha_beta_search(game, depth - 1, -beta, -alpha, best_move);
    game_undo_move(game, move);
    if (score >= beta) {
      free_moves(&moves);
      return beta;
    }
    if (score > alpha) {
      *best_move = *move;
      alpha = score;
    }
  }

  free_moves(&moves);
  return alpha;
}