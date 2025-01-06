#include "server_snake.h"
#include <time.h>
#include "client_snake.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "shared_game_state.h"
#include <pthread.h>

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t game_state_updated = PTHREAD_COND_INITIALIZER;
GameTimer game_timers[MAX_GAMES];
volatile int server_running = 1; 


void* handle_game_timer_thread(void* arg) {
    Server* server = (Server*)arg;
    while (server_running) {
        pthread_mutex_lock(&server_mutex);
        for (int i = 0; i < server->num_games; i++) {
            if (server->games[i].is_active && server->games[i].num_players == 0) {
                if (!game_timers[i].timer_started) {
                    // Start the timer when the last player leaves
                    game_timers[i].timer_started = 1;
                    game_timers[i].last_player_left_time = time(NULL);
                    printf("Game %d timer started (no players).\n", i);
                } else {
                    // Check if 10 seconds have passed since the last player left
                    time_t current_time = time(NULL);
                    if (current_time - game_timers[i].last_player_left_time >= 10) {
                        printf("Game %d timer expired. Closing game.\n", i);
                        close_game(server, i);
                        game_timers[i].timer_started = 0; // Reset the timer
                    }
                }
            } else if (server->games[i].num_players > 0) {
                // Reset the timer if players have joined
                game_timers[i].timer_started = 0;
            }
        }
        pthread_mutex_unlock(&server_mutex);
        usleep(1000000); // Sleep for 1 second
    }
    return NULL;
}


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

void* handle_player_input_thread(void* arg) {
    Server* server = (Server*)arg;
    while (server_running) {
        pthread_mutex_lock(&server_mutex);
        for (int i = 0; i < server->num_games; i++) {
            if (server->games[i].is_active) {
                for (int j = 0; j < server->games[i].num_players; j++) {
                    handle_player_input(server, i, j);
                }
            }
        }
        pthread_mutex_unlock(&server_mutex);
        usleep(100000);
    }
    return NULL;
}

void* update_game_state_thread(void* arg) {
    Server* server = (Server*)arg;
    while (server_running) {
        pthread_mutex_lock(&server_mutex);
        for (int i = 0; i < server->num_games; i++) {
            if (server->games[i].is_active) {
                update_game_state(server, i);
                send_game_state_to_players(server, i);
            }
        }
        pthread_mutex_unlock(&server_mutex);
        usleep(750000);  
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    Server server;
    int port = 5097;
    srand(time(NULL));

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    if (init_server(&server, port) < 0) {
        printf("Failed to initialize server\n");
        return -1;
    }

    pthread_mutex_init(&server_mutex, NULL);
    pthread_cond_init(&game_state_updated, NULL);

    pthread_t input_thread, update_thread,timer_thread;
    pthread_create(&input_thread, NULL, handle_player_input_thread, &server);
    pthread_create(&update_thread, NULL, update_game_state_thread, &server);
    pthread_create(&timer_thread, NULL, handle_game_timer_thread, &server);


    while (server_running) {
        wait_for_clients(&server);
        if (kbhit() && getchar() == 'e') {
            printf("Exit key pressed, shutting down server...\n");
            server_running = 0;
            break;
        }

        usleep(100000);
    }

    pthread_join(input_thread, NULL);
    pthread_join(update_thread, NULL);
    pthread_join(timer_thread, NULL);

    pthread_mutex_destroy(&server_mutex);
    pthread_cond_destroy(&game_state_updated);

    cleanup_server(&server);

    return 0;
}
