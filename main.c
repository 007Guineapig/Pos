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
#include "shared_game_state.h"

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

    wait_for_clients(&server);

    while (1){
        if(server.num_players == 0) {
            init_grid(&server);
            printf("No players connected. Waiting for players...\n");
            wait_for_clients(&server);
            continue; 
        }
        for(int i = 0; i < server.num_players; i++) {
            handle_player_input(&server, i);
        }

        check_food(&server);
        update_game_state(&server);
        send_game_state_to_players(&server);
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server.server_socket, &read_fds);

        struct timeval timeout = {0, 0};
        int result = select(server.server_socket + 1, &read_fds, NULL, NULL, &timeout);

        if(result > 0 && server.num_players < MAX_PLAYERS) {

            int client_socket = accept(server.server_socket, NULL, NULL);
            if(client_socket >= 0) {
                int random_width = (rand() % 18) + 1;
                 int random_height = (rand() % 18) + 1;
                int player_index = server.num_players;
                server.players[player_index].socket = client_socket;
                //int start_x = GRID_WIDTH / 2 + player_index * 5;
                //int start_y = GRID_HEIGHT / 2;
                int start_x = random_width;
                int start_y = random_height;
                initializeSnake(&server.players[player_index].snake, start_x, start_y);
                server.num_players++;
                printf("New player %d joined the game\n", server.num_players);
                send(client_socket, &server.game_grid, sizeof(server.game_grid), 0);
                spawnFood(&server.game_grid);
            }
        }
        if (kbhit() && getchar() == 'e') {
            printf("Exit key pressed, shutting down server...\n");
            cleanup_server(&server);
            break;
        }

        usleep(750000);
    }
    return 0;
}