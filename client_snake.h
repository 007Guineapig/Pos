#ifndef CLIENT_SNAKE_H
#define CLIENT_SNAKE_H

#define SERVER_IP "127.0.0.1"  
#define SERVER_PORT 5097     
#include "server_snake.h"
#include "snake.h"
#include "shared_game_state.h"

int init_client(const char *server_ip, const int port);
void handle_user_input(int socket, int *game_chosen);
void cleanup_client(int *socket);
void run_server(Server *server);
int kbhit(void);
void printGrid(const Grid *grid);
void sigchld_handler();
void display_game_list(GameMessage *msg);
void receive_game_state_with_timeout(int socket, GameState *game_state,char represent_snake);

#endif 
