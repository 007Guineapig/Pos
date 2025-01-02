
#pragma once
#include <stddef.h>
#define MAX_PLAYERS 4  
#define PORT 5095    
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH 100

typedef enum {
    EMPTY = 0,  
    SNAKE,      
    FOOD        
} Cell;
typedef struct {
    int width;                
    int height;               
    Cell cells[GRID_HEIGHT][GRID_WIDTH]; 
} Grid;
typedef struct {
    int x[MAX_SNAKE_LENGTH]; 
    int y[MAX_SNAKE_LENGTH];  
    int length;   
    char direction;
    int score;
} Snake;

typedef struct {
    int socket; 
    Snake snake; 
} Player;
typedef struct {
    Grid grid;        
    Snake snake;      
    int foodX;        
    int foodY;        
} GameState;

typedef struct {
    int (*preprocess)(GameState *state); // Optional: Pre-send processing
    int (*postprocess)(GameState *state); // Optional: Post-receive processing
    int (*serialize)(GameState *state, char *buffer, size_t buffer_size); // Serialization
    int (*deserialize)(char *buffer, size_t buffer_size, GameState *state); // Deserialization
} DataPipeline;

typedef struct {
    int server_socket;         
    Player players[MAX_PLAYERS]; 
    int num_players;
    DataPipeline pipeline;            
    Grid game_grid;             
} Server;


