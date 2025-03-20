#include "checkers.h"

#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

static inline int bitlen_u128(uint128_t u) {
  if (u == 0) {
    return 0;
  }
  uint64_t upper = u >> 64;
  if (upper) {
    return 128 - __builtin_clzll(upper);
  } else {
    return 64 - __builtin_clzll((uint64_t)u);
  }
}
// Iterate over each set bit (1) in a 128-bit unsigned integer `u`,
// updating `i` with the index of the highest set bit in each iteration.
#define u128_for_each_1(u, i) \
  for (; i = bitlen_u128(u) - 1, u; u ^= ((uint128_t)1 << i))

// 1(-1) 2(-1) 9(-7) 11(-8) 18(-14) 19(-14)
#define hash_adj(p, adj)                                                     \
  ((((adj >> (p - 10)) & 2) >> 1) | (((adj >> (p - 10)) & 4) >> 1) |         \
   (((adj >> (p - 10)) & 0x200) >> 7) | (((adj >> (p - 10)) & 0x800) >> 8) | \
   (((adj >> (p - 10)) & 0x40000) >> 14) |                                   \
   (((adj >> (p - 10)) & 0x80000) >> 14))

const int BOARD_DISTANCES[81] = {
    0, 1, 2,  3,  4,  5,  6,  7,  8,   // 0
    1, 2, 3,  4,  5,  6,  7,  8,  9,   // 1
    2, 3, 4,  5,  6,  7,  8,  9,  10,  // 2
    3, 4, 5,  6,  7,  8,  9,  10, 11,  // 3
    4, 5, 6,  7,  8,  9,  10, 11, 12,  // 4
    5, 6, 7,  8,  9,  10, 11, 12, 13,  // 5
    6, 7, 8,  9,  10, 11, 12, 13, 14,  // 6
    7, 8, 9,  10, 11, 12, 13, 14, 15,  // 7
    8, 9, 10, 11, 12, 13, 14, 15, 16,  // 8
};

int gen_moves(struct board_t *board, uint128_t from, struct list_head *moves) {
  int len = 0;
  int src, dst;
  u128_for_each_1(from, src) {
    uint128_t to = 0;
    uint128_t adj = ADJ_POSITIONS[src];
    to |= adj;
    to &= ~(board->red | board->green) & BOARD_MASK;
    jump_moves(board, src, &to);
    u128_for_each_1(to, dst) {
      struct move_t *m = (struct move_t *)malloc(sizeof(struct move_t));
      m->src = src;
      m->dst = dst;
      list_add_tail(&m->list, moves);
      len++;
    }
  }
  return len;
}

void jump_moves(struct board_t *board, int src, uint128_t *to) {
  uint128_t adj = ADJ_POSITIONS[src] & (board->red | board->green);
  uint128_t jumps = JUMP_POSITIONS[src][hash_adj(src, adj)] & BOARD_MASK;
  jumps &= ~(board->red | board->green);
  if ((jumps | *to) == *to) {
    // no more jumps
    return;
  }
  *to |= jumps;
  u128_for_each_1(jumps, src) { jump_moves(board, src, to); }
  return;
}

void apply_move(struct board_t *board, struct move_t *move, enum color_t turn) {
  if (turn == RED) {
    board->red &= ~MASK_AT(move->src);
    board->red |= MASK_AT(move->dst);
  } else {
    board->green &= ~MASK_AT(move->src);
    board->green |= MASK_AT(move->dst);
  }
}

void game_apply_move(struct game_t *game, struct move_t *move) {
  if (game->turn == RED) {
    game->board.red &= ~MASK_AT(move->src);
    game->board.red |= MASK_AT(move->dst);
    game->turn = GREEN;
  } else {
    game->board.green &= ~MASK_AT(move->src);
    game->board.green |= MASK_AT(move->dst);
    game->turn = RED;
    game->round++;
  }
}

void game_undo_move(struct game_t *game, struct move_t *move) {
  if (game->turn == GREEN) {
    game->board.red &= ~MASK_AT(move->dst);
    game->board.red |= MASK_AT(move->src);
    game->turn = RED;
  } else {
    game->board.green &= ~MASK_AT(move->dst);
    game->board.green |= MASK_AT(move->src);
    game->turn = GREEN;
    game->round--;
  }
}

int game_apply_move_with_check(struct game_t *game, struct move_t *move) {
  if (game->turn == RED) {
    if (!(game->board.red >> move->src & 1)) {
      return -1;
    }
  } else {
    if (!(game->board.green >> move->src & 1)) {
      return -1;
    }
  }

  LIST_HEAD(moves);
  gen_moves(&(game->board), MASK_AT(move->src), &moves);
  if (list_empty(&moves)) {
    return -1;
  }

  struct move_t *m;
  int found = 0;
  while (!list_empty(&moves)) {
    m = list_head_entry(&moves, struct move_t, list);
    if (m->dst == move->dst) {
      found = 1;
    }
    list_del(&m->list);
    free(m);
  }

  if (!found) {
    return -1;
  }

  apply_move(&(game->board), move, game->turn);

  game->turn = game->turn == RED ? GREEN : RED;
  if (game->turn == RED) {
    game->round++;
  }
  return 0;
}

void game_str(struct game_t *game, char *str) {
  int p = 0;
  for (int i = 0; i < 81; i++) {
    if (game->board.red >> i & 1) {
      str[p++] = '1';
    } else if (game->board.green >> i & 1) {
      str[p++] = '2';
    } else {
      str[p++] = '0';
    }
    if (i % 9 == 8 && i != 80) {
      str[p++] = '/';
    }
  }
  str[p++] = ' ';
  str[p++] = game->turn == RED ? 'r' : 'g';
  p += sprintf(&str[p], " %d", game->round);
  str[p++] = '\0';
}

void draw_board(struct board_t *board) {
  int p = 0;
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 8 - i; j++) {
      printf("  ");
    }
    p = 9 * i;
    for (int j = 0; j < i + 1; j++) {
      if (board->red >> p & 1) {
        printf("\033[91m%02d\033[0m  ", p);
      } else if (board->green >> p & 1) {
        printf("\033[92m%02d\033[0m  ", p);
      } else {
        printf("\033[90m%02d\033[0m  ", p);
      }
      p -= 8;
    }
    printf("\n");
  }
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < i + 1; j++) {
      printf("  ");
    }
    p = 73 + i;
    for (int j = 0; j < 8 - i; j++) {
      if (board->red >> p & 1) {
        printf("\033[91m%02d\033[0m  ", p);
      } else if (board->green >> p & 1) {
        printf("\033[92m%02d\033[0m  ", p);
      } else {
        printf("\033[90m%02d\033[0m  ", p);
      }
      p -= 8;
    }
    printf("\n");
  }
  printf("\n");
}

int game_evaluate(struct game_t *game) {
  uint128_t current_color =
      game->turn == RED ? game->board.red : game->board.green;
  int p, score = 0;
  u128_for_each_1(current_color, p) {
    if (game->turn == RED) {
      score += (16 - BOARD_DISTANCES[p]);
    } else {
      score += BOARD_DISTANCES[p];
    }
  }
  return score;
}