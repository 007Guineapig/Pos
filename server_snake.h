#include "shared_game_state.h"
#ifndef SERVER_SNAKE_H
#define SERVER_SNAKE_H
#include "server_snake.h"
#include "snake.h"  

int init_server(Server *server, const int port);
void wait_for_clients(Server *server);
void handle_player_input(Server *server, int player_id);
void init_grid(Server * server);
void update_game_state(Server *server);
void send_game_state_to_players(Server *server);
void cleanup_server(Server *server);
void check_food(Server * server);
#endif
