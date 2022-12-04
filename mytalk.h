#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pwd.h>
#include <sys/select.h>
#define DEFAULT_BACK_LOG 100
#define MESG "Hi, TCP Client.\n"
#define MESGC "Hi, TCP Server.\n"
#define END "\nConnection closed. ^C to terminate.\n"
#define MAXLEN 2
#define UIDMAXLEN 1000
#define CLIENTV "VERB: Waiting for connection..."
#define SERVERV "Waiting for response from "
#define MAXLINE 80
#define SERV 1
#define CLIENT 0


int handle_args(int *v, int *a, int *n, int argc, char *argv[], int *hostname, int *portnum);
void print_usage_and_exit();

void handle_client(char *argv[], int hostname, int portnum, int v, int a, int n);
void handle_server(char *argv[], int portnum, int v, int a, int n);
void print_options(int v, int a, int n, int serv, char *argv[], int hostname);
void start_windowing(void);
void update_input_buffer(void);
void stop_windowing(void); 
int read_from_input(char *buf, size_t len);
int write_to_output(const char *buf, size_t len); 
int has_whole_line(void); 
int has_hit_eof(void); 
int set_verbosity(int level); 
void chat_client(int sockfd, int v, int n);  
