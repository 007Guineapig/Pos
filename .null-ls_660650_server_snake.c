#include "server_snake.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



int init_server(Server *server, const int port) {
    struct sockaddr_in server_addr;
    // Create the server socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        perror("Error creating server socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the server socket
    if (bind(server->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        return -1;
    }
  server->game_grid.width = GRID_WIDTH;
    server->game_grid.height = GRID_HEIGHT;  
for (int i = 0; i < server->game_grid.height; i++) {
        for (int j = 0; j < server->game_grid.width; j++) {
            server->game_grid.cells[i][j] = EMPTY;
        }
    }
    if (listen(server->server_socket, MAX_PLAYERS) < 0) {
        perror("Error listening for connections");
        return -1;
    }
    printf("Server is listening on port %d...\n", port);
    server->num_players = 0;
    return 0;
}

void wait_for_clients(Server *server) {
    while (server->num_players < 1) {
        int client_socket = accept(server->server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }
        server->players[server->num_players].socket = client_socket;
        initializeSnake(&server->players[server->num_players].snake, GRID_WIDTH / 2, GRID_HEIGHT / 2);
        spawnFood(&server->game_grid);  // Spawn food for this game session
        server->num_players++;
        printf("Player %d connected\n", server->num_players);
    }
}

/*
void handle_player_input1(Server *server) {
    char buffer[1];
    int bytes_received = recv(server->player.socket, buffer, sizeof(buffer), 0);

    if (bytes_received > 0) {
        char input = buffer[0];
        int deltaX = 0, deltaY = 0;

        switch (input) {
            case 'w': deltaX = 0; deltaY = -1; break;
            case 's': deltaX = 0; deltaY = 1; break;
            case 'a': deltaX = -1; deltaY = 0; break;
            case 'd': deltaX = 1; deltaY = 0; break;
        }

        // Move snake and update grid
        moveSnake(&server->snake, deltaX, deltaY);
        updateGrid(&server->game_grid, &server->snake);
    }
}
*/
//toto
void handle_player_input(Server *server, int player_id) {
    char buffer[1];  // To hold the input received from the client
    int bytes_received = recv(server->players[player_id].socket, buffer, sizeof(buffer), 0);

    // Check if data is received successfully
    if (bytes_received > 0) {
        char input = buffer[0];  // Get the player's input (one character)

        Snake *snake = &server->players[player_id].snake;  // Get the snake of the player

        // Declare deltaX and deltaY for movement direction
        int deltaX = 0;
        int deltaY = 0;

        // Handle player input for snake movement
        if (input == 'w' && deltaY == 0) {  // Up
            deltaX = 0;
            deltaY = -1;
        }
        else if (input == 's' && deltaY == 0) {  // Down
            deltaX = 0;
            deltaY = 1;
        }
        else if (input == 'a' && deltaX == 0) {  // Left
            deltaX = -1;
            deltaY = 0;
        }
        else if (input == 'd' && deltaX == 0) {  // Right
            deltaX = 1;
            deltaY = 0;
        }

        if (deltaX != 0 || deltaY != 0) {
          //  moveSnake(snake, deltaX, deltaY);
        printf("Moved snake: deltaX = %d, deltaY = %d\n", deltaX, deltaY);
        }
    }
   update_game_state(server);
    send_game_state_to_players(server);
}



void update_game_state(Server *server) {
    for (int i = 0; i < server->num_players; i++) {
        Snake *snake = &server->players[i].snake;
        // The deltaX and deltaY should be updated by handle_player_input() function
        updateGrid(&server->game_grid, snake);

        // Check for collisions
        if (checkCollision(&server->game_grid, snake->x[0], snake->y[0])) {

            // Handle game over logic
            printf("Player %d hit an obstacle or self. Game Over!\n", i + 1);
        }
    }
}
/*
void send_game_state(Server *server) {
    send(server->player.socket, &server->game_grid, sizeof(server->game_grid), 0);
}
*/

void send_game_state_to_players(Server *server) {
    printf("Sending updated game state to players...\n");  // Debug print
    for (int i = 0; i < server->num_players; i++) {
        int bytes_sent = send(server->players[i].socket, &server->game_grid, sizeof(server->game_grid), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send game state to player");
        } else {
            printf("Sent %d bytes to player %d\n", bytes_sent, i + 1);  // Debug print
        }
    }
}

void cleanup_server(Server *server) {
    for (int i = 0; i < server->num_players; i++) {
        close(server->players[i].socket);
    }
    close(server->server_socket);
    printf("Server cleaned up and shut down.\n");
}
/*
void run_server(Server *server) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Initialize server socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server->server_socket, MAX_PLAYERS);

    printf("Waiting for a client to connect...\n");
    server->players.socket = accept(server->server_socket, (struct sockaddr *)&client_addr, &addr_len);
    printf("Client connected.\n");

    initialize_game(server);

    while (1) {
        handle_player_input(server);
        send_game_state(server);
        usleep(100000); // Game pace
    }
}*/
