#include "search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

#define forward_distance(color, src, dst)                             \
  (color == PIECE_GREEN ? BOARD_DISTANCES[dst] - BOARD_DISTANCES[src] \
                        : BOARD_DISTANCES[src] - BOARD_DISTANCES[dst])

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

#define NULL_MOVE_R 3
#define TABLE_SIZE (1 << 22)
#define TABLE_MASK ((1 << 22) - 1)
#define MAX_DEPTH 64

struct hash_entry_t _hash_table[TABLE_SIZE];
struct move_t _killer_moves[MAX_DEPTH][2];

int alpha_beta_search(struct game_t *game, int depth, int alpha, int beta,
                      struct move_t *best_move, clock_t stop_time) {
  int score;
  struct list_head *pos, *_n;
  struct move_t *move;
  struct move_t _best_move, _hash_move = {-1, -1}, _killer_move0, _killer_move1;
  enum hash_flag_t flag = HASH_ALPHA;
  bool found_pv = false, already_gen_moves = false;
  struct hash_entry_t *entry = probe_hash(game->hash, depth, alpha, beta);
  LIST_HEAD(moves);

  // Look up hash table
  if (entry != NULL) {
    if (entry->depth >= depth) {
      if (entry->flag == HASH_EXACT) {
        *best_move = entry->best;
      }
      return entry->value;
    }
    // history best move
    if (entry->flag == HASH_EXACT || entry->flag == HASH_BETA) {
      _hash_move = entry->best;
    }
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

  pos = moves.next;
  if (_killer_moves[depth][0].src != -1 &&
      game_is_move_valid(game, &_killer_moves[depth][0])) {
    // try killer move 0
    _killer_move0 = _killer_moves[depth][0];
    _killer_move0.list.next = pos;
    pos = &_killer_move0.list;
  }
  if (_killer_moves[depth][1].src != -1 &&
      game_is_move_valid(game, &_killer_moves[depth][1])) {
    // try killer move 1
    _killer_move1 = _killer_moves[depth][1];
    _killer_move1.list.next = pos;
    pos = &_killer_move1.list;
  }
  if (_hash_move.src != -1) {
    // try hash move first
    _hash_move.list.next = pos;
    pos = &_hash_move.list;
  }
  while (pos != &moves || !already_gen_moves) {
    if (pos == &moves) {
      // the history best move and killer moves can't cut off the search
      // generate normal moves
      gen_moves(&(game->board),
                game->turn == PIECE_RED ? game->board.red : game->board.green,
                &moves);
      sort_moves(&moves, game->turn);
      already_gen_moves = true;
      pos = moves.next;
      continue;
    }

    move = list_entry(pos, struct move_t, list);

    if (forward_distance(game->turn, move->src, move->dst) < -1) {
      // skip backward moves
      pos = pos->next;
      continue;
    }

    game_apply_move(game, move);
    if (found_pv) {
      score = -alpha_beta_search(game, depth - 1, -alpha - 1, -alpha,
                                 &_best_move, stop_time);
      if (score > alpha && score < beta) {
        score = -alpha_beta_search(game, depth - 1, -beta, -alpha, &_best_move,
                                   stop_time);
      }
    } else {
      score = -alpha_beta_search(game, depth - 1, -beta, -alpha, &_best_move,
                                 stop_time);
    }
    game_undo_move(game, move);

    if (score >= beta) {
      _killer_moves[depth][1] = _killer_moves[depth][0];
      _killer_moves[depth][0] = *move;
      record_hash(game->hash, beta, depth, HASH_BETA, move);
      free_moves(&moves);
      return beta;
    }
    if (score > alpha) {
      *best_move = *move;
      flag = HASH_EXACT;
      found_pv = true;
      alpha = score;
    }
    if (clock() > stop_time) {
      // return SCORE_NAN;
      break;
    }
    pos = pos->next;
  }

  record_hash(game->hash, alpha, depth, flag, best_move);
  free_moves(&moves);
  return alpha;
}

void record_hash(uint64_t hash, int value, int depth, enum hash_flag_t flag,
                 struct move_t *best) {
  struct hash_entry_t *entry = &_hash_table[hash & TABLE_MASK];
  if (entry->hash == hash && entry->depth > depth) {
    return;
  }
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

void clear_killer_moves() {
  for (int i = 0; i < MAX_DEPTH; i++) {
    _killer_moves[i][0] = (struct move_t){-1, -1};
    _killer_moves[i][1] = (struct move_t){-1, -1};
  }
}