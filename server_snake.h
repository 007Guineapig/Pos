#include "shared_game_state.h"
#ifndef SERVER_SNAKE_H
#define SERVER_SNAKE_H
#include "server_snake.h"
#include "snake.h"  
extern time_t last_player_activity;


int init_server(Server *server, const int port);
void wait_for_clients(Server *server);
void cleanup_server(Server *server);
void send_game_list(int client_socket, Server *server);
void print_active_games(Server *server);
int create_new_game(Server *server, int max_players);
int add_player_to_game(Server *server, int game_id, int player_socket);
void update_game_state(Server *server, int game_id);
void send_game_state_to_players(Server *server, int game_id);
void handle_player_input(Server *server, int game_id, int player_id);
void init_grid(Grid *grid);
void check_food(Game *game);
void close_game(Server *server, int game_id);
#endif
