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
#include <stdatomic.h>
time_t last_player_activity = 0;
_Atomic int server_running = 1;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t game_state_updated = PTHREAD_COND_INITIALIZER;
GameTimer game_timers[MAX_GAMES];


void* server_shutdown_timer_thread(void* arg) {
    Server* server = (Server*)arg;

    while (atomic_load(&server_running)) {
        pthread_mutex_lock(&server_mutex);

        int players_active = 0;
        for (int i = 0; i < MAX_GAMES; i++) {
            if (server->games[i].is_active && server->games[i].num_players > 0) {
                players_active = 1;
                last_player_activity = time(NULL);
                break;
            }
        }

        if (!players_active && last_player_activity != 0 && (time(NULL) - last_player_activity) >= 10) {
            //printf("No players for 20 seconds. Shutting down server...\n");
            atomic_store(&server_running, 0);
        }

        pthread_mutex_unlock(&server_mutex);
        sleep(1);
    }

    return NULL;
}

void* handle_game_timer_thread(void* arg) {
    Server* server = (Server*)arg;
    while (atomic_load(&server_running)) {
        pthread_mutex_lock(&server_mutex);
        for (int i = 0; i < server->num_games; i++) {
            if (server->games[i].is_active && server->games[i].num_players == 0) {
                if (!game_timers[i].timer_started) {
                    game_timers[i].timer_started = 1;
                    game_timers[i].last_player_left_time = time(NULL);
                } else {
                    time_t current_time = time(NULL);
                    if (current_time - game_timers[i].last_player_left_time >= 10) {
                        close_game(server, i);
                        game_timers[i].timer_started = 0;
                    }
                }
            } else if (server->games[i].num_players > 0) {
                game_timers[i].timer_started = 0;
            }
        }
        pthread_mutex_unlock(&server_mutex);
        usleep(1000000); 
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
    while (atomic_load(&server_running)) {
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

int main(int argc, char *argv[]) {
    atomic_store(&server_running, 1);

    Server server;
    int port = 5097;
    srand(time(NULL));

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    if (init_server(&server, port) < 0) {
        return -1;
    }

    pthread_mutex_init(&server_mutex, NULL);
    pthread_cond_init(&game_state_updated, NULL);

    pthread_t input_thread, timer_thread,server_shutdown_timer_thread_id;
    pthread_create(&input_thread, NULL, handle_player_input_thread, &server);
    pthread_create(&timer_thread, NULL, handle_game_timer_thread, &server);
    pthread_create(&server_shutdown_timer_thread_id,NULL, server_shutdown_timer_thread, &server);


    while (atomic_load(&server_running)) {
        wait_for_clients(&server);
         
        /*
        if (kbhit() && getchar() == 'q') {
            server_running = 0;
            break;
        }
        */

        usleep(100000);
                


    }


    for (int i = 0; i < server.num_games; i++) {
        if (server.games[i].is_active) {
            pthread_join(server.games[i].thread, NULL);
        }
    }


    pthread_join(input_thread, NULL);
    pthread_join(timer_thread, NULL);
    pthread_join(server_shutdown_timer_thread_id, NULL);

    pthread_mutex_destroy(&server_mutex);
    pthread_cond_destroy(&game_state_updated);


    cleanup_server(&server);

    return 0;
}
