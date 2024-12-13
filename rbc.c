#include "rbc.h"

// message; nom_du_train:commande:option

int main(int argc, char *argv[]) {
    struct sockaddr_in addr_serv;

    if (argc != 3){
        printf("Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    u_int16_t port = atoi(argv[2]);

    init_list_train(list_trains, MAXTRAINS);

    init_socket(&sd, addr_serv, port, argv[1]);

    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_handler = signalHandler;
    new_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &new_action, NULL);
    signal(SIGINT, signalHandler);

    while (1){
        handle_client();
    }

    exit(EXIT_SUCCESS);
}

void init_list_train(Train *list_trains, int nbtrains){
    for (int i=0; i<nbtrains; i++){
        list_trains[i].location = 0; // Initialisation des localisations à 0 (pas de train)
        list_trains[i].EOA = 0;
        list_trains[i].speed = 0;
        strcpy(list_trains[i].name, "");
        list_trains[i].name[6] = '\0'; // Le dernier caractère est le caractère nul
    }
}

void init_socket(int *sd, struct sockaddr_in addr_serv, u_int16_t port, char *local_ip){
    socklen_t addr_serv_len = sizeof(addr_serv);
    
    *sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERROR(*sd, -1, "Erreur socket non cree !!! \n");
    
    //preparation de l'adresse de la socket
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(port);
    addr_serv.sin_addr.s_addr = inet_addr(local_ip);
    
    //Affectation d'une adresse a la socket
    int erreur = bind(*sd, (const struct sockaddr *) &addr_serv, addr_serv_len);
    CHECK_ERROR(erreur, -1, "Erreur de bind !!! \n");
    printf("Waiting for request...\n");
}

void handle_client(){
    char buff_emission[MAXOCTETS+1];
    char buff_reception[MAXOCTETS+1];
    struct sockaddr_in addr_client;
    socklen_t addr_client_len = sizeof(addr_client);
    int nb_car;

    nb_car = recvfrom(sd, buff_reception, MAXOCTETS+1, 0, (struct sockaddr *) &addr_client, &addr_client_len);
    printf("message received: %s\n", buff_reception);
    CHECK_ERROR(nb_car, 0, "\nProbleme de reception !!! \n");

    strcpy(buff_emission, handle_message(buff_reception));
    
    nb_car = sendto(sd, buff_emission, strlen(buff_emission)+1, 0, (struct sockaddr *) &addr_client, addr_client_len);
    CHECK_ERROR(nb_car, 0, "\nProbleme d'emission !!! \n");
    printf("message sent: %s\n", buff_emission);

}

char *handle_message(char *buff_reception){
    char ** parsed_message = parse_message(buff_reception);

    if (parsed_message[0]==NULL || parsed_message[1]==NULL){
        return "format error";
    }
    
    char *train_name = parsed_message[0];
    char *command = parsed_message[1];
    char *option = parsed_message[2];
    
    int result;

    // register handling
    if (strcmp(command, "reg") == 0) {
        result = handle_register(train_name);
        if (result == 1){
            return "already registered";
        }
        else if (result == 0) {
            return "ack";
        }
        else {
            return "first canton occupied";
        }
    }
    // forward handling
    else if (strcmp(command, "for") == 0) {

        int option_int = atoi(option);
        if (option_int == 0){
            return "canton not recognized";
        }

        result = forward(train_name, option_int);
        if (result == -1){
            return "not registered yet";
        }
        else if (result == 1){
            static char response[50];
            sprintf(response, "forward to canton %d impossible", option_int);
            return response;
        }
        else {
            return "ack";
        }
    }
    else if (strcmp(command, "unr") == 0){
        result = unregister(train_name);
        if (result == -1){
            return "not registered yet";
        }
        else if (result == 1){
            return "train not in last canton";
        }
        else {
            return "end";
        }
    }
    else if (strcmp(command,"inf") == 0){
        int train_index = find(train_name);
        if (train_index == -1){
            return "not registered yet";
        }
        static char response[50];
        sprintf(response, "train %s is in canton %d with EOA %d", train_name, list_trains[train_index].location, list_trains[train_index].EOA);
        return response;
    }
    
    // unkwown command
    else {
        return "command not recognized";
    }
}

int handle_register(char *train_name){
    // handle register command

    if (find(train_name) != -1){ // Train already registered
        return 1;
    }
    return insert_in_Ltrain(train_name);
}

int forward(char *train_name, int canton){
    /*  
    Return -1 if the train is not in the list
    Return 0 if the train is in the list
    Return 1 if forward not authorized

    Move train to the next canton and update the EOA of the next train
    */

    int train_index = find(train_name);

    if (train_index == -1){
        return -1;
    }
    if ((canton > list_trains[train_index].EOA) || (canton <= list_trains[train_index].location) || canton - list_trains[train_index].location > list_trains[train_index].speed){
        return 1;
    }

    list_trains[train_index].location = canton;
    if (train_index + 1 < nb_trains){
        list_trains[train_index + 1].EOA = canton - 1;
    }
    return 0;

}

int unregister(char *train_name){
    // Unregister train
    int train_index = find(train_name);
    if (train_index == -1){ // train not found
        return -1;
    }
    if (list_trains[train_index].location != MAXTRAINS){ // not in the last canton
        return 1;
    }
    pop_from_Ltrain();
    return 0;
}

int find(char *train_name){
    for (int i=0; i<nb_trains; i++){
        if (strcmp(list_trains[i].name, train_name) == 0){
            return i;
        }
    }
    return -1;
}

int speed(char *train_name){
    if (strncmp(train_name, "TGV", 3) == 0 || strncmp(train_name, "tgv", 3) == 0){
        return TGVSPEED;
    }
    else if (strncmp(train_name, "TER", 3) == 0 || strncmp(train_name, "ter", 3) == 0){
        return TERSPEED;
    }
    else if (strncmp(train_name, "INT", 3) == 0 || strncmp(train_name, "int", 3) == 0){
        return INTSPEED;
    }
    else {
        return INTSPEED;
    }
}

int insert_in_Ltrain(char *train_name){
    // Insert train in Ltrain
    if (nb_trains == 0){
        // Insert train in the first position
        list_trains[nb_trains].location = 0;
        list_trains[nb_trains].EOA = MAXTRAINS;
        list_trains[nb_trains].speed = speed(train_name);
        strcpy(list_trains[nb_trains].name, train_name);
    }
    // Insert train in the next position 
    else {
        // Check if the first canton is occupied
        if (list_trains[nb_trains-1].location == 0){
            return -1;
        }
        list_trains[nb_trains].location = 0;
        list_trains[nb_trains].EOA = list_trains[nb_trains-1].location-1;
        list_trains[nb_trains].speed = speed(train_name);
        strcpy(list_trains[nb_trains].name, train_name);
    }
    nb_trains++;
    return 0;
}

void pop_from_Ltrain(){
    // Pop train from Ltrain
    if (nb_trains != 1){
        list_trains[1].EOA = MAXTRAINS; // make the following train EOA the last canton
    }
    for (int i=1; i<nb_trains; i++){
        list_trains[i-1] = list_trains[i];
    }
    nb_trains--;
}

char **parse_message(char *buffer){
    char **token = (char **) malloc(3 * sizeof(char *));
    CHECK_ERROR(token, NULL, "Erreur lors de l'allocation de la mémoire pour le token");
    char *buff_copy = strdup(buffer);
    CHECK_ERROR(buff_copy, NULL, "Erreur lors de la duplication du buffer");

    // Extract train name
    token[0] = strtok(buff_copy, ":");

    // Extract command
    token[1] = strtok(NULL, ":");

    // Extract option if available
    token[2] = strtok(NULL, ":");
    if (token[2] == NULL) {
        token[2] = "";
    }

    return token;
}

static void signalHandler(int num_sig){
    switch (num_sig)
    {
    case SIGINT:
        CHECK_ERROR(close(sd), -1, "Erreur lors de la fermeture de la socket");
        printf("\n");
        exit(EXIT_SUCCESS);
        break;
    
    default:
        break;
    }
}
