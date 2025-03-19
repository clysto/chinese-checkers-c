#include <stdio.h>
#include <stdlib.h>

#include "checkers.h"
#include "list.h"

void print_all_moves(struct list_head *head) {
  struct move_t *entry;
  list_for_each_entry(entry, head, list) {
    printf("Move: %02d->%02d\n", entry->src, entry->dst);
  }
}

int main(void) {
  struct game_t game = {INIT_BOARD, RED, 1};
  LIST_HEAD(moves);
  game_apply_move(&game, &(struct move_t){53, 52});
  game_apply_move(&game, &(struct move_t){27, 28});
  game_apply_move(&game, &(struct move_t){71, 51});
  game_apply_move(&game, &(struct move_t){11, 12});
  draw_board(&(game.board));
  int l = gen_moves(&(game.board), game.board.red, &moves);
  printf("Generated %d moves\n", l);
  print_all_moves(&moves);
  return 0;
}