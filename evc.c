#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define CHECK_ERROR(val1, val2, msg) if (val1 == val2) { printf("%s \n", msg); exit(EXIT_FAILURE); }
#define MAXCARS 80

//train:command:option
//ter500:reg/for/unr:./52/.

// Déclaration des fonctions
int create_socket();
void set_socket_timeout(int sd, int sec, int usec);
void configure_address(struct sockaddr_in *addr, const char *ip, u_int16_t port);
void send_message(int sd, const char *train_name, const char *message, struct sockaddr_in *addr_rbc);
int receive_message(int sd, char *recept, struct sockaddr_in *addr_rbc);

int main(int argc, char **argv) {
    int sd;
    struct sockaddr_in addr_evc, addr_rbc;
    int longaddr = sizeof(struct sockaddr_in);
    char train_name[7];
    char recept[MAXCARS + 1]; //buffer de reception
    char result[MAXCARS + 1];  // buffer d'emission commande:option

    // Vérification des arguments
    if (argc < 4) {
        printf("Usage: %s <train name> <rbc ip address> <rbc port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialisation
    strcpy(train_name, argv[1]);
    sd = create_socket();
    configure_address(&addr_evc, "0.0.0.0", 0); // Adresse locale 
    configure_address(&addr_rbc, argv[2], htons(atoi(argv[3])));
    //printf("Adresse RBC : %s\n", inet_ntoa(addr_rbc.sin_addr));

    // Bind de la socket
    CHECK_ERROR(bind(sd, (struct sockaddr *)&addr_evc, sizeof(addr_evc)), -1, "Bind failed");

    // Configuration du timeout
    set_socket_timeout(sd, 1, 500000); // Timeout : 1,5 secondes

    // Boucle principale
    do {
        printf("EVC> ");
        fgets(result, MAXCARS, stdin);
        result[strlen(result) - 1] = '\0'; // Suppression du retour chariot

        send_message(sd, train_name, result, &addr_rbc);
        int nbcar = receive_message(sd, recept, &addr_rbc);

        if (nbcar > 0) {
            printf("RBC IP:%s> '%s'\n", inet_ntoa(addr_rbc.sin_addr), recept);
        }
    } while (strcmp(recept, "end"));

    // Fermeture
    CHECK_ERROR(close(sd), -1, "Erreur lors de la fermeture de la socket");
    return EXIT_SUCCESS;
}

// Création de la socket UDP
int create_socket() {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERROR(sd, -1, "La création de la socket a échoué");
    return sd;
}

// Configuration du timeout pour recvfrom
void set_socket_timeout(int sd, int sec, int usec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    int code_erreur = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    CHECK_ERROR(code_erreur, -1, "Erreur setsockopt pour timeout");
}

// Configuration d'une adresse socket
void configure_address(struct sockaddr_in *addr, const char *ip, u_int16_t port) {
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = port;
}

// Envoi d'un message à la RBC
void send_message(int sd, const char *train_name, const char *message, struct sockaddr_in *addr_rbc) {
    char *emiss; // buffer d'emission train:commande:option
    size_t length = strlen(train_name) + strlen(message) + 2;

    emiss = malloc(length * sizeof(char));
    CHECK_ERROR(emiss, NULL, "malloc échoué");

    strcpy(emiss, train_name);
    strcat(emiss, ":");
    strcat(emiss, message);

    int nbcar = sendto(sd, emiss, strlen(emiss) + 1, 0, (struct sockaddr *)addr_rbc, sizeof(struct sockaddr_in));
    if (nbcar < 0) {
        perror("Erreur lors de l'envoi au RBC");
    }

    free(emiss);
}

// Réception d'un message de la RBC avec gestion du timeout
int receive_message(int sd, char *recept, struct sockaddr_in *addr_rbc) {
    int longaddr = sizeof(struct sockaddr_in);
    int nbcar = recvfrom(sd, recept, MAXCARS + 1, 0, (struct sockaddr *)addr_rbc, &longaddr);

    if (nbcar < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Délai d'attente du RBC dépassé\n");
        } else {
            perror("Erreur lors de la réception RBC");
        }
    } else {
        recept[nbcar] = '\0'; // Terminer la chaîne reçue
    }

    return nbcar;
}
