

#include "client_snake.h"
#include "snake.h"
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
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5097

#define GAME_STATE_SIZE 2428

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);  // Get the current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // Apply new settings
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);  // Set non-blocking mode
    ch = getchar();  // Get character without waiting for Enter
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore old terminal settings
    fcntl(STDIN_FILENO, F_SETFL, oldf);  // Restore old file flags

    if (ch != EOF) {
        ungetc(ch, stdin);  // Put the character back to stdin
        return 1;
    }
    return 0;
}

int init_client(const char *server_ip, const int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        perror("Error creating socket");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert server IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
       perror("Invalid server IP address");
        close(socket_fd);
        return -1;
    }

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(socket_fd);
        return -1;
    }

    printf("Connected to server at %s:%d\n", server_ip, port);
    return socket_fd;
}

void handle_user_input(int socket) {
    if (kbhit()) {  // Check if a key is pressed
        char input = getchar(); 
        printf("Key pressed: %c\n", input); 
        if (input == 'w' || input == 'a' || input == 's' || input == 'd' || input == 'k' || input == 'p'){
            send(socket, &input, sizeof(input), 0);  // Send input to the server
        }
    }
}

void receive_game_state_with_timeout(int socket, GameState *game_state) {
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    timeout.tv_sec = 0;  // Timeout after 1 second
    timeout.tv_usec = 10000;

    int activity = select(socket + 1, &read_fds, NULL, NULL, &timeout);
    if (activity < 0) {
        perror("Select error");
        return ;
    } else if (activity == 0) {
        return;
    } else {
        int bytes_received = recv(socket, game_state, sizeof(GameState), 0);
        if (bytes_received > 0) {
            printf("Received game state, grid dimensions: %d x %d\n", game_state->grid.width, game_state->grid.height);
            printGrid(&game_state->grid);
            return;
        } else if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            cleanup_client(socket);
            exit(0);
        } else {
            perror("Error receiving game state");
            cleanup_client(socket);
            return;
        }
    }
}
/*
void receive_game_state_with_timeout(int socket, GameState *game_state) {
    char buffer[GAME_STATE_SIZE]; // Allocate buffer for incoming data
    DataPipeline pipeline;
    init_pipeline(&pipeline);

    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    int activity = select(socket + 1, &read_fds, NULL, NULL, &timeout);
    if (activity > 0) {
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            // Deserialize received data into game state
            if (pipeline.deserialize(buffer, bytes_received, game_state) < 0) {
                printf("Failed to deserialize game state\n");
            } else {
                //printf("Received and processed game state\n");
                  printGrid(&game_state->grid);
            }
        } else if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            cleanup_client(socket);
            exit(0);
        } else {
            perror("Error receiving game state");
        }
    }
}
*/
void printGrid(const Grid *grid) {
    system("clear");
    printf("Game Grid:\n");
    for (int i = 0; i < grid->height; i++) {
        for (int j = 0; j < grid->width; j++) {
            if (grid->cells[i][j] == EMPTY) {
                printf(". ");  // Empty space
            } else if (grid->cells[i][j] == SNAKE) {
                printf("S ");  // Snake
            } else if (grid->cells[i][j] == FOOD) {
                printf("F ");  // Food
            }
        }
        printf("\n");
    }
}
void cleanup_client(int socket) {
    close(socket);
    printf("Client connection closed.\n");
}

int main() {
char str;
/*
       while (1) {
        printf("Press 'z' to start the game: ");
        scanf(" %c", &str);  

        if (str == 'z') {
            break;
        }
    }*/

    int socket = init_client(SERVER_IP, SERVER_PORT);
    if (socket < 0) {
      return -1;
    }
  GameState game_state;
    while (1) {
      handle_user_input(socket);
      receive_game_state_with_timeout(socket,&game_state);
        usleep(10000);             

    }
    cleanup_client(socket);
    return 0;
}
