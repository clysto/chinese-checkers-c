#include "checkers.h"

#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

uint64_t _zobrist[81][3];
uint64_t _zobrist_color;

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

static inline int msb_u128(uint128_t u) {
  if (u == 0) {
    return -1;
  }
  uint64_t upper = u >> 64;
  if (upper) {
    return 127 - __builtin_clzll(upper);
  } else {
    return 63 - __builtin_clzll((uint64_t)u);
  }
}

static inline int lsb_u128(uint128_t u) {
  if (u == 0) {
    return -1;
  }
  uint64_t lower = u;
  uint64_t upper = u >> 64;
  if (lower) {
    return __builtin_ctzll(lower);
  } else {
    return 64 + __builtin_ctzll(upper);
  }
}

// Iterate over each set bit (1) in a 128-bit unsigned integer `u`,
// updating `i` with the index of the highest set bit in each iteration.
#define u128_for_each_1(u, i) \
  for (; i = bitlen_u128(u) - 1, u; u ^= ((uint128_t)1 << i))

// 1(-1) 2(-1) 9(-7) 11(-8) 18(-14) 19(-14)
#define hash_adj(p, adj)                                                       \
  (p > 10 ? ((((adj >> (p - 10)) & 2) >> 1) | (((adj >> (p - 10)) & 4) >> 1) | \
             (((adj >> (p - 10)) & 0x200) >> 7) |                              \
             (((adj >> (p - 10)) & 0x800) >> 8) |                              \
             (((adj >> (p - 10)) & 0x40000) >> 14) |                           \
             (((adj >> (p - 10)) & 0x80000) >> 14))                            \
          : ((((adj << (10 - p)) & 2) >> 1) | (((adj << (10 - p)) & 4) >> 1) | \
             (((adj << (10 - p)) & 0x200) >> 7) |                              \
             (((adj << (10 - p)) & 0x800) >> 8) |                              \
             (((adj << (10 - p)) & 0x40000) >> 14) |                           \
             (((adj << (10 - p)) & 0x80000) >> 14)))

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

const int SCORE_TABLE[81] = {
    0,  2,  2,  6,  0,  0,  10, 10, 10,  // 0
    2,  12, 13, 16, 20, 21, 14, 11, 10,  // 1
    2,  13, 13, 21, 22, 24, 23, 20, 12,  // 2
    6,  16, 21, 23, 25, 26, 28, 27, 20,  // 3
    0,  20, 22, 25, 27, 29, 30, 32, 31,  // 4
    0,  21, 24, 26, 29, 31, 33, 34, 36,  // 5
    10, 14, 23, 28, 30, 33, 35, 36, 38,  // 6
    10, 11, 20, 27, 32, 34, 36, 38, 40,  // 7
    10, 10, 12, 20, 31, 36, 38, 40, 42,  // 8
};

uint64_t rand64() {
  return rand() ^ ((uint64_t)rand() << 15) ^ ((uint64_t)rand() << 30) ^
         ((uint64_t)rand() << 45) ^ ((uint64_t)rand() << 60);
}

void init_zobrist() {
  _zobrist_color = rand64();
  for (int i = 0; i < 81; i++) {
    _zobrist[i][0] = rand64();
    _zobrist[i][1] = rand64();
    _zobrist[i][2] = rand64();
  }
}

uint64_t game_hash(struct game_t *game) {
  uint64_t hash = 0;
  uint128_t red = game->board.red;
  uint128_t green = game->board.green;
  int p;
  u128_for_each_1(red, p) { hash ^= _zobrist[p][0]; }
  u128_for_each_1(green, p) { hash ^= _zobrist[p][1]; }
  if (game->turn == PIECE_GREEN) {
    hash ^= _zobrist_color;
  }
  game->hash = hash;
  return hash;
}

int gen_moves(struct board_t *board, uint128_t from, struct list_head *moves) {
  int len = 0;
  int src, dst;
  uint128_t all = board->red | board->green;
  u128_for_each_1(from, src) {
    uint128_t to = 0;
    uint128_t adj = ADJ_POSITIONS[src];
    to |= adj;
    to &= ~all & BOARD_MASK;

    uint128_t jump_to = 0, prev_jump_to = MASK_AT(src), jumps;
    while ((prev_jump_to | jump_to) != jump_to) {
      jump_to |= prev_jump_to;
      jumps = 0;
      u128_for_each_1(prev_jump_to, dst) {
        adj = ADJ_POSITIONS[dst] & all;
        jumps |= JUMP_POSITIONS[dst][hash_adj(dst, adj)] & BOARD_MASK;
      }
      prev_jump_to = jumps & ~all;
    }
    jump_to &= ~MASK_AT(src);
    to |= jump_to;

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

bool game_is_move_valid(struct game_t *game, struct move_t *move) {
  uint128_t all = game->board.red | game->board.green;
  int to;
  if (all >> move->dst & 1 || !(all >> move->src & 1)) {
    return false;
  }
  if (MASK_AT(move->src) &
      (game->turn == PIECE_RED ? game->board.green : game->board.red)) {
    return false;
  }
  // check for adjacent moves
  if (ADJ_POSITIONS[move->src] & MASK_AT(move->dst)) {
    return true;
  }
  // check for jumps
  uint128_t jump_to = 0, prev_jump_to = MASK_AT(move->src), jumps = 0;
  uint128_t adj;
  while ((prev_jump_to | jump_to) != jump_to) {
    jump_to |= prev_jump_to;
    jumps = 0;
    u128_for_each_1(prev_jump_to, to) {
      adj = ADJ_POSITIONS[to] & all;
      jumps |= JUMP_POSITIONS[to][hash_adj(to, adj)] & BOARD_MASK;
      if (jumps & MASK_AT(move->dst)) {
        return true;
      }
    }
    prev_jump_to = jumps & ~all;
  }
  return false;
}

void sort_moves(struct list_head *moves, enum color_t color) {
  struct list_head buckets[64];
  for (int i = 0; i < 64; i++) {
    INIT_LIST_HEAD(&buckets[i]);
  }

  struct list_head *pos, *_n;
  struct move_t *move;
  list_for_each_safe(pos, _n, moves) {
    move = list_entry(pos, struct move_t, list);
    list_del(pos);
    int distance = BOARD_DISTANCES[move->dst] - BOARD_DISTANCES[move->src];
    list_add_tail(
        pos,
        &buckets[distance + 16]);  // Offset by 16 to handle negative distances
  }

  if (color == PIECE_RED) {
    for (int i = 0; i < 64; i++) {
      list_splice_tail(&buckets[i], moves);
    }
  } else {
    for (int i = 63; i >= 0; i--) {
      list_splice_tail(&buckets[i], moves);
    }
  }
}

void game_apply_move(struct game_t *game, struct move_t *move) {
  if (game->hash != 0) {
    game->hash ^= _zobrist[move->src][game->turn];
    game->hash ^= _zobrist[move->dst][game->turn];
    game->hash ^= _zobrist_color;
  }
  if (game->turn == PIECE_RED) {
    game->board.red &= ~MASK_AT(move->src);
    game->board.red |= MASK_AT(move->dst);
    game->turn = PIECE_GREEN;
  } else {
    game->board.green &= ~MASK_AT(move->src);
    game->board.green |= MASK_AT(move->dst);
    game->turn = PIECE_RED;
    game->round++;
  }
}

void game_undo_move(struct game_t *game, struct move_t *move) {
  if (game->turn == PIECE_GREEN) {
    game->board.red &= ~MASK_AT(move->dst);
    game->board.red |= MASK_AT(move->src);
    game->turn = PIECE_RED;
  } else {
    game->board.green &= ~MASK_AT(move->dst);
    game->board.green |= MASK_AT(move->src);
    game->turn = PIECE_GREEN;
    game->round--;
  }
  if (game->hash != 0) {
    game->hash ^= _zobrist[move->src][game->turn];
    game->hash ^= _zobrist[move->dst][game->turn];
    game->hash ^= _zobrist_color;
  }
}

void game_apply_null_move(struct game_t *game) {
  if (game->hash != 0) {
    game->hash ^= _zobrist_color;
  }
  game->turn = game->turn == PIECE_RED ? PIECE_GREEN : PIECE_RED;
  if (game->turn == PIECE_RED) {
    game->round++;
  }
}

void game_undo_null_move(struct game_t *game) {
  game->turn = game->turn == PIECE_RED ? PIECE_GREEN : PIECE_RED;
  if (game->turn == PIECE_GREEN) {
    game->round--;
  }
  if (game->hash != 0) {
    game->hash ^= _zobrist_color;
  }
}

int game_apply_move_with_check(struct game_t *game, struct move_t *move) {
  if (game->turn == PIECE_RED) {
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

  game_apply_move(game, move);

  game->turn = game->turn == PIECE_RED ? PIECE_GREEN : PIECE_RED;
  if (game->turn == PIECE_RED) {
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
  str[p++] = game->turn == PIECE_RED ? 'r' : 'g';
  p += sprintf(&str[p], " %d", game->round);
  str[p++] = '\0';
}

void init_game(struct game_t *game) {
  game->board.red = INITIAL_RED;
  game->board.green = INITIAL_GREEN;
  game->turn = PIECE_RED;
  game->round = 1;
  game_hash(game);
}

void load_game(struct game_t *game, char *state) {
  game->board.red = 0;
  game->board.green = 0;
  game->turn = PIECE_RED;
  game->round = 0;
  game->hash = 0;
  char round[16];
  int round_len = 0;
  int p = 0;
  int index = 0;
  while (state[p] != '\0') {
    if (index < 81) {
      if (state[p] == '1') {
        game->board.red |= MASK_AT(index);
        index++;
      } else if (state[p] == '2') {
        game->board.green |= MASK_AT(index);
        index++;
      } else if (state[p] == '0') {
        index++;
      }
    } else {
      if (state[p] == 'r') {
        game->turn = PIECE_RED;
      } else if (state[p] == 'g') {
        game->turn = PIECE_GREEN;
      } else if (state[p] >= '0' && state[p] <= '9' && round_len < 15) {
        round[round_len++] = state[p];
      }
    }
    p++;
  }
  round[round_len] = '\0';
  game->round = atoi(round);
  game_hash(game);
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
}

int game_evaluate(struct game_t *game) {
  int p, red_score = 0, green_score = 0, red_moves_score = 0,
         green_moves_score = 0;
  uint128_t red = game->board.red;
  uint128_t green = game->board.green;

  if (red == INITIAL_GREEN) {
    return game->turn == PIECE_RED ? SCORE_WIN : -SCORE_WIN;
  }
  if (green == INITIAL_RED) {
    return game->turn == PIECE_GREEN ? SCORE_WIN : -SCORE_WIN;
  }

  LIST_HEAD(red_moves);
  LIST_HEAD(green_moves);

  gen_moves(&(game->board), red, &red_moves);
  gen_moves(&(game->board), green, &green_moves);

  struct list_head *pos, *_n;
  list_for_each_safe(pos, _n, &red_moves) {
    struct move_t *move = list_entry(pos, struct move_t, list);
    if (BOARD_DISTANCES[move->src] - BOARD_DISTANCES[move->dst] > 0) {
      red_moves_score += 1;
    }
    list_del(pos);
    free(move);
  }
  list_for_each_safe(pos, _n, &green_moves) {
    struct move_t *move = list_entry(pos, struct move_t, list);
    if (BOARD_DISTANCES[move->dst] - BOARD_DISTANCES[move->src] > 0) {
      green_moves_score += 1;
    }
    list_del(pos);
    free(move);
  }

  u128_for_each_1(red, p) { red_score += (SCORE_TABLE[80 - p]); }
  u128_for_each_1(green, p) { green_score += (SCORE_TABLE[p]); }

  red_score = 3 * red_score + red_moves_score;
  green_score = 3 * green_score + green_moves_score;

  return game->turn == PIECE_RED ? red_score - green_score
                                 : green_score - red_score;
}