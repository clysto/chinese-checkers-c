#include <pthread.h>
#include <raylib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../checkers.h"
#include "../list.h"
#include "../search.h"

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 600
#define SQRT3 (1.7320508075688772)

struct game_t game;
int selected = -1;
enum color_t player_color;
uint128_t selected_moves = 0;
atomic_bool search_done = false;
atomic_bool search_running = false;
struct move_t ai_last_move = {-1, -1};

int PIECES_MAPPING[81] = {
    0,  9,  1,  18, 10, 2,  27, 19, 11, 3,  36, 28, 20, 12, 04, 45, 37,
    29, 21, 13, 5,  54, 46, 38, 30, 22, 14, 6,  63, 55, 47, 39, 31, 23,
    15, 07, 72, 64, 56, 48, 40, 32, 24, 16, 8,  73, 65, 57, 49, 41, 33,
    25, 17, 74, 66, 58, 50, 42, 34, 26, 75, 67, 59, 51, 43, 35, 76, 68,
    60, 52, 44, 77, 69, 61, 53, 78, 70, 62, 79, 71, 80};

void *search_ai_move(void *arg) {
  ai_last_move = (struct move_t){-1, -1};
  struct game_t _game = game;
  struct move_t best_move;
  clock_t stop_time = clock() + CLOCKS_PER_SEC * 5;
  clear_hash_table();
  int alpha = SCORE_MIN;
  int beta = SCORE_MAX;
  int val_window = 100;
  for (int d = 1; d <= 32; d++) {
    if (clock() > stop_time) {
      break;
    }
    clear_killer_moves();
    struct move_t _best_move;
    int eval =
        alpha_beta_search(&_game, d, alpha, beta, &_best_move, stop_time);
    if (eval != SCORE_NAN) {
      printf("Depth: %2d, Eval: %6d, Move: %02d->%02d\n", d, eval, _best_move.src,
             _best_move.dst);
      if (eval == SCORE_WIN) {
        best_move = _best_move;
        break;
      }
      best_move = _best_move;
    }
    // alpha = eval - val_window;
    // beta = eval + val_window;
  }
  printf("AI move: %02d->%02d\n", best_move.src, best_move.dst);
  game_apply_move(&game, &best_move);
  char str[128];
  game_str(&game, str);
  printf("Board: %s\n", str);
  ai_last_move = best_move;
  search_done = true;
  search_running = false;
  return NULL;
}

void gui_draw_board(float x0, float y0, float r, float gap) {
  Vector2 mouse = GetMousePosition();
  bool mouse_in_circle = false;
  float dx = 0;
  float dy = 0;
  int index = 0, p = 0, selected_prev = selected;
  for (int i = 0; i <= 16; i++) {
    dx = -(i < 9 ? i : 16 - i) * (r + gap / 2);
    for (int j = 0; j < (i < 9 ? i + 1 : 17 - i); j++) {
      p = player_color == PIECE_RED ? PIECES_MAPPING[index]
                                    : 80 - PIECES_MAPPING[index];
      if (CheckCollisionPointCircle(mouse, (Vector2){x0 + dx, y0 + dy}, r)) {
        mouse_in_circle = true;
      }
      if (CheckCollisionPointCircle(mouse, (Vector2){x0 + dx, y0 + dy}, r) &&
          IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (game.turn == player_color) {
          if (player_color == PIECE_RED && game.board.red >> p & 1) {
            selected = p;
          } else if (player_color == PIECE_GREEN && game.board.green >> p & 1) {
            selected = p;
          }
        }
        if (selected != -1 && selected_moves >> p & 1) {
          struct move_t move = {selected, p};
          game_apply_move(&game, &move);
          selected = -1;
          selected_moves = 0;
          if (!search_running) {
            pthread_t thread_id;
            search_done = false;
            search_running = true;
            pthread_create(&thread_id, NULL, search_ai_move, NULL);
            pthread_detach(thread_id);
          }
        }
      }
      if (game.board.red >> p & 1) {
        if (selected == p && player_color == PIECE_RED) {
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r,
                      ColorBrightness(RED, 0.7));
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r - 3, RED);
        } else {
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r, RED);
        }
      } else if (game.board.green >> p & 1) {
        if (selected == p && player_color == PIECE_GREEN) {
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r,
                      ColorBrightness(GREEN, 0.7));
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r - 3, GREEN);
        } else {
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r, GREEN);
        }
      } else {
        DrawCircleV((Vector2){x0 + dx, y0 + dy}, r, GRAY);
        if (selected != -1 && selected_moves >> p & 1) {
          DrawCircleV((Vector2){x0 + dx, y0 + dy}, r, ColorAlpha(WHITE, 0.6));
        }
      }

      if (ai_last_move.src == p || ai_last_move.dst == p) {
        DrawRectangleV((Vector2){x0 + dx - 3, y0 + dy - 3}, (Vector2){6, 6},
                       ColorAlpha(WHITE, 0.7));
      }

      index++;
      dx += 2 * r + gap;
    }
    dy += (2 * r + gap) * SQRT3 / 2;
  }
  if (selected != selected_prev) {
    selected_moves = 0;
    LIST_HEAD(moves);
    gen_moves(&game.board, MASK_AT(selected), &moves);
    struct list_head *pos, *_n;
    list_for_each_safe(pos, _n, &moves) {
      struct move_t *move = list_entry(pos, struct move_t, list);
      selected_moves |= MASK_AT(move->dst);
      list_del(pos);
      free(move);
    }
  }
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !mouse_in_circle) {
    selected = -1;
    selected_moves = 0;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s [red|green]\n", argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "red") == 0) {
    player_color = PIECE_RED;
  } else if (strcmp(argv[1], "green") == 0) {
    player_color = PIECE_GREEN;
  } else {
    printf("Usage: %s [red|green]\n", argv[0]);
    return 1;
  }
  freopen("/dev/null", "w", stderr);

  init_zobrist();
  init_game(&game);
  clear_killer_moves();

  if (player_color == PIECE_GREEN) {
    pthread_t thread_id;
    search_done = false;
    search_running = true;
    pthread_create(&thread_id, NULL, search_ai_move, NULL);
    pthread_detach(thread_id);
  }

  SetTraceLogLevel(LOG_NONE);
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chinese Checkers");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(BLACK);

    float x0 = SCREEN_WIDTH / 2;
    float y0 = 60;
    gui_draw_board(x0, y0, 15, 4);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
