#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

typedef struct {
    int dir_x;
    int dir_y;
    int x;
    int y;
    int score_1;
    int score_2;
} Ball;

typedef struct {
    int dir_y;
    int x;
    int y;
} PlayerSkin;

typedef struct{
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr, clientAddrs[2];
    int addr_size;
} GameThreadArgs;

FILE *logfd;


void ballPhysics(Ball* ball);
void reset(Ball* ball);
void ballBounce(Ball* ball);

void playerPhysics(PlayerSkin* player);
void hit(PlayerSkin* player, Ball* ball, int Player_1);

int find_client_index(struct sockaddr_in *clientAddrs, struct sockaddr_in client);
void log_info(const char *message);
void log_error(const char *message);

void *handle_game(void *arg);

int main(int argc, char *argv[]);


void ballPhysics(Ball* ball) {
    ball->x += ball->dir_x;
    ball->y += ball->dir_y;
}

void reset(Ball* ball) {
    srand(time(NULL));
    ball->x = 380;
    ball->y = 280;
    ball->dir_x = -(ball->dir_x);
    ball->dir_y = (rand() % 2) ? -2 : 2;
}

void ballBounce(Ball* ball) {
    if (ball->x <= -40) {
        reset(ball);
        ball->score_2 += 1;
    }
    if (ball->x >= 800) {
        reset(ball);
        ball->score_1 += 1;
    }
    if (ball->y <= 0) {
        ball->dir_y = -(ball->dir_y);
    }
    if (ball->y + 40 >= 600) {
        ball->dir_y = -(ball->dir_y);
    }
}

void playerPhisics(PlayerSkin* player) {
    player->y += player->dir_y;
    if (player->y <= 0) {
        player->y = 0;
    }
    if (player->y + 100 >= 600) {
        player->y = 600 - 100;
    }
}

void hit(PlayerSkin* player, Ball* ball, int Player_1) {
    if (ball->x < player->x + 22 && ball->x > player->x && ball->y + 40 > player->y && ball->y < player->y + 100) {
        ball->dir_x = -ball->dir_x;
        ball->x = Player_1 ? (player->x + 22) : (player->x - 40);
    }
}

int find_client_index(struct sockaddr_in *clientAddrs, struct sockaddr_in client) {
    for (int i = 0; i < 2; i++) {
        if (clientAddrs[i].sin_addr.s_addr == client.sin_addr.s_addr && clientAddrs[i].sin_port == client.sin_port) {
            return i;
        }
    }
    return -1;
}

void log_info(const char *message){
    fprintf(logfd, "[INFO] %s\n", message);
    fflush(logfd);
}

void log_error(const char *message){
    fprintf(logfd, "[ERROR] %s\n", message);
    fflush(logfd);
}

void *handle_game(void *arg)
{
    GameThreadArgs *args = (GameThreadArgs *)arg;
    char start_msg[] = "START";
    for (int i = 0; i < 2; i++)
    {
        sendto(args->sockfd, start_msg, strlen(start_msg), 0, (struct sockaddr *)&args->clientAddrs[i], args->addr_size);
    }

    printf("El juego está comenzando...\n");
    log_info("El juego está comenzando\n");

    Ball ball;

    ball.x = 380;
    ball.y = 280;

    int random_number = rand() % 2;
    ball.dir_x = (random_number == 0) ? -2 : 2;
    random_number = rand() % 2;
    ball.dir_y = (random_number == 0) ? -2 : 2;

    ball.score_1 = 0;
    ball.score_2 = 0;

    PlayerSkin player_1, player_2;

    player_1.x = 60;
    player_1.y = 250;
    player_1.dir_y = 0;

    player_2.x = 718;
    player_2.y = 250;
    player_2.dir_y = 0;

    char buffer[BUFFER_SIZE];

    while (1)
    {
        int bytesReceived = recvfrom(args->sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&args->clientAddr, &args->addr_size);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';

            int sendingClientIndex = find_client_index(args->clientAddrs, args->clientAddr);
            char logMessage[256];
            sprintf(logMessage, "El jugaror %d está: %d.\n", sendingClientIndex, atoi(buffer));
            printf(logMessage);
            log_info(logMessage);

            if (sendingClientIndex == 0)
            {
                player_1.dir_y = atoi(buffer);
            }
            else if (sendingClientIndex == 1)
            {
                player_2.dir_y = atoi(buffer);
            }

            ballPhysics(&ball);
            ballBounce(&ball);
            playerPhisics(&player_1);
            playerPhisics(&player_2);
            hit(&player_1, &ball, 1);
            hit(&player_2, &ball, 0);

            for (int clientIndex = 0; clientIndex < 2; clientIndex++)
            {
                char tempBuffer2[256];
                sprintf(tempBuffer2, "%d %d %d %d %d %d", ball.x, ball.y, player_1.y, player_2.y, ball.score_1, ball.score_2);
                sendto(args->sockfd, tempBuffer2, strlen(tempBuffer2), 0, (struct sockaddr *)&args->clientAddrs[clientIndex], args->addr_size);
            }
        }
    }
}



int main(int argc, char *argv[]) {
    if (argc != 3){
        fprintf(stderr, "Uso: %s <PORT> <Log File>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *logFile = argv[2];

    logfd = fopen(logFile, "a");
    if (logfd == NULL)
    {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error al enlazar");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in clientAddr;
    socklen_t addr_size = sizeof(clientAddr);
    struct sockaddr_in clientAddrs[2];

    char buffer[BUFFER_SIZE];

    while (1)
    {
        int connected_clients = 0;
        while (connected_clients < 2)
        {
            int bytesReceived = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addr_size);
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                if (strcmp(buffer, "CONNECT") == 0 && find_client_index(clientAddrs, clientAddr) == -1)
                {
                    clientAddrs[connected_clients] = clientAddr;
                    connected_clients++;
                    printf("%d", connected_clients);
                    printf("Jugador %d conectado%s:%d\n", connected_clients, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                    char log_message[256];
                    sprintf(log_message, "Jugador %d conectado%s:%d", connected_clients, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                    log_info(log_message);
                }
            }
        }

        if (connected_clients == 2){
            pthread_t game_thread;
            GameThreadArgs *args = malloc(sizeof(GameThreadArgs));
            memcpy(args->clientAddrs, clientAddrs, sizeof(clientAddrs));
            args->sockfd = sockfd;
            args->addr_size = sizeof(clientAddrs[0]);
            pthread_create(&game_thread, NULL, handle_game, (void *)args);
            pthread_detach(game_thread);
        }
    }
    close(sockfd);

    return 0;
}