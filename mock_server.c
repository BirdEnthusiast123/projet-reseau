// Serveur : socket, bind, listen, accept, send/receive, close
// Client  : socket, (bind), connect, send/receive, close

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

void generate_random_board(char board[XMAX][YMAX])
{
    for (size_t i = 0; i < XMAX; i++)
    {
        for (size_t j = 0; j < YMAX; j++)
        {
            board[i][j] = (rand() % NB_COLORS);
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

    if(players < 2)
    {
        int tmp = 1;
        if (sendto(new_fd, &tmp, 4, 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
        {
            perror("erreur a l'appel de la fonction sendto -> ");
            exit(-2);
        }
    }

    char board[XMAX][YMAX];
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

            printf("in server_recv : input : %c from player no : %d \n", client_in.input, client_in.id);
        }
    }
    else
    {
        // parent
        while (1)
        {
            generate_random_board(board);

            if (sendto(new_fd, board, XMAX * YMAX, 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
            {
                perror("erreur a l'appel de la fonction sendto -> ");
                pid_t raison;
                if (wait(&raison) == -1)
                    perror("wait");
                exit(-2);
            }

            sleep(2);
        }
    }

    // fermeture de la socket
    close(new_fd);
    close(sockfd);

    return 0;
}
