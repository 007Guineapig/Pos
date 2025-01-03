

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
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5097

#define GAME_STATE_SIZE 2428
int gameover;
int score;
int bestScore;
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);  

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int init_client(const char *server_ip, const int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        perror("Error creating socket\n");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert server IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
       perror("Invalid server IP address\n");
        close(socket_fd);
        return -1;
    }

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server\n");
        close(socket_fd);
        return -1;
    }

    printf("Connected to server at %s:%d\n", server_ip, port);
    return socket_fd;
}

void handle_user_input(int socket) {
    if (kbhit()) { 
        char input = getchar(); 
        //printf("Key pressed: %c\n", input); 
        if (input == 'w' || input == 'a' || input == 's' || input == 'd' || input == 'k' || input == 'p'){
            send(socket, &input, sizeof(input), 0); 
        }
    }
}
void receive_game_state_with_timeout(int socket, GameState *game_state) {
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    timeout.tv_sec = 0; 
    timeout.tv_usec = 10000;

    int activity = select(socket + 1, &read_fds, NULL, NULL, &timeout);
    if (activity < 0) {
        perror("Select error");
        return;
    } else if (activity == 0) {
        return;
    } else {
        GameMessage msg;
        int bytes_received = recv(socket, &msg, sizeof(msg), 0);

        if (bytes_received > 0) {
            switch (msg.type) {
                case MSG_GAME_STATE:

                    memcpy(game_state, msg.data, sizeof(GameState)); 
                    renderGrid(&game_state->grid);
                    break;

                case MSG_GAME_OVER:
                    system("clear"); 
                    printf("Game Over: %s\n", msg.data);
                    score = msg.score;
                    if(score > bestScore){
                        bestScore = score;
                    }
                    gameover = 1;
                    break;

                case MSG_SERVER_FULL:
                    printf("Server is full: %s\n", msg.data);
                    score = 0;
                    gameover = 1;
                    break;

                case MSG_SERVER_SHUTDOWN:
                    printf("Server is shutting down: %s\n", msg.data);
                    score = 0;
                    gameover = 1;
                    break;

                default:
                    printf("Unknown message type received\n");
                    break;
            }
        } else if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            gameover = 1;
        } else {
            perror("Error receiving game state\n");
            cleanup_client(socket);
            return;
        }
    }
}
void display_menu(int score) {
    system("clear");
    printf("Menu:\n");
    printf("1. View Score\n");
    printf("2. Reconnect to Server\n");
    printf("3. Exit\n");
    //printf("Your current score: %d\n", score);
    //printf("Your best score: %d\n", bestScore);
}
void printGrid(const Grid *grid) {
    system("clear");
    printf("Game Grid:\n");
    for (int i = 0; i < grid->height; i++) {
        for (int j = 0; j < grid->width; j++) {
            if (grid->cells[i][j] == EMPTY) {
                printf(". ");  
            } else if (grid->cells[i][j] == SNAKE) {
                printf("S "); 
            } else if (grid->cells[i][j] == FOOD) {
                printf("F ");  
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
    int socket = -1;
    score = 0;
    GameState game_state;
    int menu_choice = 0;
    bestScore = 0;
    gameover = 0;

    while (1) {
        if (socket < 0) {
            display_menu(score); 
            printf("Enter your choice: ");
            if (scanf("%d", &menu_choice) != 1) {
                while (getchar() != '\n');  
                printf("Invalid choice. Please enter a number.\n");
                sleep(2);
                continue; 
            }

            if (menu_choice == 1) {
               printf("Your current score: %d\n", score);
                printf("Your best score: %d\n", bestScore);
                sleep(2);
            } else if (menu_choice == 2) {
                gameover = 0;
                socket = init_client(SERVER_IP, SERVER_PORT);
                if (socket < 0) {
                    printf("Failed to connect to server. Retrying...\n");
                    sleep(2);
                }
            } else if (menu_choice == 3) {
                break;
            } else {
                printf("Invalid choice. Please try again.\n");
                sleep(2);
            }
        } else {
            handle_user_input(socket);
            receive_game_state_with_timeout(socket, &game_state);
            if (gameover == 1) {
                cleanup_client(socket);
                socket = -1;
            }

            if (socket < 0) {
                printf("Press Enter to return to menu.\n");
                while (1) {
                    if (getchar() == '\n') {
                        break;  
                    }
                }
            }

            usleep(10000);
        }
    }

    if (socket >= 0) {
        cleanup_client(socket);
    }

    return 0;
}