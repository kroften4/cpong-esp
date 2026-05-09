#include "esp_log.h"
#include "rasterizer.h"

#include "cpong_logic.h"
#include "krft/log.h"
#include "render.h"
#include "vec.h"

static const char *TAG = "render.c";

void draw_obj_debug(struct game_obj obj, struct bitmap *bmp,
					struct vec screen_pos)
{
	WARN("not yet implemented");
	// SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	// SDL_RenderLine(renderer, obj.pos.x, obj.pos.y,
	// 			   obj.pos.x + obj.velocity.x * 100,
	// 			   obj.pos.y + obj.velocity.y * 100);
}

void draw_obj(struct game_obj obj, struct bitmap *bmp, pixel_t color,
			  struct vec screen_pos)
{
	struct vec top_left = { obj.pos.x - obj.size.x / 2.0f,
							obj.pos.y + obj.size.y / 2.0f };
	top_left = world_to_screen_coords(screen_pos, top_left);
	rast_fillrect(bmp, top_left.x, top_left.y, obj.size.x, obj.size.y, color);
#ifdef DEBUG
	draw_obj_debug(obj, &bmp);
#endif
}

void draw_score(struct pong_state state, struct bitmap *bmp, pixel_t color,
				struct vec screen_pos)
{
	int score_diff = state.score[1] - state.score[0];
	struct vec pos = { state.box_size.x / 2, 0 };
	rast_fillrect(bmp, pos.x, pos.y, score_diff * 10, state.box_size.y, color);
}

void draw_server_state(struct pong_state server_state,
					   struct pong_state local_state, struct bitmap *bmp,
					   struct vec screen_pos)
{
	// Adjust X position and sizes, which are only stored in local
	// state
	server_state.player1.pos.x = local_state.player1.pos.x;
	server_state.player1.size = local_state.player1.size;
	server_state.player2.pos.x = local_state.player2.pos.x;
	server_state.player2.size = local_state.player2.size;
	server_state.ball.size = local_state.ball.size;
	draw_state(server_state, bmp, RGB565(255, 255, 255), screen_pos);
}

void draw_state(struct pong_state state, struct bitmap *bmp, pixel_t color,
				struct vec screen_pos)
{
	draw_obj(state.player1, bmp, color, screen_pos);
	ESP_LOGD(TAG, "drew player1");
	draw_obj(state.player2, bmp, color, screen_pos);
	ESP_LOGD(TAG, "drew player2");
	draw_obj(state.ball, bmp, color, screen_pos);
	ESP_LOGD(TAG, "drew ball");
	draw_score(state, bmp, RGB565(0, 0, 255), screen_pos);
	ESP_LOGD(TAG, "drew score");
}

void clear_screen(struct bitmap *bmp)
{
	rast_fillrect(bmp, 0, 0, bmp->size_x, bmp->size_y, RGB565(255, 255, 255));
}

void draw_frame(struct pong_state local_state, struct bitmap *bmp,
				struct vec screen_pos)
{
	clear_screen(bmp);
#ifdef DEBUG
	// TODO: i dont wanna pass server state idk
	// draw_server_state(server_state, bmp);
#endif
	draw_state(local_state, bmp, RGB565(0, 0, 0), screen_pos);
}
