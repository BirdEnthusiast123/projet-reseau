#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>

#include <ncurses.h>

#include "common.h"

void tune_terminal()
{
    struct termios term;
    tcgetattr(0, &term);
    term.c_iflag &= ~ICANON;
    term.c_lflag &= ~ICANON;
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &term);
}

void init_graphics()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    start_color();
    init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
    init_pair(MAGENTA_ON_BLACK, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

    init_pair(BLUE_ON_BLUE, COLOR_BLUE, COLOR_BLUE);
    init_pair(RED_ON_RED, COLOR_RED, COLOR_RED);
    init_pair(YELLOW_ON_YELLOW, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(MAGENTA_ON_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(CYAN_ON_CYAN, COLOR_CYAN, COLOR_CYAN);

    init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
}

void display_character(int color, int y, int x, char character)
{
    attron(COLOR_PAIR(color));
    mvaddch(y, x, character);
    attroff(COLOR_PAIR(color));
}

// TODO: ajouter les conventions par exemple: moto := '8', mur := ACS_VLINE
void print_game(char board[XMAX][YMAX])
{
    for (size_t x = 0; x < XMAX; x++)
        for (size_t y = 0; y < YMAX; y++)
            display_character(board[x][y], y, x, 'X');
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: ./client [IP_serveur] [port_serveur] [nb_joueurs]\n");
        exit(EXIT_FAILURE);
    }

    // Récupération des arguments
    char *arg_addr = argv[1];
    int arg_port = atoi(argv[2]);
    struct client_init_infos client_info;
    client_info.nb_players = atoi(argv[3]);

    // Buffers de réception et d'envoi
    struct client_input client_input;
    display_info display;

    // Paramètres de connexion
    // Socket
    int sockfd;
    CHECK((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != -1);

    // Connect
    struct sockaddr_in server_struct;
    unsigned int sizeof_struct = sizeof(server_struct);

    server_struct.sin_family = AF_INET;
    server_struct.sin_port = htons(arg_port);
    server_struct.sin_addr.s_addr = inet_addr(arg_addr);
    memset(&(server_struct.sin_zero), '\0', 8);

    CHECK(connect(sockfd, (struct sockaddr *)&server_struct, sizeof_struct) != -1);

    // tune_terminal -> les inputs sont détectés directement
    // plus besoin de la touche 'entrée'
    tune_terminal();
    init_graphics();

    // Envoi du nombre de joueurs au server
    CHECK(
        sendto(
            sockfd, 
            &(client_info.nb_players), 
            sizeof(client_info.nb_players), 
            0, 
            (struct sockaddr *)&server_struct, 
            sizeof_struct
        ) 
        > 0
    );

    // Réception de l'id que nous donne le server
    if(client_info.nb_players < 2)
    {
        CHECK(
            recvfrom(
                sockfd, 
                &(client_input.id), 
                sizeof(client_input.id), 
                0, 
                (struct sockaddr *)&server_struct, 
                &sizeof_struct
            ) 
            > 0
        );
    }

    // Initialisation 'select'
    fd_set ens, tmp;
    FD_ZERO(&ens);
    FD_SET(sockfd, &ens);
    FD_SET(STDIN_FILENO, &ens);
    tmp = ens;
    while (1)
    {
        CHECK(select(sockfd + 1, &tmp, NULL, NULL, NULL) != -1);

        // Gestion d'envoi d'inputs du client au server
        if (FD_ISSET(STDIN_FILENO, &tmp))
        {
            CHECK(read(STDIN_FILENO, &(client_input.input), sizeof(client_input.input)) > 0);

            CHECK(
                sendto(
                    sockfd, 
                    &client_input, 
                    sizeof(client_input), 
                    0, 
                    (struct sockaddr *)&server_struct, 
                    sizeof_struct
                ) 
                > 0
            );
        }

        // Réception et affichage du board envoyé par le serveur
        if (FD_ISSET(sockfd, &tmp))
        {
            // TODO: Peut etre tester (recvfrom(...) == XMAX * YMAX) si faux alors pertes/manque
            CHECK(
                recvfrom(
                    sockfd, 
                    display.board, 
                    XMAX * YMAX, 
                    0, 
                    (struct sockaddr *)&server_struct, 
                    &sizeof_struct
                ) 
                > 0
            );
            clear();
            print_game(display.board);
            refresh();
        }

        // Réinitialiser le mask de select
        tmp = ens;
    }

    return 0;
}