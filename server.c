#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common.h"

// macro ajoutées
#define RES_LOCAL 1
#define RES_INTERNET 2

#define NB_CLIENT_MAX 2

void init_board(display_info disp_struct, struct client_input* client_struct)
{
    disp_struct.winner = NO_WINNER_YET;

    // empty the grid
    memset(disp_struct.board, EMPTY_SQUARE, XMAX * YMAX);

    // fill borders
    for (size_t i = 0; i < XMAX; i++)
    {
        disp_struct.board[i][0] = WALL;
        disp_struct.board[i][YMAX - 1] = WALL;
    }
    for (size_t i = 0; i < YMAX; i++)
    {
        disp_struct.board[0][i] = WALL;
        disp_struct.board[XMAX - 1][i] = WALL;
    }
    
    // init players
    disp_struct.board[(XMAX / 2) - 3][YMAX / 2] = client_struct[0].id;
    client_struct[0].input = UP;
    disp_struct.board[(XMAX / 2) + 3][YMAX / 2] = client_struct[1].id;
    client_struct[1].input = DOWN;
}

int main(int argc, char **argv)
{
    // verification du nombre d'arguments sur la ligne de commande
    if (argc != 3)
    {
        printf("Usage: %s port_local refresh_rate\n", argv[0]);
        exit(-1);
    }

    int local_port = atoi(argv[1]);
    CHECK(local_port != 0);
    int refresh_rate = atoi(argv[2]);
    CHECK(refresh_rate != 0);

    struct sockaddr_in my_addr;     // my struct
    struct sockaddr_in clients[2];  // clients struct

    int sockfd;         // machine port
    int client_fd[2];   // TCP specific port

    // taille d'une structure sockaddr_in utile pour la fonction recvfrom
    unsigned int sin_size = sizeof(struct sockaddr_in);

    // type de reseau (RES_LOCAL: 2 sur un clavier ou RES_INTERNET : plusieurs machines)
    int res_type, nb_of_machines;

    // creation de la socket
    CHECK((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != -1);

    // initialisation de la structure d'adresse du recepteur (pg local)
    memset(&my_addr, 0, sin_size);
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(local_port);

    // bind: association de la socket et des param reseaux du recepteur
    CHECK(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == 0);
    char *ip_str = inet_ntoa(my_addr.sin_addr); // Promis c'est comme ça que ça s'utilise
    printf("Bind successful. IP @: %s, port no: %d\n", ip_str, SERV_PORT);

    // listen
    CHECK(listen(sockfd, NB_CLIENT_MAX) != -1);

    // accept premier client
    CHECK((client_fd[0] = accept(sockfd, (struct sockaddr *)&(clients[0]), &sin_size)) != -1);

    // reception du nb de joueurs sur la machine
    int players; 
    CHECK(recvfrom(client_fd[0], &players, sizeof(int), 0, (struct sockaddr *)&(clients[0]), &sin_size) != -1);

    // affichage de la chaine de caracteres recue
    printf("Connexion with: %d, port no: %d, succesful,"
    " message received: %d \n", clients[0].sin_addr.s_addr, clients[0].sin_port, players);

    if(players == 2)
    {
        // Cas multi local
        res_type = nb_of_machines = RES_LOCAL;
    }
    else
    {
        // Cas multi réseau
        res_type = nb_of_machines = RES_INTERNET;

        printf("Waiting for another player (1/2)...\n");

        CHECK(listen(sockfd, NB_CLIENT_MAX) != -1);
        CHECK((client_fd[1] = accept(sockfd, (struct sockaddr *)&(clients[1]), &sin_size)) != -1);
        CHECK(recvfrom(client_fd[1], NULL, 0, 0, (struct sockaddr *)&(clients[1]), &sin_size) != -1);

        printf("Connexion with: %d, port no: %d, succesful\n"
        "Game will start !!!\n", clients[1].sin_addr.s_addr, clients[1].sin_port);
    }

    struct client_input client_in[2];
    // TODO : Envoyer et garder en mémoire des ID pour les joueurs
    // Ces id permettent de savoir quelle moto appartient à qui

    display_info display_struct;
    init_board(display_struct, client_in);

    // TODO : Initialiser un thread qui récupère les inputs client
    // ce thread pourra surement aussi être utilisé pour les commandes (reset/quit) 
    // dans le terminal du server (avec select)
    // TODO : Initialiser un timer de refresh rate ms
    // TODO : à la fin de ce timer calculer et envoyer la structure
    // avec la grille aux clients
    // TODO : Si qqun a gagné / égalité, envoyer l'info aux clients et terminer execution
    // (attendre qq secondes, fermer les threads/semaphores/fd/...)

    // fermeture de la socket
    close(client_fd[0]);
    close(client_fd[1]);
    close(sockfd);

    return 0;
}
