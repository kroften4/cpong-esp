#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "ili9486_display_init.h"
#include "sdkconfig.h"
#include <freertos/idf_additions.h>

#include "client_logic.h"
#include "cpong_logic.h"
#include "cpong_packets.h"
#include "esp_check.h"
#include "krft/log.h"
#include "krft/run_every.h"
#include "krft/server.h"
#include "rasterizer.h"

#include "cpong/client_main.h"

static const char *TAG = "cpong client main";

#define LINES_IN_STRIPE CONFIG_ILI9486_V_RES / 16

// #define FPS_CAP 1000
#define MIN_FRAME_DURATION_MS 1000 / CONFIG_CPONG_FPS_CAP

// #define NET_TPS_CAP 60
#define MIN_NET_TICK_DURATION_MS 1000 / CONFIG_CPONG_NET_TPS_CAP

static struct pong_state local_state;
static struct pong_state server_state;
static pthread_mutex_t state_mtx = PTHREAD_MUTEX_INITIALIZER;

static int8_t ssync;
static pthread_mutex_t sync_mtx = PTHREAD_MUTEX_INITIALIZER;

static server_t server;

static struct input input = { 0 };
pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;

static esp_lcd_panel_handle_t panel;

bool onTransDone(esp_lcd_panel_io_handle_t panel_io,
				 esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	TaskHandle_t draw_task = user_ctx;
	xTaskNotifyGive(draw_task);
	return true;
}

int client_main(char *server_ip, char *port)
{
	TaskHandle_t draw_task = xTaskGetCurrentTaskHandle();
	ili9486_display_init(&panel, onTransDone, draw_task);

	// server.fd = connect_to_server(server_ip, port);
	//
	// struct packet init_packet = { 0 };
	// recv_packet(server.fd, &init_packet);
	// if (init_packet.type != PACKET_INIT) {
	// 	ERRORF("did not receive init packet: got type %d", init_packet.type);
	// 	return 1;
	// }

	struct pong_state mock_server_state = { .player_ids = { 0, 0 },
											.score = { 0, 0 } };
	init_game(&mock_server_state);
	struct packet init_packet = {
		.type = PACKET_INIT,
		.sync = 0,
		.size = 0,
		.data.state = mock_server_state,
	};

	ssync = init_packet.sync;
	struct pong_state state = init_packet.data.state;
	LOGF("received init packet: player id %d",
		 state.player_ids[state.own_id_index]);

	local_state = init_packet.data.state;
	local_state.score[0] = 0;
	local_state.score[1] = 0;
	LOG("got init state");
	print_state(local_state);

	// pthread_t server_state_recv_thread;
	// pthread_create(&server_state_recv_thread, NULL, server_state_receiver,
	// 			   &server);
	// pthread_detach(server_state_recv_thread);
	//
	// struct run_every_args net_re_args = { .func = network_update,
	// 									  .args = NULL,
	// 									  .interval_ms =
	// 										  MIN_NET_TICK_DURATION_MS };
	// pthread_t network_thread = start_run_every_thread(&net_re_args);
	// pthread_detach(network_thread);

	struct bitmap stripe_bmp = {};
	stripe_bmp.size_x = CONFIG_ILI9486_H_RES;
	stripe_bmp.size_y = LINES_IN_STRIPE;
	stripe_bmp.buf = heap_caps_malloc(stripe_bmp.size_x * stripe_bmp.size_y *
										  sizeof(stripe_bmp.buf[0]),
									  MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

	int ret = 0;
	ESP_GOTO_ON_FALSE(stripe_bmp.buf != NULL, 1, error, TAG,
					  "Failed to allocate stripe buffer");

	struct update_args update_args = {
		.panel = panel,
		.client_data = {
			.input = &input,
			.input_mtx = &input_mtx,
			.local_state = &local_state,
			.server_state = &server_state,
			.server = server,
			.sync = &ssync,
		},
		.quit = false,
		.stripe_bmp = &stripe_bmp,
	};
	struct run_every_args update_re_args = { .func = update,
											 .args = &update_args,
											 .interval_ms =
												 MIN_FRAME_DURATION_MS };
	run_every(update_re_args);

	return 0;

error:
	return ret;
}
