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
  int n = atoi(argv[1]);

  srand(time(NULL));
  struct board_t board = {INITIAL_RED, 0};
  LIST_HEAD(moves);
  for (int i = 0; i < n; i++) {
    if (gen_moves(&board, board.red, &moves) == 0) {
      printf("No moves left\n");
      break;
    }
    struct move_t *entry;
    struct list_head *pos, *_n;
    list_for_each_safe(pos, _n, &moves) {
      entry = list_entry(pos, struct move_t, list);
      if (BOARD_DISTANCES[entry->dst] - BOARD_DISTANCES[entry->src] > 0) {
        list_del(pos);
        free(entry);
      }
    }
    if (list_empty(&moves)) {
      printf("No forward moves left\n");
      break;
    }
    int n = rand() % list_len(&moves);
    list_nth_entry(entry, &moves, n, list);
    printf("Move: %02d->%02d\n", entry->src, entry->dst);
    apply_move(&board, entry, RED);
    draw_board(&board);

    list_for_each_safe(pos, _n, &moves) {
      entry = list_entry(pos, struct move_t, list);
      list_del(pos);
      free(entry);
    }
  }
  return 0;
}

int main(void) {
  struct game_t game = {INIT_BOARD, RED, 1};
  struct move_t best_move = {-1, -1};
  draw_board(&game.board);
  alpha_beta_search(&game, 5, -INT_MAX, INT_MAX, &best_move);
  printf("Best move: %02d->%02d\n", best_move.src, best_move.dst);
  return 0;
}