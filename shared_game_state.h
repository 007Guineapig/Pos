
#pragma once
#include <stddef.h>
#define MAX_PLAYERS 10  
#define PORT 5095    
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH 100

typedef enum {
    EMPTY = 0,  
    SNAKE,      
    FOOD        
} Cell;

typedef enum {
    MSG_GAME_STATE,
    MSG_GAME_OVER,
    MSG_SERVER_FULL,
    MSG_SERVER_SHUTDOWN
} MessageType;

typedef struct {
    MessageType type;
    int score;  // Add this field to store the player's score
    char data[2428];  // Optional: Additional message data
} GameMessage;

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
    int playing; 
    int sendData;
} Player;
typedef struct {
    Grid grid;        
    Snake snake;      
    int foodX;        
    int foodY;        
} GameState;

typedef struct {
    int server_socket;         
    Player players[MAX_PLAYERS]; 
    int num_players;          
    Grid game_grid;             
} Server;


