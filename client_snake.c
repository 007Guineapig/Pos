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
#include <sys/wait.h>
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5097
#define GAME_STATE_SIZE 2428
int jo;
int gameover;
int score;
char represent_snake;
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
    memset(&server_addr, 0, sizeof(server_addr));
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

void cleanup_client(int *socket) {
    if (*socket >= 0) {
        close(*socket);
        *socket = -1;
    }
}

void sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int start_server() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Failed to set up SIGCHLD handler");
        return -1;
    }

    int socket = init_client(SERVER_IP, SERVER_PORT);

    if (socket == -1) {
        pid_t pid = fork();
        if (pid == 0) {
            pid_t pid2 = fork();
            if (pid2 == 0) {
                execl("./server_snake", "server_snake", NULL);
                perror("Failed to start server");
                jo = 0;
            } else if (pid2 > 0) {
                jo = 0;
            } else {
                perror("Failed to fork second child process");
                jo = 0;
            }
        } else if (pid > 0) {
            printf("Server process created with PID: %d\n", pid);
            sleep(2);
            int retries = 5;

            while (retries--) {
                socket = init_client(SERVER_IP, SERVER_PORT);
                if (socket >= 0) {
                    printf("Connected to server successfully.\n");
                    gameover = 0;
                    return socket;
                } else {
                    printf("Failed to connect to server. Retries left: %d\n", retries);
                    cleanup_client(&socket);
                    sleep(2);
                }
            }

            fprintf(stderr, "Failed to connect to server after multiple retries.\n");
            return -1;
        } else {
            perror("Failed to fork server process");
            return -1;
        }
    } else {
        printf("Connected to server successfully.\n");
        gameover = 0;
        return socket;
    }
    return socket;
}

void display_game_list(GameMessage *msg) {
    system("clear");
    printf("Available Games:\n");
    int numeroGames = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        if (msg->games[i].game_id != -1) {
            printf("Game %d: %d/%d players\n", msg->games[i].game_id, msg->games[i].num_players, msg->games[i].max_players);
            numeroGames++;
        }
    }

    if (numeroGames == 0) {
        printf("None available existing games\n");
    }
    printf("_______________________________________________________________________\n");
    printf("Press 'c' to create a new game, press a number to join an existing game\n");
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
                while (1) {
                    fgets(max_players_input, sizeof(max_players_input), stdin);
                    max_players = atoi(max_players_input);

                    if (max_players > 0 && max_players <= MAX_PLAYERS_PER_GAME) {
                        break;
                    } else {
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
                send(socket, &input, sizeof(input), 0);
                close(socket);
                
            }
        } else {
            if (input == 'w' || input == 'a' || input == 's' || input == 'd' || input == 'k' || input == 'p') {
                send(socket, &input, sizeof(input), 0);
            }
        }
    }
}

void receive_game_state_with_timeout(int socket, GameState *game_state,char represent_snake) {
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
                    game_state->snake = msg.snake;
                    renderGrid(&game_state->grid,&game_state->snake,represent_snake);
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

void display_menu() {
    system("clear");
    printf("Menu:\n");
    printf("1. View Score\n");
    printf("2. Reconnect to Server\n");
    printf("3. Keyboard Overlay\n");
    printf("4. Choose snake representation\n");
    printf("5. Exit\n");
    printf("your skin : %c\n",represent_snake);
}


int main() {
    represent_snake= '1';
    jo = 1;
    int socket = -1;
    score = 0;
    GameState game_state;
    int menu_choice = 0;
    bestScore = 0;
    gameover = 0;
    int game_chosen = 0;

        while (1) {
            if (socket < 0) {
            display_menu();
            printf("Enter your choice: \n");
            if (scanf("%d", &menu_choice) != 1) {
                while (getchar() != '\n');
                printf("Invalid choice. Please enter a number.\n");
                sleep(2);
                continue;
            }
            while (getchar() != '\n');

            if (menu_choice == 1) {
                system("clear");
                printf("Your last gameplay score: %d\n", score);
                printf("Your best score: %d\n", bestScore);
                printf("Press Enter to return to menu.\n");
                while (getchar() != '\n');
            } else if (menu_choice == 2) {
                socket = start_server();

                if(jo == 0){
                    break;
                }

                if (socket >= 0) {
                    gameover = 0;
                    game_chosen = 0;
                }
            } else if (menu_choice == 5) {
                cleanup_client(&socket);
                break;
            } else if (menu_choice == 3) {
                printf("W, A, S, D - move forward, left, backward and right\n");
                printf("P - pause/stop snake movement\n");
                printf("K - leave the game\n");
                printf("C - create a new game\n");
                printf("Q - quit and return to the main menu\n");
                printf("Press Enter to return to menu.\n");
                while (getchar() != '\n');
                while (getchar() != '\n');
            } else if (menu_choice == 4) {
                system("clear");
                printf("Choose your game skin :)\n");
                printf("Press key on keyboard\n");

                char input[100];
                char represent;
                while (1) {
                    if (fgets(input, sizeof(input), stdin) != NULL) {
                        if (input[0] != '\n' && input[0] != 'F' && input[1] == '\n' && isprint(input[0])) {
                        represent = input[0];
                        break; 
                        } else {
                            printf("Invalid input. Please choose a single printable character (not space or enter):\n");
                        }
                    }
                }

                system("clear");
                printf("You chose '%c' as your game skin!\n", represent);
                printf("Press Enter to return to menu.\n");
                represent_snake = represent;
                while (getchar() != '\n');

            } else {
                printf("Invalid choice. Please try again.\n");
                printf("Press Enter to return to menu.\n");
                while (getchar() != '\n');
            }
            } else {
            handle_user_input(socket, &game_chosen);

            if (game_chosen == -1) {
                cleanup_client(&socket);
                socket = -1;
                game_chosen = 0;
                continue;
            }
            receive_game_state_with_timeout(socket, &game_state,represent_snake);
            
            if (gameover == 1) {
                cleanup_client(&socket);
                socket = -1;
                game_chosen = 0;
            }

            if (socket < 0) {
                printf("Press Enter to return to menu.\n");
                while (getchar() != '\n');
            }

            usleep(10000);
        }
    }

    if (socket >= 0) {
        cleanup_client(&socket);
    }

    return 0;
}
