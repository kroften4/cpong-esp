#ifndef _RENDER_H
#define _RENDER_H

#include "cpong_logic.h"
#include "rasterizer.h"

void draw_obj_debug(struct game_obj obj, struct bitmap *bmp,
					struct vec screen_pos);

void draw_obj(struct game_obj obj, struct bitmap *bmp, pixel_t color,
			  struct vec screen_pos);

void draw_score(struct pong_state state, struct bitmap *bmp, pixel_t color,
				struct vec screen_pos);

void draw_server_state(struct pong_state server_state,
					   struct pong_state local_state, struct bitmap *bmp,
					   struct vec screen_pos);

void draw_state(struct pong_state state, struct bitmap *bmp, pixel_t color,
				struct vec screen_pos);

void clear_screen(struct bitmap *bmp);

void draw_frame(struct pong_state local_state, struct bitmap *bmp,
				struct vec screen_pos);

#endif
