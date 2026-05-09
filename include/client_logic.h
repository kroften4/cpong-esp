#ifndef __CLIENT_LOGIC_H__
#define __CLIENT_LOGIC_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"
#ifdef CONFIG_IDF_TARGET_LINUX
#include <SDL3/SDL.h>
#else
#include "esp_lcd_types.h"
#endif
#include "cpong_logic.h"
#include "cpong_packets.h"
#include "krft/server.h"
#include "rasterizer.h"

struct client_data {
	pthread_mutex_t *state_mtx;
	pthread_mutex_t *sync_mtx;
	pthread_mutex_t *input_mtx;
	struct input *input;
	server_t *server;
	struct pong_state *server_state;
	struct pong_state *local_state;
	int8_t *sync;
};

int server_state_receive(struct client_data *client_data);

void *server_state_receiver(void *client_data);

#ifdef CONFIG_IDF_TARGET_LINUX
void push_frame(struct pong_state state, struct bitmap *stripe_bmp,
				SDL_Renderer *renderer, SDL_Texture *texture);
#else
void push_frame(struct pong_state state, struct bitmap *stripe_bmp,
				esp_lcd_panel_handle_t panel);
#endif

struct update_args {
	bool quit;
#ifdef CONFIG_IDF_TARGET_LINUX
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event *event;
#else
	esp_lcd_panel_handle_t panel;
#endif
	struct client_data client_data;
	struct bitmap *stripe_bmp;
};

bool update(int delta_time, void *update_args);

void update_local_state(struct pong_state *local, int delta_time,
						int input_direction);
void server_snap(struct pong_state *local, struct pong_state server);

bool network_update(int delta_time, void *client_data);

#endif // __CLIENT_LOGIC_H__
