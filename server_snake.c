 #include "server_snake.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include "data_pipeline.h"


#define GAME_STATE_SIZE 2428

void check_food(Server * server){
    
    int width = server->game_grid.width;
    int height =server->game_grid.height ;
    int food = 0;
    int players = server->num_players;
    for (int i = 0; i < server->game_grid.height; i++) {
        for (int j = 0; j < server->game_grid.width; j++) {
            if(server->game_grid.cells[i][j] == FOOD){
                food += 1;
            };
        }
    }
    int total = food -players;
    if(players<food){
        for (int i = 0; i < server->game_grid.height; i++) {
            for (int j = 0; j < server->game_grid.width; j++) {
                if(server->game_grid.cells[i][j] == FOOD){
                    server->game_grid.cells[i][j] = EMPTY;
                    total -=1;
                    if(total == 0){
                    break;
                    }
                };
            }
            if(total == 0){
                break;
            }
        }
    }
}


void init_grid(Server * server){
    server->game_grid.width = GRID_WIDTH;
    server->game_grid.height = GRID_HEIGHT;  
    for (int i = 0; i < server->game_grid.height; i++) {
        for (int j = 0; j < server->game_grid.width; j++) {
            server->game_grid.cells[i][j] = EMPTY;
        }
    }
}

int init_server(Server *server, const int port) {
    struct sockaddr_in server_addr;
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server->server_socket < 0) {
        perror("Error creating server socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        return -1;
    }

    init_grid(server);

    if (listen(server->server_socket, MAX_PLAYERS) < 0) {
        perror("Error listening for connections");
        return -1;
    }
    //ijrwposafkdoeiwujsfdk\lmpijfadskvx
    /*
    if (init_pipeline(&server->pipeline) < 0) {
        fprintf(stderr, "Failed to initialize pipeline\n");
        return -1;
    }
    */
    
    printf("Server is listening on port %d...\n", port);
    server->num_players = 0;
    return 0;
}

void wait_for_clients(Server *server) {
    int game_started = 0;

    while (1) {
        
        int client_socket = accept(server->server_socket, NULL, NULL);
        
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        if (server->num_players < MAX_PLAYERS) {
            int player_index = server->num_players;
            server->players[player_index].socket = client_socket;
            int start_x = GRID_WIDTH / 2 + player_index * 5; 
            int start_y = GRID_HEIGHT / 2;
            initializeSnake(&server->players[player_index].snake, start_x, start_y);
            server->num_players++;
            printf("Player %d connected\n", server->num_players);
            send(client_socket, &server->game_grid, sizeof(server->game_grid), 0);
            spawnFood(&server->game_grid); 
            if (!game_started) {
                game_started = 1;
                printf("Starting the game with Player 1...\n");
                break; 
            }
        } else {
            char full_msg[] = "Server is full. Try again later.";
            send(client_socket, full_msg, sizeof(full_msg), 0);
            close(client_socket);
        }
        
    }
}



void handle_player_input(Server *server, int player_id) {
    char buffer[1]; 
    struct timeval timeout;
    timeout.tv_sec = 0; 
    timeout.tv_usec = 10000;  

fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(server->players[player_id].socket, &read_fds);
int bytes_received=0;
int result = select(server->players[player_id].socket + 1, &read_fds, NULL, NULL, &timeout);
    if (result > 0) {
        bytes_received = recv(server->players[player_id].socket, buffer, sizeof(buffer), 0);
    }
    if (bytes_received > 0) {
        char input = buffer[0]; 

        Snake *snake = &server->players[player_id].snake; 

         switch (input) {
                case 'w': 
                    if (snake->direction != 's') {  
                        snake->direction = 'w';
                    }
                    break;
                case 's': 
                    if (snake->direction != 'w') { 
                        snake->direction = 's';
                    }
                    break;
                case 'a': 
                    if (snake->direction != 'd') {  
                        snake->direction = 'a';
                    }
                    break;
                case 'd': 
                    if (snake->direction != 'a') {  
                        snake->direction = 'd';
                    }
                    break;

                case 'k': 
                snake->direction = 'k';
                
                        char exit_msg[] = "Server is shutting down. Goodbye!";
                        close(server->players[player_id].socket);

                        printf("Closed connection for player %d\n", player_id + 1);
                        for (int i = player_id; i < server->num_players - 1; i++) {
                           server->players[i] = server->players[i + 1]; 
                        }
                        server->num_players--;  
                        if (server->num_players == 0) {
                        printf("No players left in the game. Resetting game state.\n");
                }
                    break;
                    case 'p': 
                    
                        snake->direction = 'p';
                    
                    break;
                default:
                //nic
                    break;
            
         }
    }
    
  }



void update_game_state(Server *server) {

  

    Snake snakes[MAX_PLAYERS]; 
    int numSnakes = 0;        
    printf("number: %d\n",server->num_players);
    for (int i = 0; i < server->num_players; i++) {
        Snake *snake = &server->players[i].snake;
        int deltaX = 0, deltaY = 0;

        switch (snake->direction) {
            case 'w': deltaX = 0; deltaY = -1; break;
            case 's': deltaX = 0; deltaY = 1; break;
            case 'a': deltaX = -1; deltaY = 0; break;
            case 'd': deltaX = 1; deltaY = 0; break;
            case 'k': deltaX = 1; deltaY = 0; break;
            case 'p': deltaX = 0; deltaY = 0; break;
        }
        
        moveSnake(snake, deltaX, deltaY);
         printf("kontroluje %d hraca \n", i+1);
        if (checkCollision(&server->game_grid, snake, deltaX, deltaY)) {
            printf("Player %d hit an obstacle. Game over!\n", i + 1);
        }
        snakes[numSnakes++] = *snake;
    }

    updateGrid(&server->game_grid, snakes, numSnakes);
}


void send_game_state_to_players(Server *server) {
    printf("Sending updated game state to players...\n");
    for (int i = 0; i < server->num_players; i++) {
        int bytes_sent = send(server->players[i].socket, &server->game_grid, sizeof(server->game_grid), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send game state to player");
        } else {
            printf("Sent %d bytes to player %d\n", bytes_sent, i + 1);
        }
    }
    /*
 for (int i = 0; i < server->num_players; i++){
     printf("asjodlnfpijosaldksdoipjfdspja %c\n",server->players[i].snake.direction );
    if(server->players[i].snake.direction == 'k'){
    char exit_msg[] = "Server is shutting down. Goodbye!";
                        printf("Closed connection for player %d\n", i + 1);
                        for (int j = i; j < server->num_players - 1; j++) {
                           server->players[j] = server->players[j+ 1]; 
                        }
                        close(server->players[i].socket);
                        server->num_players--;  
                        if (server->num_players == 0) {
                        printf("No players left in the game. Resetting game state.\n");
                        }
    }
    }*/

}

/*
void send_game_state_to_players(Server *server) {
    char buffer[GAME_STATE_SIZE]; // Allocate buffer for serialized data
    DataPipeline pipeline;
    init_pipeline(&pipeline);

    for (int i = 0; i < server->num_players; i++) {
        // Serialize game state into a buffer
//        if (pipeline.serialize(&server->game_grid, buffer, sizeof(buffer)) < 0) {
     if (pipeline.serialize((GameState *)&server->game_grid, buffer, sizeof(buffer)) < 0) {

            printf("Failed to serialize game state for player %d\n", i + 1);
            continue;
        }

        int bytes_sent = send(server->players[i].socket, buffer, sizeof(GameState), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send game state to player");
        } else {
            printf("Sent game state to player %d\n", i + 1);
        }
    }
}*/

void cleanup_server(Server *server) {
    for (int i = 0; i < server->num_players; i++) {
        char shutdown_msg[] = "Server is shutting down. Goodbye!";
        int bytes_sent = send(server->players[i].socket, shutdown_msg, sizeof(shutdown_msg), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send shutdown message to player");
        } else {
            printf("Shutdown message sent to player %d\n", i + 1);
        }
    }

    for (int i = 0; i < server->num_players; i++) {
        close(server->players[i].socket);
        printf("Closed connection for player %d\n", i + 1);
    }
    close(server->server_socket);
    printf("Server socket closed and server shut down.\n");
}
