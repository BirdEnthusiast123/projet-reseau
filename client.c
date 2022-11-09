#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <ncurses.h>

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

    init_pair(EMPTY_SQUARE, COLOR_BLACK, COLOR_BLACK);
    init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
}

void display_character(int color, int y, int x, char character)
{
    attron(COLOR_PAIR(color));
    mvaddch(y, x, character);
    attroff(COLOR_PAIR(color));
}

// Traduction d'un char (input clavier) en macro lisible par le server 
int prep_send_macro(struct client_input *c_input, char input, int player_count)
{
    char macros[6] = {UP, LEFT, DOWN, RIGHT, TRAIL_UP, '\0'};
    char tmp_str1[6] = "zqsd ";
    char* tmp_ptr = strchr(tmp_str1, input);
    if(tmp_ptr != NULL)
    {
        // strchr renvoie un pointeur sur premiere occurrence de input dans tmp_str
        // (tmp_ptr - tmp_str) est l'index de la premiere occurrence
        // la liste macros a ete construite telle que tmp_str[i] <=> macros[i]
        c_input[0].input = macros[tmp_ptr - tmp_str1];
        return 0;
    }
    
    char tmp_str2[6] = "ijklm";
    tmp_ptr = strchr(tmp_str2, input);
    if((tmp_ptr != NULL) && (player_count == 2))
    {
        c_input[1].input = macros[tmp_ptr - tmp_str2];
        return 1;
    }

    // input invalide
    return -1;
}

// Renvoie le char associée à la valeur d'une case de la grille
// Si la valeur n'est pas reconnue -> affiche un mûr
char map_board_to_char(char board_value)
{
    char res;
    if((board_value == EMPTY_SQUARE) || (board_value == WALL))
        res = ACS_VLINE;
    else if((board_value >= BLUE_ON_BLUE) && (board_value <= CYAN_ON_CYAN))
        res = ACS_VLINE;
    else if((board_value >= BLUE_ON_BLACK) && (board_value <= CYAN_ON_BLACK))
        res = '8';
    else
    {
        res = WALL;
        // char non reconnu, lancer erreur ?
    }

    return res;
}   

void print_game(char board[XMAX][YMAX])
{
    char tmp;
    for (size_t x = 0; x < XMAX; x++)
    {
        for (size_t y = 0; y < YMAX; y++)
        {
            tmp = board[x][y];
            display_character(tmp, y, x, map_board_to_char(tmp));
        }
    }
    mvaddstr(0, XMAX/2 - strlen("C-TRON")/2, "C-TRON");
}

void game_over_display(int winner_id, struct client_input* p_strct_cli_input, int player_count)
{
    if(winner_id == TIE)
    {
        // nobody won
        mvaddstr(YMAX/2, XMAX/2, "Nobody won, its a tie!");
    }
    else if(player_count > 1)
    {
        for (int i = 0; i < player_count; i++)
        {
            if (winner_id == p_strct_cli_input[i].id)
            {
                mvaddstr(YMAX/2, XMAX/2, "Player who won is none other than:");
                char winner[2];
                snprintf(winner, 2, "%d", winner_id);
                mvaddstr((YMAX/2) + 1, XMAX/2, winner);                
            }   
        }
    }
    else
    {
        // player count == 1
        if(winner_id == p_strct_cli_input[0].id)
        {
            // local player won
            mvaddstr(YMAX/2, XMAX/2, "You won, great job!");
        }
        else
        {
            // distant player won
            mvaddstr(YMAX/2, XMAX/2, "You lost, better luck next time!");
        }
    }

    refresh();

    sleep(10);
    exit(EXIT_SUCCESS);
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
    struct client_input client_input[client_info.nb_players];
    display_info display_struct;

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
    for (int i = 0; i < client_info.nb_players; i++)
    {
        CHECK(
            recvfrom(
                sockfd,
                &(client_input[i].id),
                sizeof(client_input[i].id),
                0,
                (struct sockaddr *)&server_struct,
                &sizeof_struct
            ) 
            > 0
        );
    }

    // tune_terminal -> les inputs sont détectés directement
    // plus besoin de la touche 'entrée'
    tune_terminal();
    init_graphics();

    // Attente d'un.e autre joueur.euse distant.e
    if(client_info.nb_players < 2)
    {
        clear();
        mvaddstr(YMAX/2, XMAX/2, "Waiting for players (1/2) ...");
        refresh();
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
            // TODO: traduire char -> MACRO, et n'envoyer au serveur que si input valide
            char c_buf;
            CHECK(read(STDIN_FILENO, &c_buf, sizeof(c_buf)) > 0);

            int sender_id = prep_send_macro(client_input, c_buf, client_info.nb_players);
            if(sender_id != -1)
            {
                CHECK(
                    sendto(
                        sockfd,
                        &(client_input[sender_id]),
                        sizeof(client_input[sender_id]),
                        0,
                        (struct sockaddr *)&server_struct,
                        sizeof_struct
                    ) 
                    > 0
                );
            }
        }

        // Réception et affichage du board envoyé par le serveur
        if (FD_ISSET(sockfd, &tmp))
        {
            // TODO: Peut etre tester (recvfrom(...) == XMAX * YMAX) si faux alors pertes/manque
            CHECK(
                recvfrom(
                    sockfd,
                    &display_struct,
                    sizeof(display_struct),
                    0,
                    (struct sockaddr *)&server_struct,
                    &sizeof_struct
                ) 
                > 0
            );
            if (display_struct.winner != NO_WINNER_YET)
            {
                game_over_display(display_struct.winner, client_input, client_info.nb_players);
            }
            else
            {
                clear();
                print_game(display_struct.board);
                refresh();
            }
        }

        // Réinitialiser le mask de select
        tmp = ens;
    }

    return 0;
}