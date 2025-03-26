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
  struct game_t game = {INIT_BOARD, PIECE_RED, 1};
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

// int main(void) {
//   init_zobrist();
//   struct game_t game = {INIT_BOARD, PIECE_RED, 1, 0};
//   game_hash(&game);
//   struct move_t user_move = {-1, -1};
//   struct move_t best_move = {-1, -1};

//   while (1) {
//     draw_board(&game.board);

//     if (game.turn == PIECE_RED) {
//       printf("Your turn (RED). Enter your move (src dst): ");
//       if (scanf("%hhu %hhu", &user_move.src, &user_move.dst) != 2) {
//         printf("Invalid input. Please enter two integers.\n");
//         continue;
//       }
//       game_apply_move(&game, &user_move);
//     } else {
//       printf("AI's turn (GREEN).\n");
//       clock_t stop_time = clock() + CLOCKS_PER_SEC * 8;
//       clear_hash_table();
//       for (int d = 3; d <= 16; d++) {
//         if (clock() > stop_time) {
//           break;
//         }
//         struct move_t _best_move;
//         int eval = alpha_beta_search(&game, d, SCORE_MIN, SCORE_MAX,
//                                      &_best_move, stop_time);
//         if (eval != SCORE_NAN) {
//           best_move = _best_move;
//           printf("Depth: %d, Eval: %d\n", d, eval);
//         }
//       }

//       printf("AI move: %02d->%02d\n", best_move.src, best_move.dst);
//       game_apply_move(&game, &best_move);
//     }
//   }

//   return 0;
// }

int main(void) {
  LIST_HEAD(moves);
  init_zobrist();
  struct game_t game;
  load_game(&game, "222200000/222000000/220000000/020000000/000000000/000000011/000000011/000000110/000001111 g 2");
  // init_game(&game);
  // char s[128];
  // game_str(&game, s);
  // printf("%s\n", s);
  // game_apply_move(&game, &(struct move_t){53, 52});
  // game_apply_move(&game, &(struct move_t){27, 28});
  // game_apply_move(&game, &(struct move_t){71, 51});
  draw_board(&game.board);
  for (int src = 0; src <= 80; src++) {
    for (int dst = 0; dst <= 80; dst++) {
      if (game_is_move_valid(&game, &(struct move_t){src, dst})) {
        printf("Move: %02d->%02d\n", src, dst);
      }
    }
  }
  gen_moves(&game.board, game.board.green, &moves);
  printf("\n");
  print_all_moves(&moves);
  // int a = game_is_move_valid(&game, &(struct move_t){52, 29});
  // printf("%d\n", a);
  // game_apply_move(&game, &(struct move_t){62, 44});
  // draw_board(&game.board);
  // game_undo_move(&game, &(struct move_t){62, 44});
  // draw_board(&game.board);

  return 0;
}