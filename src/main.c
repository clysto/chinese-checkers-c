#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "checkers.h"
#include "list.h"
#include "search.h"

void print_all_moves(struct list_head *head) {
  struct move_t *entry;
  list_for_each_entry(entry, head, list) {
    printf("Move: %02d->%02d\n", entry->src, entry->dst);
  }
}

int main1(int argc, char *argv[]) {
  struct game_t game = {INIT_BOARD, RED, 1};
  LIST_HEAD(moves);
  game_apply_move(&game, &(struct move_t){53, 52});
  game_apply_move(&game, &(struct move_t){27, 28});
  game_apply_move(&game, &(struct move_t){71, 51});
  gen_moves(&game.board, game.board.red, &moves);
  sort_moves(&moves, 1);
  print_all_moves(&moves);
  draw_board(&game.board);
  return 0;
}

int main(void) {
  init_zobrist();
  struct game_t game = {INIT_BOARD, RED, 1, 0};
  game_hash(&game);
  struct move_t user_move = {-1, -1};
  struct move_t best_move = {-1, -1};

  while (1) {
    draw_board(&game.board);

    if (game.turn == RED) {
      printf("Your turn (RED). Enter your move (src dst): ");
      if (scanf("%hhu %hhu", &user_move.src, &user_move.dst) != 2) {
        printf("Invalid input. Please enter two integers.\n");
        continue;
      }
      game_apply_move(&game, &user_move);
    } else {
      printf("AI's turn (GREEN).\n");
      alpha_beta_search(&game, 8, -INT_MAX, INT_MAX, &best_move);
      printf("AI move: %02d->%02d\n", best_move.src, best_move.dst);
      game_apply_move(&game, &best_move);
    }
  }

  return 0;
}

// int main(void) {
//   init_zobrist();
//   struct game_t game = {INIT_BOARD, RED, 1, 0};
//   game_hash(&game);
//   printf("game hash: %" PRIu64 "\n", game.hash);
//   game_apply_move(&game, &(struct move_t){53, 52});
//   game_apply_move(&game, &(struct move_t){61, 43});
//   printf("game hash: %" PRIu64 "\n", game.hash);
//   game_undo_move(&game, &(struct move_t){61, 43});
//   game_undo_move(&game, &(struct move_t){53, 52});
//   printf("game hash: %" PRIu64 "\n", game.hash);
//   draw_board(&game.board);
//   return 0;
// }