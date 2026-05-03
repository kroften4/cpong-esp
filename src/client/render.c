#include "rasterizer.h"

#include "cpong_logic.h"
#include "krft/log.h"
#include "render.h"

void draw_obj_debug(struct game_obj obj, struct bitmap *bmp)
{
	WARN("not yet implemented");
	// SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	// SDL_RenderLine(renderer, obj.pos.x, obj.pos.y,
	// 			   obj.pos.x + obj.velocity.x * 100,
	// 			   obj.pos.y + obj.velocity.y * 100);
}

void draw_obj(struct game_obj obj, struct bitmap *bmp, pixel_t color)
{
	rast_fillrect(bmp, obj.pos.x - obj.size.x / 2.0f,
				  obj.pos.y - obj.size.y / 2.0f, obj.size.x, obj.size.y, color);
#ifdef DEBUG
	draw_obj_debug(obj, &bmp);
#endif
}

void draw_score(struct pong_state state, struct bitmap *bmp, pixel_t color)
{
	int score_diff = state.score[1] - state.score[0];
	rast_fillrect(bmp, state.box_size.x / 2, 0, score_diff * 10,
				  state.box_size.y, color);
}

void draw_server_state(struct pong_state server_state,
					   struct pong_state local_state, struct bitmap *bmp)
{
	// Adjust X position and sizes, which are only stored in local
	// state
	server_state.player1.pos.x = local_state.player1.pos.x;
	server_state.player1.size = local_state.player1.size;
	server_state.player2.pos.x = local_state.player2.pos.x;
	server_state.player2.size = local_state.player2.size;
	server_state.ball.size = local_state.ball.size;
	draw_state(server_state, bmp, RGB565(255, 255, 255));
}

void draw_state(struct pong_state state, struct bitmap *bmp, pixel_t color)
{
	draw_obj(state.player1, bmp, color);
	draw_obj(state.player2, bmp, color);
	draw_obj(state.ball, bmp, color);
	draw_score(state, bmp, RGB565(0, 0, 255));
}

void clear_screen(struct bitmap *bmp)
{
	rast_fillrect(bmp, 0, 0, bmp->size_x, bmp->size_y, RGB565(255, 255, 255));
}

void draw_frame(struct pong_state local_state, struct bitmap *bmp)
{
	clear_screen(bmp);
#ifdef DEBUG
	// TODO: i dont wanna pass server state idk
	// draw_server_state(server_state, bmp);
#endif
	draw_state(local_state, bmp, RGB565(0, 0, 0));
}
