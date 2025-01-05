
#pragma once
#include <stddef.h>
#include <sys/time.h>
#define MAX_PLAYERS 100  
#define MAX_PLAYERS_PER_GAME 10  
#define MAX_GAMES 10  
#define PORT 5095    
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH 100

extern volatile int server_running;

typedef enum {
    EMPTY = 0,  
    SNAKE,      
    FOOD        
} Cell;

typedef struct {
    int game_id;
    time_t last_player_left_time;
    int timer_started;
} GameTimer;

typedef enum {
    MSG_GAME_STATE,
    MSG_GAME_OVER,
    MSG_SERVER_FULL,
    MSG_SERVER_SHUTDOWN,
    MSG_CREATE_GAME,
    MSG_GAME_LIST  
} MessageType;

typedef struct {
    int game_id;
    int num_players;
    int max_players;
} GameInfo;  

typedef struct {
    MessageType type;
    int score; 
    char data[2428];  
    GameInfo games[MAX_GAMES];
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
    int game_id;               
    Grid game_grid;            
    Player players[MAX_PLAYERS_PER_GAME];
    int num_players;            
    int max_players;  
    int is_active;              
} Game;
typedef struct {
    Grid grid;        
    Snake snake;      
    int foodX;        
    int foodY;        
} GameState;


typedef struct {
    int server_socket;         
    Game games[MAX_GAMES];   
    int num_games;              
} Server;

