

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
#include <ctype.h>
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

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
       perror("Invalid server IP address\n");
        close(socket_fd);
        return -1;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server\n");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

void display_game_list(GameMessage *msg) {
    system("clear");
    printf("Available Games:\n");
    int numeroGames = 0;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (msg->games[i].game_id != -1) {  
            printf("Game %d: %d/%d players\n", msg->games[i].game_id, msg->games[i].num_players,msg->games[i].max_players);
            numeroGames++;
        }
    }
    if(numeroGames == 0){
        printf("None available existing games\n");
    }
    printf("_______________________________________________________________________\n");
    printf("Press 'c' to create a new game, press a number to join an existing game\n");
    printf("Press a number to join an existing game\n");
    printf("Press 'q' to quit and return to the main menu.\n");

}

void handle_user_input(int socket, int *game_chosen) {
    if (kbhit()) {
        char input = getchar();

        if (!(*game_chosen)) {
            if (input == 'c') {  
                system("clear");
                printf("Enter the maximum number of players for the new game: \n");
                printf("Choose between <1,%d>\n", MAX_PLAYERS_PER_GAME);
                char max_players_input[10];
                int max_players = 0;
                while(1){
                fgets(max_players_input, sizeof(max_players_input), stdin);
                max_players = atoi(max_players_input);

                if (max_players > 0 && max_players <= MAX_PLAYERS_PER_GAME) {
                    break;
                    
                }else {
                    printf("Invalid input: %d\n", max_players);
                    printf("Choose between <1,%d>\n", MAX_PLAYERS_PER_GAME);
                }
                }

            
                char command[10];
                snprintf(command, sizeof(command), "c%d", max_players);
                send(socket, command, strlen(command), 0);

                *game_chosen = 1;  
            } else if (input >= '0' && input <= '9') {  
                send(socket, &input, sizeof(input), 0);
                *game_chosen = 1;
            } else if (input == 'q') {  
                *game_chosen = -1;  
            }
        } else {
           
            if (input == 'w' || input == 'a' || input == 's' || input == 'd' || input == 'k' || input == 'p') {
                send(socket, &input, sizeof(input), 0);
            }
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
                 cleanup_client(&socket);
                    system("clear");
                    printf("Game Over: %s\n", msg.data);
                    score = msg.score;
                    if (score > bestScore) {
                        bestScore = score;
                    }
                    gameover = 1;
                    break;

                case MSG_SERVER_FULL:
                 cleanup_client(&socket);
                    printf("Server is full: %s\n", msg.data);
                    score = 0;
                    gameover = 1;
                    break;

                case MSG_SERVER_SHUTDOWN:
                 cleanup_client(&socket);
                    printf("Server is shutting down: %s\n", msg.data);
                    score = 0;
                    gameover = 1;
                    break;

                case MSG_GAME_LIST:  
                    display_game_list(&msg);
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
            cleanup_client(&socket);
            return;
        }
    }
}

void display_menu(int score) {
    system("clear");
    printf("Menu:\n");
    printf("1. View Score\n");
    printf("2. Reconnect to Server\n");
    printf("3. keyboard overlay\n");
    printf("4. Exit\n");

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

void cleanup_client(int *socket) {
    if (*socket >= 0) {
        close(*socket);
    }
    *socket = -1; 
}

int main() {
    int socket = -1;
    score = 0;
    GameState game_state;
    int menu_choice = 0;
    bestScore = 0;
    gameover = 0;
    int game_chosen = 0;  

    while (1) {
        if (socket < 0) {
            display_menu(score);
            printf("Enter your choice: \n");
            if (scanf("%d", &menu_choice) != 1) {
                while (getchar() != '\n');
                printf("Invalid choice. Please enter a number.\n");
                sleep(2);
                continue;
            }

            if (menu_choice == 1) {
                printf("Your last gameplay score: %d\n", score);
                printf("Your best score: %d\n", bestScore);
                printf("Press Enter to return to menu.\n");
                int cd;
                while ((cd = getchar()) != '\n' && cd != EOF);
                while (1) {
                    if (getchar() == '\n') {
                        break;
                    }
                }
            } else if (menu_choice == 2) {
                gameover = 0;
                socket = init_client(SERVER_IP, SERVER_PORT);
                if (socket < 0) {
                    printf("Failed to connect to server. Retrying...\n");
                    cleanup_client(&socket);
                    sleep(2);
                } 
            } else if (menu_choice == 4) {
                 cleanup_client(&socket);
                break;
            } else if (menu_choice == 3) {
                printf("W, A, S, D - move forward, left, backward and right\n");
                printf("P - pause/stop snake movement\n");
                printf("K - leave the game\n");
                printf("C - create a new game\n");
                printf("Q - quit and return to the main menu\n");
                printf("Press Enter to return to menu.\n");
                int cd;
               while ((cd = getchar()) != '\n' && cd != EOF);
                while (1) {
                    if (getchar() == '\n') {
                        break;
                    }
                }
            } else {
                printf("Invalid choice. Please try again.\n");
                printf("Press Enter to return to menu.\n");
                while (1) {
                    if (getchar() == '\n') {
                        break;
                    }
                }
            }
        } else {
            handle_user_input(socket, &game_chosen);  
            if (game_chosen == -1) {  
                cleanup_client(&socket);
                socket = -1;
                game_chosen = 0;  
                continue;
            }

            receive_game_state_with_timeout(socket, &game_state);
            if (gameover == 1) {
                cleanup_client(&socket);
                socket = -1;
                game_chosen = 0;  
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
        cleanup_client(&socket);
    }

    return 0;
}