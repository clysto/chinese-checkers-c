#include <pthread.h>
#include <raylib.h>
#include <rlgl.h>
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
#define MIN_SCREEN_WIDTH 300
#define MIN_SCREEN_HEIGHT 450
#define MIN_SCALE 0.35f
#define SQRT3 (1.7320508075688772)

struct game_t game;
int selected = -1;
enum color_t player_color;
uint128_t selected_moves = 0;
atomic_bool search_done = false;
atomic_bool search_running = false;
struct move_t ai_last_move = {-1, -1};
struct move_t player_last_move = {-1, -1};
atomic_bool game_over = false;

static inline void rotate60(float *x, float *y) {
  float x0 = *x;
  float y0 = *y;
  *x = x0 * 0.5 - y0 * SQRT3 / 2;
  *y = x0 * SQRT3 / 2 + y0 * 0.5;
}

#define foreach_circle(x, y, radius, gap, p)                                   \
  for (p = 0; x = (p % 9) * (2 * radius + gap) + (p / 9) * (radius + gap / 2), \
      y = (p / 9) * (2 * radius + gap) * SQRT3 / 2, rotate60(&x, &y),          \
      y -= 8 * (2 * radius + gap) * SQRT3 / 2, p < 81;                         \
       p++)

void draw_board_background(Vector2 *points, float r) {
  Vector2 points2[9];
  Vector2 center;
  center.x = (points[0].x + points[2].x) / 2;
  center.y = (points[0].y + points[2].y) / 2;

  for (int i = 0; i < 4; i++) {
    DrawCircleV(points[i], r, GetColor(0x4b3110ff));
  }

  points2[0].x = points[0].x - r * SQRT3 / 2;
  points2[0].y = points[0].y - r / 2;
  points2[1].x = points[0].x + r * SQRT3 / 2;
  points2[1].y = points[0].y - r / 2;

  points2[2].x = points[1].x + r * SQRT3 / 2;
  points2[2].y = points[1].y - r / 2;
  points2[3].x = points[1].x + r * SQRT3 / 2;
  points2[3].y = points[1].y + r / 2;

  points2[4].x = points[2].x + r * SQRT3 / 2;
  points2[4].y = points[2].y + r / 2;
  points2[5].x = points[2].x - r * SQRT3 / 2;
  points2[5].y = points[2].y + r / 2;

  points2[6].x = points[3].x - r * SQRT3 / 2;
  points2[6].y = points[3].y + r / 2;
  points2[7].x = points[3].x - r * SQRT3 / 2;
  points2[7].y = points[3].y - r / 2;

  points2[8] = points2[0];

  rlBegin(RL_TRIANGLES);
  rlColor4ub(0x4b, 0x31, 0x10, 0xff);
  for (int i = 0; i < 8; i++) {
    rlVertex2f(center.x, center.y);
    rlVertex2f(points2[i + 1].x, points2[i + 1].y);
    rlVertex2f(points2[i].x, points2[i].y);
  }
  rlEnd();
}

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
      printf("Depth: %2d, Eval: %6d, Move: %02d->%02d\n", d, eval,
             _best_move.src, _best_move.dst);
      if (eval == SCORE_WIN) {
        best_move = _best_move;
        break;
      }
      best_move = _best_move;
    }
  }
  printf("AI move: %02d->%02d\n", best_move.src, best_move.dst);
  game_apply_move(&game, &best_move);
  char str[128];
  game_str(&game, str);
  printf("Board: %s\n", str);
  ai_last_move = best_move;
  search_done = true;
  search_running = false;
  game_over = is_game_over(&game);
  return NULL;
}

void handle_click(int p) {
  if (game.turn != player_color || game_over) {
    return;
  }
  if (player_color == PIECE_RED ? game.board.red >> p & 1
                                : game.board.green >> p & 1 && selected != p) {
    selected = p;
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
  } else if (selected_moves >> p & 1) {
    struct move_t move = {selected, p};
    player_last_move = move;
    game_apply_move(&game, &move);
    selected = -1;
    selected_moves = 0;
    game_over = is_game_over(&game);
    if (!search_running) {
      search_done = false;
      search_running = true;
      pthread_t thread_id;
      pthread_create(&thread_id, NULL, search_ai_move, NULL);
      pthread_detach(thread_id);
    }
  } else {
    selected = -1;
    selected_moves = 0;
  }
}

void gui_draw_circle(float x, float y, float radius, int p) {
  float ring_thickness = radius * (3.5f / 15.0f);
  if (ring_thickness < 1.5f) {
    ring_thickness = 1.5f;
  }
  if (ring_thickness > radius - 1.0f) {
    ring_thickness = radius - 1.0f;
  }

  if (game.board.red >> p & 1) {
    if (p == selected) {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0xffafafff));
      DrawCircleV((Vector2){x, y}, radius - ring_thickness, GetColor(0xe50000ff));
    } else {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0xe50000ff));
    }
  } else if (game.board.green >> p & 1) {
    if (p == selected) {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0xcfffcfff));
      DrawCircleV((Vector2){x, y}, radius - ring_thickness, GetColor(0x35cc35ff));
    } else {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0x35cc35ff));
    }
  } else {
    if (selected_moves >> p & 1) {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0xd6b17dff));
    } else {
      DrawCircleV((Vector2){x, y}, radius, GetColor(0x977d5aff));
    }
  }
}

void gui_draw_board(float x0, float y0, float r, float gap) {
  Vector2 mouse = GetMousePosition();
  bool click_circle = false;
  float marker_size = r * (6.0f / 15.0f);
  if (marker_size < 2.0f) {
    marker_size = 2.0f;
  }
  float marker_half = marker_size * 0.5f;
  float dx = 0, dy = 0;
  int p, _p;
  Vector2 points[5];
  int points_len = 0;
  Vector2 center;

  int key_code = GetKeyPressed();

  if (key_code == KEY_R && game.turn == player_color && !game_over &&
      player_last_move.src != -1) {
    game_undo_move(&game, &ai_last_move);
    game_undo_move(&game, &player_last_move);
    ai_last_move = (struct move_t){-1, -1};
  }

  foreach_circle(dx, dy, r, gap, p) {
    _p = player_color == PIECE_RED ? p : 80 - p;
    if (CheckCollisionPointCircle(mouse, (Vector2){x0 + dx, y0 + dy}, r) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      handle_click(_p);
      click_circle = true;
    }
    if (_p == 0 || _p == 8 || _p == 72 || _p == 80) {
      points[points_len].x = x0 + dx;
      points[points_len].y = y0 + dy;
      points_len += 1;
    }
    if (_p == 40) {
      center.x = x0 + dx;
      center.y = y0 + dy;
    }
  }

  points[4] = points[2];
  points[2] = points[3];
  points[3] = points[4];

  draw_board_background(points, 1.8 * r);

  foreach_circle(dx, dy, r, gap, p) {
    _p = player_color == PIECE_RED ? p : 80 - p;
    gui_draw_circle(x0 + dx, y0 + dy, r, _p);

    if (ai_last_move.src == _p || ai_last_move.dst == _p) {
      DrawRectangleV((Vector2){x0 + dx - marker_half, y0 + dy - marker_half},
                     (Vector2){marker_size, marker_size},
                     ColorAlpha(WHITE, 0.7));
    }
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !click_circle) {
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

  if (player_color == PIECE_GREEN) {
    pthread_t thread_id;
    search_done = false;
    search_running = true;
    pthread_create(&thread_id, NULL, search_ai_move, NULL);
    pthread_detach(thread_id);
  }

  SetTraceLogLevel(LOG_NONE);
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chinese Checkers");
  SetWindowMinSize(MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(GetColor(0xe8b061ff));

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float sx = (float)w / (float)SCREEN_WIDTH;
    float sy = (float)h / (float)SCREEN_HEIGHT;
    float scale = sx < sy ? sx : sy;
    if (scale < MIN_SCALE) {
      scale = MIN_SCALE;
    }

    float x0 = (float)w * 0.5f;
    float y0 = (float)h * 0.5f;
    gui_draw_board(x0, y0, 15.0f * scale, 5.0f * scale);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
