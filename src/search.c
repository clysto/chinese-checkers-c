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
#define is_forwards(color, src, dst) \
  ((color == PIECE_RED && src >= dst) || (color == PIECE_GREEN && src <= dst))

#define NULL_MOVE_R 2
#define TABLE_SIZE (1 << 22)
#define TABLE_MASK ((1 << 22) - 1)

struct hash_entry_t _hash_table[TABLE_SIZE];

int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move, clock_t stop_time) {
  int score;
  struct list_head *pos, *_n;
  struct move_t *move;
  struct move_t _best_move;
  enum hash_flag_t flag = HASH_ALPHA;
  struct hash_entry_t *entry = probe_hash(game->hash, depth, alpha, beta);
  LIST_HEAD(moves);

  // Look up hash table
  if (entry != NULL && entry->depth >= depth) {
    if (entry->flag == HASH_EXACT) {
      *best_move = entry->best;
    }
    return entry->value;
  }

  if (depth <= 0) {
    return game_evaluate(game);
  }

  // Null-Move Forward Pruning
  if (depth - 1 - NULL_MOVE_R >= 0) {
    game_apply_null_move(game);
    score = -alpha_beta_search(game, depth - 1 - NULL_MOVE_R, -beta, -beta + 1,
                               &_best_move, stop_time);
    game_undo_null_move(game);
    if (score >= beta) {
      return beta;
    }
  }

  gen_moves(&(game->board),
            game->turn == PIECE_RED ? game->board.red : game->board.green, &moves);
  sort_moves(&moves, game->turn);
  list_for_each_safe(pos, _n, &moves) {
    move = list_entry(pos, struct move_t, list);
    game_apply_move(game, move);
    score = -alpha_beta_search(game, depth - 1, -beta, -alpha, &_best_move,
                               stop_time);
    game_undo_move(game, move);
    if (score >= beta) {
      free_moves(&moves);
      record_hash(game->hash, beta, depth, HASH_BETA, move);
      return beta;
    }
    if (score > alpha) {
      *best_move = *move;
      flag = HASH_EXACT;
      alpha = score;
    }
    if (clock() > stop_time) {
      return SCORE_NAN;
    }
  }

  record_hash(game->hash, alpha, depth, flag, best_move);
  free_moves(&moves);
  return alpha;
}

void record_hash(uint64_t hash, int value, int depth, enum hash_flag_t flag,
                 struct move_t *best) {
  struct hash_entry_t *entry = &_hash_table[hash & TABLE_MASK];
  entry->hash = hash;
  entry->value = value;
  entry->depth = depth;
  entry->flag = flag;
  entry->best = *best;
}

struct hash_entry_t *probe_hash(uint64_t hash, int depth, int alpha, int beta) {
  struct hash_entry_t *entry = &_hash_table[hash & TABLE_MASK];
  if (entry->hash == hash) {
    if (entry->flag == HASH_EXACT) {
      return entry;
    } else if (entry->flag == HASH_ALPHA && entry->value <= alpha) {
      return entry;
    } else if (entry->flag == HASH_BETA && entry->value >= beta) {
      return entry;
    }
  }
  return NULL;
}

void clear_hash_table() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    _hash_table[i].hash = 0;
  }
}

int mtdf_search(struct game_t *game, int depth, int guess,
                struct move_t *best_move, clock_t stop_time) {
  int beta;
  int upperbound = SCORE_MAX;
  int lowerbound = SCORE_MIN;
  int score = guess;
  do {
    beta = (score == lowerbound ? score + 1 : score);
    score =
        alpha_beta_search(game, depth, beta - 1, beta, best_move, stop_time);
    if (score < beta) {
      upperbound = score;
    } else {
      lowerbound = score;
    }
  } while (lowerbound < upperbound);
  return score;
}
