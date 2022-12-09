#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <semaphore.h>

#include "common.h"

// macro ajoutées
#define RES_LOCAL 1
#define RES_INTERNET 2

#define NB_CLIENT_MAX 2
#define BUF_MAX_SIZE 64

#define THRD_RESTART 101
#define THRD_QUIT 102

#define GAME_OVER (THRD_RESTART | THRD_QUIT)
#define GAME_ONGOING ~(GAME_OVER)

#define EMPTY_SQUARE -1
#define NO_WINNER_YET -1
#define TRAIL_DOWN ~(TRAIL_UP)

#define TCHK(x) \
  do { \
    errno = x; \
    if (errno != 0) { \
      fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
      perror(#x); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)


typedef struct{
    int sockfd, network_type;
    int* client_fd;
    struct sockaddr_in* clients;
    struct client_input* client_input;
    pthread_mutex_t * crit;
    int* status;
    sem_t * sem_command;
    int* player_wall;
} thread_struct;

void init_board(display_info * disp_struct, struct client_input* client_struct)
{
    disp_struct->winner = NO_WINNER_YET;

    // empty the grid
    memset(disp_struct->board, EMPTY_SQUARE, XMAX * YMAX);

    // fill borders
    for (size_t i = 0; i < XMAX; i++)
    {
        disp_struct->board[i][0] = WALL;
        disp_struct->board[i][YMAX - 1] = WALL;
    }
    for (size_t i = 0; i < YMAX; i++)
    {
        disp_struct->board[0][i] = WALL;
        disp_struct->board[XMAX - 1][i] = WALL;
    }
    
    // init players
    disp_struct->board[(XMAX / 2) - 3][YMAX / 2] = client_struct[0].id;
    client_struct[0].input = UP;
    disp_struct->board[(XMAX / 2) + 3][YMAX / 2] = client_struct[1].id;
    client_struct[1].input = DOWN;
}

int max_fd(int *arr, int arr_size, int sockfd)
{
    int max = arr[0];
    for (int i = 0; i < arr_size; i++)
        max = (max < arr[i]) ? arr[i] : max;

    return (max > sockfd) ? max : sockfd;
}

void* thread_fct (void* thread_arg)
{
    thread_struct* tmp_thrd_arg = thread_arg;

    fd_set ens, tmp;
    FD_ZERO(&ens);
    FD_SET(STDIN_FILENO, &ens);
    FD_SET(tmp_thrd_arg->sockfd, &ens);
    FD_SET(tmp_thrd_arg->client_fd[0], &ens);
    FD_SET(tmp_thrd_arg->client_fd[1], &ens);
    tmp = ens;

    while (1)
    {
        // Attente bloquante d'inputs de client
        CHECK(
            select(
                max_fd(
                    tmp_thrd_arg->client_fd, 
                    tmp_thrd_arg->network_type, 
                    tmp_thrd_arg->sockfd
                ) + 1, 
                &tmp, NULL, NULL, NULL
            )
            != -1
        );

        // message receiving
        for (int i = 0; i < tmp_thrd_arg->network_type; i++)
        {
            if (FD_ISSET(tmp_thrd_arg->client_fd[i], &tmp))
            {
                // Le client numéro i a envoyé un input
                unsigned int sin_size = sizeof(struct sockaddr_in);

                // Cas : 2 sur une machine, l'id du client est dans son msg
                if(tmp_thrd_arg->network_type == RES_LOCAL)
                {
                    struct client_input tmp;
                    CHECK(
                        recvfrom(
                            tmp_thrd_arg->client_fd[i], 
                            &tmp, 
                            sizeof(struct client_input), 
                            0, 
                            (struct sockaddr *)(&(tmp_thrd_arg->clients[i])), 
                            &sin_size
                        )
                        > 0
                    );

                    int mem_input = tmp_thrd_arg->client_input[tmp.id].input;
                    TCHK(pthread_mutex_lock(tmp_thrd_arg->crit));
                    tmp_thrd_arg->client_input[tmp.id].input = tmp.input;
                    TCHK(pthread_mutex_unlock(tmp_thrd_arg->crit));

                    if(tmp.input == TRAIL_UP)
                    {
                        tmp_thrd_arg->player_wall[tmp.id] = ~(tmp_thrd_arg->player_wall[tmp.id]);
                        tmp_thrd_arg->client_input[tmp.id].input = mem_input;
                    }   
                }
                // Cas : 1 client par machine, l'id du client == son fd
                else if(tmp_thrd_arg->network_type == RES_INTERNET)
                {    
                    int tmp_input = tmp_thrd_arg->client_input[i].input;
                    TCHK(pthread_mutex_lock(tmp_thrd_arg->crit));
                    CHECK(
                        recvfrom(
                            tmp_thrd_arg->client_fd[i], 
                            &(tmp_thrd_arg->client_input[i]), 
                            sizeof(struct client_input), 
                            0, 
                            (struct sockaddr *)(&(tmp_thrd_arg->clients[i])), 
                            &sin_size
                        )
                        > 0
                    );
                    TCHK(pthread_mutex_unlock(tmp_thrd_arg->crit));

                    // gestion murs de lumiere
                    if (tmp_thrd_arg->client_input[i].input == TRAIL_UP)
                    {
                        tmp_thrd_arg->player_wall[i] = ~(tmp_thrd_arg->player_wall[i]);
                        tmp_thrd_arg->client_input[i].input = tmp_input;
                    }                
                }

            }
        }

        if (FD_ISSET(STDIN_FILENO, &tmp))
        {
            char buf_char[BUF_MAX_SIZE];
            int nb_char;
            CHECK((nb_char = read(STDIN_FILENO, buf_char, BUF_MAX_SIZE)) > 0);
            buf_char[nb_char - 1] = '\0';

            if(strcmp(buf_char, "restart") == 0)
            {
                *(tmp_thrd_arg->status) = THRD_RESTART;
                TCHK(sem_post(tmp_thrd_arg->sem_command));
            }
            else if(strcmp(buf_char, "quit") == 0)
            {
                *(tmp_thrd_arg->status) = THRD_QUIT;
                TCHK(sem_post(tmp_thrd_arg->sem_command));
                return NULL;
            }
        }

        tmp = ens;
    }
}

void update_board
(
    display_info * disp, 
    struct client_input * c_in, 
    pthread_mutex_t * mut_crit, 
    int player_pos[NB_CLIENT_MAX][2], 
    int player_wall[NB_CLIENT_MAX]
)
{
    int player_i_lost[NB_CLIENT_MAX] = {0, 0};
    TCHK(pthread_mutex_lock(mut_crit));

    for (int i = 0; i < NB_CLIENT_MAX; i++)
    {

        if(player_wall[i] == TRAIL_UP)
        {
            disp->board[player_pos[i][0]][player_pos[i][1]] = i + TRAIL_INDEX_SHIFT;
        }
        else if(player_wall[i] == TRAIL_DOWN)
        {
            disp->board[player_pos[i][0]][player_pos[i][1]] = EMPTY_SQUARE;
        }

        switch (c_in[i].input)
        {
        case UP:
            player_pos[i][1] -= 1;
            break;
        case DOWN:
            player_pos[i][1] += 1;
            break;
        case LEFT:
            player_pos[i][0] -= 1;
            break;
        case RIGHT:
            player_pos[i][0] += 1;
            break;
        default:
            break;
        }

        if(disp->board[player_pos[i][0]][player_pos[i][1]] != EMPTY_SQUARE)
        {
            player_i_lost[i] = 1;
        }
        else
        {
            disp->board[player_pos[i][0]][player_pos[i][1]] = i;
        }

    }

    if((player_i_lost[0] == 1) && (player_i_lost[1] == 1))
        disp->winner = TIE;
    else if(player_i_lost[0] == 1)
        disp->winner = 1;
    else if(player_i_lost[1] == 1)
        disp->winner = 0;

    TCHK(pthread_mutex_unlock(mut_crit));
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
    int u_refresh_rate = 1000 * refresh_rate; // pour usleep()

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
    char str_ip_addr[64];
    CHECK(inet_ntop(AF_INET, &(my_addr.sin_addr), str_ip_addr, sizeof(str_ip_addr)) != NULL);
    // retourne 0.0.0.0 pour des raisons qui m'echappent mais le client 
    // peut utiliser 0.0.0.0 comme argument (sur turing.unistra au moins)
    printf("Bind successful. IP @: %s, port no: %d\n", str_ip_addr, SERV_PORT);

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

    // Initalisation argument thread
    thread_struct str_thread_arg;
    str_thread_arg.sockfd = sockfd;
    str_thread_arg.network_type = res_type;
    str_thread_arg.clients = clients;
    str_thread_arg.client_fd = client_fd;

    struct client_input client_in[2];
    str_thread_arg.client_input = client_in;

    pthread_mutex_t mutex_crit;
    TCHK(pthread_mutex_init(&mutex_crit, 0));
    str_thread_arg.crit = &mutex_crit;

    int game_status = GAME_ONGOING;
    str_thread_arg.status = &game_status;

    sem_t sem_wait_command;
    TCHK(sem_init(&sem_wait_command, 0, 0));
    str_thread_arg.sem_command = &sem_wait_command;

    int player_wall[NB_CLIENT_MAX] = {TRAIL_UP, TRAIL_UP};
    str_thread_arg.player_wall = player_wall;

    // thread s'occupe de la partie reception
    pthread_t id;
    TCHK(pthread_create(&id, NULL, thread_fct, &str_thread_arg));

    // main s'occupe de rafraichir et envoyer la grille 
    display_info display_struct;
    do
    {
        // grille
        init_board(&display_struct, client_in);
        game_status = GAME_ONGOING;
        
        // joueurs
        player_wall[1] = TRAIL_UP;
        player_wall[0] = TRAIL_UP;
        int player_pos[NB_CLIENT_MAX][2] = {{(XMAX / 2) - 3, YMAX / 2}, {(XMAX / 2) + 3, YMAX / 2}};

        while (display_struct.winner == NO_WINNER_YET)
        {
            usleep(u_refresh_rate);
            update_board(&display_struct, client_in, &mutex_crit, player_pos, player_wall);
            for (int i = 0; i < nb_of_machines; i++)
            {            
                CHECK(
                    sendto(
                        client_fd[i],
                        &(display_struct),
                        sizeof(display_struct),
                        0,
                        (struct sockaddr *)& (clients[i]),
                        sin_size
                    ) 
                    > 0
                );
            }

            if((game_status & GAME_ONGOING) == 0)
                break;
        }

        // attente de l'entree serveur 'restart' ou 'quit'
        TCHK(sem_wait(&sem_wait_command));
    } while (game_status != THRD_QUIT);

    TCHK(pthread_join(id, NULL));
    TCHK(pthread_mutex_destroy(&mutex_crit));
    TCHK(sem_destroy(&sem_wait_command));
    
    // fermeture de la socket
    close(client_fd[0]);
    close(client_fd[1]);
    close(sockfd);

    return 0;
}
