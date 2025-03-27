#ifndef _CHECKERS_H
#define _CHECKERS_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "list.h"

#define uint128_t unsigned __int128

#define BOARD_MASK (((uint128_t)0x1ffff << 64) | 0xffffffffffffffff)
#define INITIAL_RED (((uint128_t)0x1e0e0 << 64) | 0x6020000000000000)
#define INITIAL_GREEN 0x80c0e0f
#define MASK_AT(p) ((uint128_t)1 << p)
#define SCORE_MAX (INT_MAX)
#define SCORE_MIN (-INT_MAX)
#define SCORE_NAN (INT_MIN)
#define SCORE_WIN (99999)

extern const int BOARD_DISTANCES[81];

enum color_t {
  PIECE_RED,
  PIECE_GREEN,
};

struct board_t {
  uint128_t red;
  uint128_t green;
};

struct game_t {
  struct board_t board;
  enum color_t turn;
  int round;
  uint64_t hash;
};

struct move_t {
  int8_t src;
  int8_t dst;
  struct list_head list;
};

#define INIT_BOARD {INITIAL_RED, INITIAL_GREEN}

void init_zobrist();

int gen_moves(struct board_t *board, uint128_t from, struct list_head *moves);

void sort_moves(struct list_head *moves, enum color_t color);

void jump_moves(struct board_t *board, int src, uint128_t *to);

void draw_board(struct board_t *board);

void game_str(struct game_t *game, char *str);

int game_apply_move_with_check(struct game_t *game, struct move_t *move);

void game_apply_move(struct game_t *game, struct move_t *move);

void game_undo_move(struct game_t *game, struct move_t *move);

void game_apply_null_move(struct game_t *game);

void game_undo_null_move(struct game_t *game);

int game_evaluate(struct game_t *game);

void load_game(struct game_t *game, char *state);

void init_game(struct game_t *game);

bool game_is_move_valid(struct game_t *game, struct move_t *move);

bool is_game_over(struct game_t *game);

uint64_t game_hash(struct game_t *game);

#endif  // _CHECKERS_H