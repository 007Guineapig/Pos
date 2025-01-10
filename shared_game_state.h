#pragma once
#include <stddef.h>
#include <sys/time.h>
#include <pthread.h>  
#include <stdatomic.h>

#define MAX_PLAYERS 100  
#define MAX_PLAYERS_PER_GAME 10  
#define MAX_GAMES 10  
#define PORT 5095    
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH 100

extern _Atomic int server_running;

//represent tile in grid
typedef enum {
    EMPTY = 0,  
    SNAKE,      
    FOOD        
} Cell;

//timer for each game
typedef struct {
    int game_id;
    time_t last_player_left_time;
    int timer_started;
} GameTimer;

//type of message that is sent from server to clients
typedef enum {
    MSG_GAME_STATE,
    MSG_GAME_OVER,
    MSG_SERVER_FULL,
    MSG_SERVER_SHUTDOWN,
    MSG_CREATE_GAME,
    MSG_GAME_LIST  
} MessageType;

//basic info of the game
typedef struct {
    int game_id;
    int num_players;
    int max_players;
} GameInfo;  

//snakes informations
typedef struct {
    int x[MAX_SNAKE_LENGTH]; 
    int y[MAX_SNAKE_LENGTH];  
    int length;   
    char direction;
    int score;
} Snake;

//message that is sent from server to clients
typedef struct {
    MessageType type;
    int score;
    Snake snake;
    char data[2428];  
    GameInfo games[MAX_GAMES];
} GameMessage;

//grid infos
typedef struct {
    int width;                
    int height;               
    Cell cells[GRID_HEIGHT][GRID_WIDTH]; 
} Grid;

//players infos
typedef struct {
    int socket; 
    Snake snake;
    int playing; 
    int sendData;
} Player;

//infos of the game
typedef struct {
    int game_id;               
    Grid game_grid;            
    Player players[MAX_PLAYERS_PER_GAME];
    int num_players;            
    int max_players;  
    int is_active;              
    pthread_t thread;          
    int thread_active;        
    struct Server* server;
} Game;

//state of the game
typedef struct {
    Grid grid;        
    Snake snake;    
    //forgot to delete  
    int foodX;        
    int foodY;        
} GameState;

//server info
typedef struct Server {
    int server_socket;         
    Game games[MAX_GAMES];   
    int num_games;              
} Server;

//not needed forgot to delete
typedef struct {
    int socket;
    int *game_chosen;
    GameState *game_state;
    int *gameover;
} ThreadData;