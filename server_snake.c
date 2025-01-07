#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include "server_snake.h"
#include <pthread.h>

#define GAME_STATE_SIZE 2428

extern pthread_mutex_t server_mutex;

void* game_loop(void* arg);

void print_active_games(Server *server) {
    printf("Active Games:\n");
    for (int i = 0; i < MAX_GAMES; i++) {
        Game *game = &server->games[i];
        if (game->is_active) {
            printf("Game ID: %d, Players: %d/%d\n", game->game_id, game->num_players, game->max_players);
        }
    }
}

void send_game_list(int client_socket, Server *server) {
    GameMessage msg;
    msg.type = MSG_GAME_LIST;
    msg.score = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        msg.games[i].game_id = -1;
        msg.games[i].num_players = 0;
    }

    int active_games_count = 0;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (server->games[i].is_active) {
            msg.games[active_games_count].game_id = server->games[i].game_id;
            msg.games[active_games_count].num_players = server->games[i].num_players;
            msg.games[active_games_count].max_players = server->games[i].max_players;
            active_games_count++;
        }
    }

    send(client_socket, &msg, sizeof(msg), 0);
}

void check_food(Game *game) {
    int width = game->game_grid.width;
    int height = game->game_grid.height;

    int total = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (game->game_grid.cells[i][j] == FOOD) {
                game->game_grid.cells[i][j] = EMPTY;
                total = 1;
                if (total == 1) {
                    break;
                }
            }
        }
        if (total == 1) {
            break;
        }
    }
}

void close_game(Server *server, int game_id) {
    if (game_id < 0 || game_id >= MAX_GAMES) {
        return;
    }

    Game *game = &server->games[game_id];

    for (int i = 0; i < game->num_players; i++) {
        GameMessage msg;
        msg.type = MSG_GAME_OVER;
        msg.score = game->players[i].snake.score;
        strncpy(msg.data, "The game has been closed by the server.", sizeof(msg.data));
        send(game->players[i].socket, &msg, sizeof(msg), 0);
        close(game->players[i].socket);
    }

    game->is_active = 0;
    if (game->thread_active) {
        pthread_join(game->thread, NULL);
        game->thread_active = 0; 
        printf("Game %d thread joined.\n", game_id);
    }

    game->num_players = 0;
    init_grid(&game->game_grid);
    //tento minusovy kokot mi zabil 3 hodiny mojho života >:O
    //server->num_games--;

    printf("Game %d has been closed.\n", game_id);
}

void init_grid(Grid *grid) {
    grid->width = GRID_WIDTH;
    grid->height = GRID_HEIGHT;
    for (int i = 0; i < grid->height; i++) {
        for (int j = 0; j < grid->width; j++) {
            grid->cells[i][j] = EMPTY;
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

    int flags = fcntl(server->server_socket, F_GETFL, 0);
    if (fcntl(server->server_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Error setting server socket to non-blocking mode");
        close(server->server_socket);
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

    if (listen(server->server_socket, MAX_PLAYERS) < 0) {
        perror("Error listening for connections");
        return -1;
    }

    printf("Server is listening on port %d...\n", port);
    server->num_games = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        server->games[i].is_active = 0;
    }

    return 0;
}

int create_new_game(Server *server, int max_players) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!server->games[i].is_active) {
            server->games[i].game_id = i;
            init_grid(&server->games[i].game_grid);

            server->games[i].num_players = 0;
            server->games[i].max_players = max_players;
            server->games[i].is_active = 1; 
            server->games[i].thread_active = 1;  
            server->games[i].server = server;

            pthread_create(&server->games[i].thread, NULL, game_loop, &server->games[i]);

            server->num_games++;

            return i;
        }
    }

    return -1;
}

int add_player_to_game(Server *server, int game_id, int player_socket) {
    if (game_id < 0 || game_id >= MAX_GAMES || !server->games[game_id].is_active) {
        return -1;
    }

    Game *game = &server->games[game_id];
    if (game->num_players >= game->max_players) {
        GameMessage msg;
        msg.type = MSG_SERVER_FULL;
        strncpy(msg.data, "Game is full. Try another game.", sizeof(msg.data));
        send(player_socket, &msg, sizeof(msg), 0);
        close(player_socket);
        return -1;
    }

    int player_id = game->num_players;
    game->players[player_id].socket = player_socket;
    srand(time(NULL));
    int start_x = 2 + rand() % (GRID_WIDTH - 4);
    int start_y = 2 + rand() % (GRID_HEIGHT - 4); 

    initializeSnake(&game->players[player_id].snake, start_x, start_y);
    game->players[player_id].playing = 1;
    game->players[player_id].sendData = 1;
    game->num_players++;
    spawnFood(&server->games[game_id].game_grid);
    return player_id;
}

void wait_for_clients(Server *server) {
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server->server_socket, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        if (FD_ISSET(server->server_socket, &read_fds)) {
            int client_socket = accept(server->server_socket, NULL, NULL);
            if (client_socket < 0) {
                perror("Error accepting client connection");
                continue;
            }

            send_game_list(client_socket, server);

            char buffer[10];
            int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';

                if (buffer[0] == 'c') {
                    int max_players = atoi(buffer + 1);
                    if (max_players <= 0 || max_players > MAX_PLAYERS_PER_GAME) {
                        max_players = MAX_PLAYERS_PER_GAME;
                    }

                    int game_id = create_new_game(server, max_players);
                    if (game_id >= 0) {
                        add_player_to_game(server, game_id, client_socket);
                        printf("Player created and joined game %d (max players: %d)\n", game_id, max_players);
                    }
                } else if (buffer[0] >= '0' && buffer[0] <= '9') {
                    int game_id = buffer[0] - '0';
                    if (game_id >= 0 && game_id < MAX_GAMES && server->games[game_id].is_active) {
                        add_player_to_game(server, game_id, client_socket);
                        printf("Player joined game %d\n", game_id);
                    }
                }
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input = getchar();
            if (input == 'q') {
                printf("Exit key pressed, shutting down server...\n");
                server_running = 0; 
                break;
            } else if (input == 'c') {
                printf("Type ID of server that you want to shut down (or 'q' to quit):\n");
                print_active_games(server);

                char game_id_input[10];
                scanf("%s", game_id_input);

                if (game_id_input[0] == 'q') {
                    printf("Quit command received. No games were closed.\n");
                } else {
                    int game_id = atoi(game_id_input);
                    if (game_id >= 0 && game_id < MAX_GAMES && server->games[game_id].is_active) {
                        close_game(server, game_id);
                    } else {
                        printf("Invalid game ID. No game closed.\n");
                    }
                }
            }
        }

        if (server->num_games == 0) {
            usleep(100000);
        }
    }
}

void handle_player_input(Server *server, int game_id, int player_id) {
    char buffer[1];
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server->games[game_id].players[player_id].socket, &read_fds);

    int result = select(server->games[game_id].players[player_id].socket + 1, &read_fds, NULL, NULL, &timeout);
    if (result > 0) {
        int bytes_received = recv(server->games[game_id].players[player_id].socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            char input = buffer[0];

            Snake *snake = &server->games[game_id].players[player_id].snake;
            switch (input) {
                case 'w': if (snake->direction != 's') snake->direction = 'w'; break;
                case 's': if (snake->direction != 'w') snake->direction = 's'; break;
                case 'a': if (snake->direction != 'd') snake->direction = 'a'; break;
                case 'd': if (snake->direction != 'a') snake->direction = 'd'; break;
                case 'k':
                    printf("Player %d in game %d left the game.\n", player_id + 1, game_id);

                    GameMessage msg;
                    msg.type = MSG_GAME_OVER;
                    msg.score = snake->score;
                    strncpy(msg.data, "You left the game.", sizeof(msg.data));
                    send(server->games[game_id].players[player_id].socket, &msg, sizeof(msg), 0);

                    close(server->games[game_id].players[player_id].socket);
                    check_food(&server->games[game_id]);

                    for (int j = player_id; j < server->games[game_id].num_players - 1; j++) {
                        server->games[game_id].players[j] = server->games[game_id].players[j + 1];
                    }
                    server->games[game_id].num_players--;

                    if (server->games[game_id].num_players == 0) {
                        printf("No players left in game %d. Resetting game state.\n", game_id);
                        init_grid(&server->games[game_id].game_grid);
                    }

                    break;
                case 'p': snake->direction = 'p'; break;
                default: break;
            }
        }
    }
}

void update_game_state(Server *server, int game_id) {
    Game *game = &server->games[game_id];
    Snake snakes[MAX_PLAYERS_PER_GAME];
    int numSnakes = 0;

    for (int i = 0; i < game->num_players; i++) {
        Snake *snake = &game->players[i].snake;
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
        if (checkCollision(&game->game_grid, snake, deltaX, deltaY)) {
            printf("Player %d in game %d hit an obstacle. Game over!\n", i + 1, game_id);

            GameMessage msg;
            msg.type = MSG_GAME_OVER;
            msg.score = snake->score;
            strncpy(msg.data, "Game Over! You hit an obstacle.", sizeof(msg.data));
            send(game->players[i].socket, &msg, sizeof(msg), 0);
            check_food(game);

            close(game->players[i].socket);
            printf("Closed connection for player %d in game %d\n", i + 1, game_id);

            for (int j = i; j < game->num_players - 1; j++) {
                game->players[j] = game->players[j + 1];
            }
            game->num_players--;

            if (game->num_players == 0) {
                printf("No players left in game %d. Resetting game state.\n", game_id);
                init_grid(&game->game_grid);
            }

            i--;
        } else {
            snakes[numSnakes++] = *snake;
        }
    }

    updateGrid(&game->game_grid, snakes, numSnakes);
}

void send_game_state_to_players(Server *server, int game_id) {
    Game *game = &server->games[game_id];
    for (int i = 0; i < game->num_players; i++) {
        GameMessage msg;
        msg.type = MSG_GAME_STATE;
        memcpy(msg.data, &game->game_grid, sizeof(game->game_grid));
        send(game->players[i].socket, &msg, sizeof(msg), 0);
    }
}

void cleanup_server(Server *server) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!server->games[i].is_active) {
            continue; 
        }

        for (int j = 0; j < server->games[i].num_players; j++) {
            if (server->games[i].players[j].socket >= 0) {
                GameMessage msg;
                memset(&msg, 0, sizeof(msg));
                msg.type = MSG_SERVER_SHUTDOWN;
                strncpy(msg.data, "Server is shutting down. Goodbye!", sizeof(msg.data) - 1);
                msg.data[sizeof(msg.data) - 1] = '\0';

                if (send(server->games[i].players[j].socket, &msg, sizeof(msg), 0) < 0) {
                    perror("Failed to send shutdown message");
                }

                if (close(server->games[i].players[j].socket) < 0) {
                    perror("Failed to close socket");
                }
            }
        }

        server->games[i].is_active = 0;

        if (server->games[i].thread_active) {
            pthread_join(server->games[i].thread, NULL);
            server->games[i].thread_active = 0;
            printf("Game %d thread joined.\n", i);
        }
    }

    if (server->server_socket >= 0) {
        close(server->server_socket);
        printf("Server socket closed and server shut down.\n");
    }
}

void* game_loop(void* arg) {
    Game* game = (Game*)arg;
    while (server_running && game->is_active) {
        pthread_mutex_lock(&server_mutex);
        update_game_state(game->server, game->game_id);
        send_game_state_to_players(game->server, game->game_id);
        pthread_mutex_unlock(&server_mutex);
        usleep(750000);
    }
    printf("Game %d thread exiting.\n", game->game_id);
    return NULL;
}