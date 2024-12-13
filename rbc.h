#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>

#define CHECK_ERROR(val1,val2,msg)   if (val1==val2) \
                                    { perror(msg); \
                                        exit(EXIT_FAILURE); }

#define MAXOCTETS 150
#define MAXTRAINS 100
#define TGVSPEED 50
#define TERSPEED 25
#define INTSPEED 10

typedef struct {
  int location;
  char name[7];
  int EOA;
  int speed;
} Train;

int sd;
int nb_trains = 0;
Train list_trains[MAXTRAINS];

void init_list_train(Train *list_trains, int nbtrains);
static void signalHandler(int num_sig);
void init_socket(int *sd, struct sockaddr_in addr_serv, u_int16_t port, char *local_ip);
void handle_client();
char *handle_message(char *buff_reception);
char **parse_message(char *buffer);
int handle_register(char *train_name);
int insert_in_Ltrain(char *train_name);
void pop_from_Ltrain();
int forward(char *train_name, int canton);
int find(char *train_name);
int speed(char *train_name);
int unregister(char *train_name);
