/*
Doit etre capable de gerer en meme temps
    - un timer et son expiration
    - les inputs du client

idees : 
    - fork -> enfant s'occupe des inputs client, parent cree un timer, l'attend, tue le proc enfant
              dangereux parceque duplication des socket et des variables 
              (necessite pipe/valeur de retour pour communiquer les inputs du client au parent)
    - thread -> similaire au fork mais necessite moins de ressources (normalement)
                se rappeler d'utiliser mutex/semaphores pour les sections critiques s'il y en a
                on peut reutiliser un unique thread en le forcant a attendre pendant la periode de calcul
                plutot que de fermer/ouvrir a chaque iteration
                init_sem(S, 0), input : select; P(); get_input; V() // calcul : sleep(2); P(); calcul; V()

Doit egalement envoyer et garder en memoire les id des clients pour les differencier (particulierement en multi local)
*/

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

#define BLUE_ON_BLACK       0
#define RED_ON_BLACK        2
#define YELLOW_ON_BLACK     1
#define MAGENTA_ON_BLACK    3
#define CYAN_ON_BLACK       4

#define BLUE_ON_BLUE        50
#define RED_ON_RED          52
#define YELLOW_ON_YELLOW    51
#define MAGENTA_ON_MAGENTA  53
#define CYAN_ON_CYAN        54

#define EMPTY_SQUARE -1

void generate_random_board(char board[XMAX][YMAX])
{
    for (size_t i = 0; i < XMAX; i++)
    {
        for (size_t j = 0; j < YMAX; j++)
        {
            board[i][j] = (rand() % NB_COLORS);
            if(i == 0)
                printf("%d, ", (int) board[i][j]);
        }
    }
}

void init_board(char board[XMAX][YMAX])
{
    memset(board, -1, XMAX * YMAX);

    board[10][20] = CYAN_ON_BLACK;
    board[11][20] = CYAN_ON_CYAN;
    board[12][20] = CYAN_ON_CYAN;
    board[13][20] = CYAN_ON_CYAN;

    for (size_t i = 0; i < XMAX; i++)
    {
        for (size_t j = 0; j < YMAX; j++)
        {
            if (i == 0)
            {
                board [i][j] = WALL;
            }
        }
    }
}

int main(int argc, char **argv)
{
    int sockfd;     // descripteur de socket
    char buf[1024]; // espace necessaire pour stocker le message recu

    // taille d'une structure sockaddr_in utile pour la fonction recvfrom
    socklen_t fromlen = sizeof(struct sockaddr_in);

    struct sockaddr_in my_addr; // structure d'adresse qui contiendra les param reseaux du recepteur
    struct sockaddr_in client;  // structure d'adresse qui contiendra les param reseaux de l'expediteur

    int new_fd; // nouvelle connexion sur new_fd
    unsigned int sin_size;

    // verification du nombre d'arguments sur la ligne de commande
    if (argc != 2)
    {
        printf("Usage: %s port_local\n", argv[0]);
        exit(-1);
    }

    // creation de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket init failure");
        exit(EXIT_FAILURE);
    }

    // initialisation de la structure d'adresse du recepteur (pg local)
    my_addr.sin_family = AF_INET;                // famille d'adresse
    my_addr.sin_port = htons(atoi(argv[1]));     // recuperation du port du recepteur
    inet_aton("127.0.0.1", &(my_addr.sin_addr)); // adresse IPv4 du recepteur

    // bind: association de la socket et des param reseaux du recepteur
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) != 0)
    {
        perror("erreur lors de l'appel a bind -> ");
        exit(-2);
    }

    // listen
    if (listen(sockfd, 5) == -1)
    {
        perror("erreur d'ecoute -> ");
        exit(-3);
    }

    sin_size = sizeof(struct sockaddr_in);
    // on traitera la connexion avec new_fd
    if ((new_fd = accept(sockfd, (struct sockaddr *)&client, &sin_size)) == -1)
    {
        perror("erreur d'ecoute -> ");
        exit(-3);
    }

    
    int players;// reception de la chaine de caracteres
    if (recvfrom(new_fd, &players, sizeof(int), 0, (struct sockaddr *)&client, &fromlen) == -1)
    {
        perror("erreur de reception -> ");
        exit(-3);
    }

    // affichage de la chaine de caracteres recue
    
    printf("Connexion with: %d, port no: %d, succesful, message received: %d \n", client.sin_addr.s_addr, client.sin_port, players);

    int player_id[players];
    for (size_t i = 0; i < players; i++)
    {
        int already_in_use;
        do
        {
            player_id[i] = rand() % NB_COLORS;
            already_in_use = 0;

            for (size_t j = 0; j < i; j++)
                already_in_use = already_in_use || (player_id[i] == player_id[j]);
        } while (already_in_use);
        
        
        if (sendto(new_fd, &(player_id[i]), 4, 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
        {
            perror("erreur a l'appel de la fonction sendto -> ");
            exit(-2);
        }
    }

    display_info display_struct;
    display_struct.winner = -1;
    struct client_input client_in;
    int pid = fork();

    if (pid == -1)
    {
        perror("erreur a l'appel de la fonction fork -> ");
        exit(-2);
    }
    else if (pid == 0)
    {
        // enfant
        while (1)
        {
            memset(buf, 0, 1024);
            if (recvfrom(new_fd, &client_in, sizeof(client_in), 0, (struct sockaddr *)&client, &fromlen) < 1)
            {
                perror("erreur de reception -> ");
                exit(-3);
            }

            printf("in server_recv : input : %d from player no : %d \n", (int)client_in.input, client_in.id);
        }
    }
    else
    {
        int temp = 23;
        // parent
        while (1)
        {
            //generate_random_board(display_struct.board);
            init_board(display_struct.board);
            display_struct.board[temp][temp] = rand() % NB_COLORS;
            temp++;

            if (sendto(new_fd, &display_struct, sizeof(display_struct), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
            {
                perror("erreur a l'appel de la fonction sendto -> ");
                pid_t raison;
                if (wait(&raison) == -1)
                    perror("wait");
                exit(-2);
            }

            sleep(2);
            //display_struct.winner = TIE;
        }
    }

    // fermeture de la socket
    close(new_fd);
    close(sockfd);

    return 0;
}
