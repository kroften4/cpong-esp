#include "esp_log.h"
#include "sdkconfig.h"
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_types.h>
#include <freertos/idf_additions.h>
#include <pthread.h>

#include "client_logic.h"
#include "cpong_logic.h"
#include "krft/log.h"
#include "rasterizer.h"
#include "render.h"

static const char *TAG = "client_logic";

#define MIN_NET_TICK_DURATION_MS 1000 / CONFIG_CPONG_NET_TPS_CAP

void update_local_state(struct pong_state *local, int delta_time,
						int input_direction)
{
	// TODO: interpolate opp movement
	// TODO: come up with a less stupid way to differentiate between me and opp

	struct game_obj player1_upd = local->player1;
	struct game_obj player2_upd = local->player2;
	if (0 == local->own_id_index) {
		player1_upd.velocity.y = input_direction * 0.5;
		player1_upd = linear_move(player1_upd, delta_time);
		player2_upd = linear_move(player2_upd, delta_time);
	} else {
		player2_upd.velocity.y = input_direction * 0.5;
		player2_upd = linear_move(player2_upd, delta_time);
		player1_upd = linear_move(player1_upd, delta_time);
	}

	struct wall wall = { .up = local->box_size.y,
						 .down = 0,
						 .left = 0,
						 .right = local->box_size.x };
	struct game_obj ball_upd = { 0 };
	int scored_index;
	ball_advance(wall, local->player1, local->player2, local->ball, &ball_upd,
				 delta_time, &scored_index);

	local->ball = ball_upd;
	local->player1 = player1_upd;
	local->player2 = player2_upd;
}

void server_snap(struct pong_state *local, struct pong_state server)
{
	local->ball.pos = server.ball.pos;
	local->ball.velocity = server.ball.velocity;

	if (0 != local->own_id_index) {
		local->player1.pos.y = server.player1.pos.y;
		local->player1.velocity.y = server.player1.velocity.y;
	} else {
		local->player2.pos.y = server.player2.pos.y;
		local->player2.velocity.y = server.player2.velocity.y;
	}
}

int server_state_receive(struct client_data *args)
{
	struct packet packet = { 0 };
	int res = recv_packet(args->server.fd, &packet);
	if (res <= 0) {
		LOG("Couldn't recv from the server");
		return -1;
	}

	switch (packet.type) {
	case PACKET_STATE:
		pthread_mutex_lock(args->state_mtx);
		*args->server_state = packet.data.state;
		// LOG("received server state");
		// print_state(server_state);
		// LOG("local state");
		server_snap(args->local_state, *args->server_state);
		// print_state(local_state);
		pthread_mutex_unlock(args->state_mtx);
		break;
	case PACKET_INIT:
		pthread_mutex_lock(args->state_mtx);
		int own_index_save = args->local_state->own_id_index;
		*args->local_state = packet.data.state;
		args->local_state->own_id_index = own_index_save;
		pthread_mutex_unlock(args->state_mtx);

		pthread_mutex_lock(args->sync_mtx);
		LOGF("sync change: %d -> %d", *args->sync, packet.sync);
		*args->sync = packet.sync;
		pthread_mutex_unlock(args->sync_mtx);

		pthread_mutex_lock(args->input_mtx);
		args->input->input_acc_ms = 0;
		pthread_mutex_unlock(args->input_mtx);
		break;
	default:
		WARNF("Unexpected packet: %d", packet.type);
		return -1;
	}
	return 0;
}

void *server_state_receiver(void *client_data)
{
	struct client_data *args = client_data;
	for (;;) {
		server_state_receive(args);
	}
	return NULL;
}

bool network_update(int delta_time, void *client_data)
{
	struct client_data *args = client_data;
	if (delta_time > 3 + MIN_NET_TICK_DURATION_MS) {
		WARNF("Network updates running slow: %d ms behind",
			  delta_time - MIN_NET_TICK_DURATION_MS);
	}

	pthread_mutex_lock(args->input_mtx);
	pthread_mutex_lock(args->sync_mtx);
	struct packet packet = { .type = PACKET_INPUT,
							 .sync = *args->sync,
							 .data.input = { .input_acc_ms =
												 args->input->input_acc_ms } };
	pthread_mutex_unlock(args->sync_mtx);
	args->input->input_acc_ms = 0;
	pthread_mutex_unlock(args->input_mtx);
	int b_sent = client_send(args->server, packet);
	if (b_sent <= 0)
		LOGF("sent %d, input %d", b_sent, (int)packet.data.input.input_acc_ms);
	return true;
}

int receive_paddle_inputs(struct input *input, pthread_mutex_t *input_mtx,
						  const int delta_time)
{
	// TODO: add button reads
	bool up = true;
	bool down = true;

	int input_direction = (down - up);

	pthread_mutex_lock(input_mtx);
	input->input_acc_ms += input_direction * delta_time;
	pthread_mutex_unlock(input_mtx);
	return input_direction;
}

void push_frame(struct pong_state state, struct bitmap *bmp,
				esp_lcd_panel_handle_t panel)
{
	size_t stripe_y = 0;
	while (stripe_y < state.box_size.y) {
		draw_frame(state, bmp);
		ESP_LOGI(TAG, "Pushing stripe %d;%d - %d;%d", 0, stripe_y,
				 bmp->size_x, stripe_y + bmp->size_y);
		ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
			panel, 0, stripe_y, bmp->size_x, stripe_y + bmp->size_y, bmp->buf));
		// TODO: add circular buffer
		ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
		stripe_y += bmp->size_y;
	}
}

bool update(int delta_time, void *update_args_p)
{
	struct update_args *update_args = update_args_p;
	if (update_args->quit) {
		return false;
	}
	struct client_data args = update_args->client_data;

	int paddle_direction =
		receive_paddle_inputs(args.input, args.input_mtx, delta_time);

	pthread_mutex_lock(args.state_mtx);
	struct pong_state *local_state = args.local_state;
	pthread_mutex_unlock(args.state_mtx);

	update_local_state(local_state, delta_time, paddle_direction);

	esp_lcd_panel_handle_t panel = update_args->panel;
	struct bitmap *bmp = update_args->stripe_bmp;

	push_frame(*local_state, bmp, panel);

	return true;
}
