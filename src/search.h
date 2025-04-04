#ifndef _SEARCH_H
#define _SEARCH_H

#include <time.h>
#include <stdbool.h>

#include "checkers.h"

struct search_result_t {
  struct move_t best_move;
  int searched_nodes;
};

enum hash_flag_t {
  HASH_EXACT,
  HASH_ALPHA,
  HASH_BETA,
};

struct hash_entry_t {
  uint64_t hash;
  int value;
  int depth;
  enum hash_flag_t flag;
  struct move_t best;
};


int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move, clock_t stop_time);

void record_hash(uint64_t hash, int value, int depth, enum hash_flag_t flag,
                 struct move_t *best);

struct hash_entry_t *probe_hash(uint64_t hash, int depth, int alpha, int beta);

void clear_hash_table();

void clear_killer_moves();

#endif  // _SEARCH_H