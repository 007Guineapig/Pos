#include "shared_game_state.h"
#include "server_snake.h"
#ifndef SNAKE_H
#define SNAKE_H


void initializeSnake(Snake *snake, int startX, int startY);
void spawnFood(Grid *grid);
void renderGrid(const Grid *grid);
void moveSnake(Snake *snake, int deltaX, int deltaY);
void updateGrid(Grid *grid, Snake * snakes, int num_players);
int checkCollision(Grid *grid, Snake * snake,int deltaX,int deltaY);
void send_player_input(int socket, char input);

#endif 
