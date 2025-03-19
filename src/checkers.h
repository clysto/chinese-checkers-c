#ifndef _CHECKERS_H
#define _CHECKERS_H

#include <stdint.h>

#include "list.h"

#define uint128_t unsigned __int128

#define BOARD_MASK (((uint128_t)0x1ffff << 64) | 0xffffffffffffffff)
#define INITIAL_RED (((uint128_t)0x1e0e0 << 64) | 0x6020000000000000)
#define INITIAL_GREEN 0x80c0e0f
#define MASK_AT(p) ((uint128_t)1 << p)

enum color_t {
  RED,
  GREEN,
};

struct board_t {
  uint128_t red;
  uint128_t green;
};

struct game_t {
  struct board_t board;
  enum color_t turn;
  int round;
};

struct move_t {
  int8_t src;
  int8_t dst;
  struct list_head list;
};

#define INIT_BOARD {INITIAL_RED, INITIAL_GREEN}

int gen_moves(struct board_t *board, uint128_t from, struct list_head *moves);

void jump_moves(struct board_t *board, int src, uint128_t *to);

void draw_board(struct board_t *board);

void apply_move(struct board_t *board, struct move_t *move, enum color_t turn);

void game_str(struct game_t *game, char *str);

int game_apply_move(struct game_t *game, struct move_t *move);

#endif  // _CHECKERS_H