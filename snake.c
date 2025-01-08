#include "snake.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h> 

void send_player_input(int socket, char input) {
    send(socket, &input, sizeof(input), 0);
}

void initializeSnake(Snake *snake, int startX, int startY) {
    snake->length = 1;  
    snake->x[0] = startX;
    snake->y[0] = startY;
    snake->direction = 'p';
    snake->score = 0;
}

void spawnFood(Grid *grid) {
    srand(time(NULL));
    if(grid->width <= 0 || grid->height <= 0) {
        printf("Error: w %d ,h %d \n",grid->width,grid->height);
        fprintf(stderr, "Error: Invalid grid dimensions\n");
        exit(EXIT_FAILURE);
    }

    int food_x = rand() % grid->width;
    int food_y = rand() % grid->height;

    while(grid->cells[food_y][food_x] != EMPTY) {
        food_x = rand() % grid->width;
        food_y = rand() % grid->height;
    }

    grid->cells[food_y][food_x] = FOOD; 
}

void renderGrid(const Grid *grid, Snake *snake,char represent_snake) {
    system("clear");  
    printf("Game Grid:\n");
    for (int i = 0; i < grid->height; i++) {
        for (int j = 0; j < grid->width; j++) {
            int isSnakeCell = 0;
            for (int k = 0; k < snake->length; k++) {
                if (snake->x[k] == j && snake->y[k] == i) {
                    isSnakeCell = 1;
                    break; 
                }
            }

            if (isSnakeCell) {
                printf("%c ", represent_snake); 
            } else if (grid->cells[i][j] == FOOD) {
                printf("F "); 
            } else if (grid->cells[i][j] == SNAKE) {
                printf("S "); 
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
}

void moveSnake(Snake *snake, int deltaX, int deltaY) {
    if(deltaX ==0 && deltaY ==0){
    } else{
        for (int i = snake->length; i > 0; i--) {
        snake->x[i] = snake->x[i - 1];
        snake->y[i] = snake->y[i - 1];
        }

        snake->x[0] += deltaX;
        snake->y[0] += deltaY;
    }
    
}

void updateGrid(Grid *grid, Snake *snakes, int num_snakes) {
    for(int i = 0; i < grid->height; i++) {
        for(int j = 0; j < grid->width; j++) {
            if (grid->cells[i][j] != FOOD) {
                grid->cells[i][j] = EMPTY;
            }
        }
    }

    for(int s = 0; s < num_snakes; s++) {
        Snake *snake = &snakes[s];
        for(int i = 0; i < snake->length; i++) {
            grid->cells[snake->y[i]][snake->x[i]] = SNAKE;
        }
    }
}




int checkCollision(Grid *grid, Snake * snake) {

    if(snake->x[0] < 0 || snake->x[0] >= grid->width || snake->y[0] < 0 || snake->y[0] >= grid->height) {

        return 1; 
    }

    if(grid->cells[snake->y[0]][snake->x[0]] == SNAKE && snake->direction != 'p') {
        return 1; 
    }
    
    if(grid->cells[snake->y[0]][snake->x[0]] == FOOD){
        snake->length++;
        snake->score += 10;
        spawnFood(grid);
        
    }
    return 0; 
}
