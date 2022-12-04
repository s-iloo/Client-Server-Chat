#include "mytalk.h"

int main(int argc, char *argv[]){
  int v = 0, a = 0, n = 0, hostname = 0, portnum = 0, type = -1;
  
  /*handling arguments */
  type = handle_args(&v, &a, &n, argc, argv, &hostname, &portnum);
  if(!type){
    /*this means it is a client */
    handle_client(argv, hostname, portnum, v, a, n); 
  }else{
    /*this means it is a server */
    handle_server(argv, portnum, v, a, n); 
  }
  return type; 
}

void chat_client(int sockfd, int v, int n){
  int mlen, len; 
  fd_set readfds; 
  char buff[MAXLINE] = {0}; 
  /*only start windowing if now -n option */
  if(!n){
    start_windowing();
  }
  /*if(v){
    printf("verbosity\n"); 
    set_verbosity(5);
  } */
  do{
    /*initializes the fd set to contain no fd */
    FD_ZERO(&readfds);
    
    /*setting fd that we can get input from (stdin and socket)*/
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sockfd, &readfds);
    /*allows program to monitor multiple fd waiting until one or more 
     *become read*/ 
    if(-1 == select(sockfd + 1, &readfds, NULL, NULL, NULL)){
      perror("select\n"); 
      exit(EXIT_FAILURE); 
    } 
    /*if stdin is ready update input*/
    if(FD_ISSET(STDIN_FILENO, &readfds)){
      update_input_buffer();
      /*check if we have a whole line ready to read*/
      if(has_whole_line()){
        len = read_from_input(buff, sizeof(buff));
        if(len == -1){
          perror("read_from_input\n"); 
          exit(EXIT_FAILURE); 
        }
        /*send the message */ 
        mlen = send(sockfd, buff, len, 0);
        if(mlen == -1){
          perror("send\n"); 
          exit(EXIT_FAILURE); 
        } 
        memset(&buff, 0, sizeof(buff)); 
      }
    /*check if we've hit EOF */
    }
    if(has_hit_eof()){
      stop_windowing(); 
      close(sockfd); 
      exit(1);
    }
    /*if sockfd is ready we can recv data */    
    if(FD_ISSET(sockfd, &readfds)){
      mlen = recv(sockfd, buff, sizeof(buff), 0); 
      /*if mlen is 0 we've recv EOF */
      if(mlen == 0){
        write_to_output(END, sizeof(END)-1); 
        pause();
         
      }else{
        write_to_output(buff, mlen);
      }  
      memset(&buff, 0, sizeof(buff));
    }
  }while(1); 
}


void handle_client(char *argv[], int hostname, int portnum, int v, int a, 
  int n){
  int len;
  int sockfd;
  int userid; 
  struct sockaddr_in sa; 
  struct hostent *hostent; 
  const char *host = argv[hostname];
  char buff[MAXLEN+1] = {0};
  register uid_t uid = geteuid();
  register struct passwd *pw; 
 
  /*setting up the fd to connect with server*/
  hostent = gethostbyname(host); 
  if(NULL == hostent){
    perror("bad gethostbyname\n"); 
    exit(EXIT_FAILURE); 
  } 

  if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))){
    perror("bad socket\n");
    exit(EXIT_FAILURE); 
  } 
  sa.sin_family = AF_INET; 
  sa.sin_port = htons(atoi(argv[portnum]));
  sa.sin_addr.s_addr = *(uint32_t*)hostent->h_addr_list[0]; 
  
  if(v){
    print_options(v, a, n, CLIENT, argv, hostname); 
  }
  if(connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1){
    perror("bad connect\n");
    exit(EXIT_FAILURE);
  } 
  /*getting the username*/
  pw = getpwuid(uid);
  if(NULL == pw){
    perror("getpwuid\n");
    exit(EXIT_FAILURE); 
  }
  /*sending the username to the server*/ 
  userid = send(sockfd, pw -> pw_name, sizeof(pw->pw_name), 0); 
  if(userid == -1){
    perror("send\n"); 
    exit(EXIT_FAILURE); 
  } 
  /*recv ok or no */
  len = recv(sockfd, buff, sizeof(buff), 0);
  if(-1==len){
    perror("recv client\n");
    exit(EXIT_FAILURE); 
  }
  /*check if server is accepting connection*/
  if(!strcasecmp(buff,"ok")){
    memset(&buff, 0, sizeof(buff));
    chat_client(sockfd, v, n); 
  }else{
    printf("%s declined connection\n", argv[hostname]); 
  }
  close(sockfd); 
}

void print_options(int v, int a, int n, int serv, char *argv[], int hostname){
  printf("Options: \n");
  printf("  verbose: %d\n", v); 
  printf("  mode: %s\n", (serv)?"server": "client"); 
  printf("  accept: %d\n", a); 
  printf("  ncurse:  %d\n", n);
   
  if(serv){
    if(-1==write(STDOUT_FILENO, CLIENTV, strlen(CLIENTV))){
      perror("write\n"); 
      exit(EXIT_FAILURE); 
    }  
  }else{
    if(-1==write(STDOUT_FILENO, SERVERV, strlen(SERVERV))){
      perror("write\n"); 
      exit(EXIT_FAILURE); 
    } 
    printf("%s\n", argv[hostname]);
  }
}

void handle_server(char *argv[], int portnum, int v, int a, int n){
  int sockfd, mlen, mlenv2, newsockfd, clientinfo;
  struct sockaddr_in sa, newsockinfo, peerinfo;
  socklen_t len;
  char c[MAXLINE] = {0}; 
  char node[NI_MAXHOST] = {0};
  char localaddr[INET_ADDRSTRLEN], peeraddr[INET_ADDRSTRLEN], 
    buffuid[UIDMAXLEN + 1];
  /*create the socket and bind an address to it*/
  memset(&sa, 0, sizeof(sa)); 
  memset(&newsockinfo, 0, sizeof(newsockinfo)); 
  memset(&peerinfo, 0, sizeof(peerinfo));
 
  if(v){
    print_options(v, a, n, SERV, argv, 0);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    perror("bad socket!\n");
    exit(EXIT_FAILURE);
  }

  sa.sin_family = AF_INET; 
  sa.sin_port = htons(atoi(argv[portnum])); 
  sa.sin_addr.s_addr = htonl(INADDR_ANY); 
  if(-1 == bind(sockfd, (struct sockaddr *)&sa, sizeof(sa))){
    perror("bind: Address already in use\n");
    exit(EXIT_FAILURE);
  }
  if(-1 == listen(sockfd, DEFAULT_BACK_LOG)){
    perror("bad listen!\n");
    exit(EXIT_FAILURE);
  }
  len = sizeof(newsockinfo); 
 
  /*block until somebody connects, returns the new socket */
  /*gets the fd of the accepted socket */
  newsockfd = accept(sockfd, (struct sockaddr *)&peerinfo, &len); 
  if(newsockfd == -1){
    perror("bad accept\n"); 
    exit(EXIT_FAILURE); 
  }
  if(-1 == getsockname(newsockfd, (struct sockaddr *)&newsockinfo, &len)){
    perror("bad getsocketname\n"); 
    exit(EXIT_FAILURE);
  }
  /*convert the IPV4 address from binary to text*/
  if(inet_ntop(AF_INET, &newsockinfo.sin_addr.s_addr, localaddr, 
    sizeof(localaddr)) == NULL){
    perror("bad inet_ntop\n");
    exit(EXIT_FAILURE); 
  }
  if(inet_ntop(AF_INET, &peerinfo.sin_addr.s_addr, peeraddr, 
    sizeof(peeraddr)) == NULL){
    perror("bad inet_ntop 2\n"); 
    exit(EXIT_FAILURE); 
  }
  
  if(v){
    printf("New connection from: %s: %d\n", peeraddr, 
      ntohs(peerinfo.sin_port)); 
  }
  clientinfo = getnameinfo((struct sockaddr*)&peerinfo, sizeof(peerinfo),node,
    sizeof(node), NULL, 0, NI_NAMEREQD);
  if(clientinfo != 0){
    perror("getnameinfo\n");
    exit(EXIT_FAILURE); 
  }
  
  mlen = recv(newsockfd, buffuid, sizeof(buffuid), 0); 
  if(-1==mlen){
    perror("recv\n");
    exit(EXIT_FAILURE); 
  } 
  if(clientinfo){
    perror("getnameinfo\n");
    exit(EXIT_FAILURE); 
  }
  if(!a){
    printf("Mytalk request from %s@%s.  Accept (y/n)? ", buffuid, node);
    mlenv2 = read_from_input(c, sizeof(c));   
    if(mlenv2 == -1){
      perror("read\n");
      exit(EXIT_FAILURE); 
    }
  /*see what user inputted and then send ok if yes/y*/  
  if(!strcasecmp(c, "yes\n")||!strcasecmp(c, "y\n")){
      mlen = send(newsockfd, "ok", 2, 0); 
      if(mlen ==-1){
        perror("send\n"); 
        exit(EXIT_FAILURE); 
      }
      chat_client(newsockfd, v, n);
    }else{
      mlen = send(newsockfd, "no", 2, 0);
      if(mlen == -1){
        perror("send\n"); 
        exit(EXIT_FAILURE); 
      }
      if(v){
        printf("VERB: declining connection\n");
      }
      exit(1);
    }
  }else{
    /*don't ask to accept connection if -a option*/
    mlen = send(newsockfd, "ok", 2, 0); 
    if(-1==mlen){
      perror("send\n"); 
      exit(EXIT_FAILURE); 
    }
    chat_client(newsockfd, v, n); 
  }
  close(sockfd); close(newsockfd); 
}

int handle_args(int *v, int *a, int *n, int argc, char *argv[], int *hostname,
  int *portnum){

  int returned, i, j, nums=0; 	
  
  /*if returned is 1 its server else client */
  if(argc < 2 || argc > 6){
    print_usage_and_exit();
  }
  /*if there's 2 args then it is either a server or invalid*/ 
  if(argc == 2){
    *portnum = 1;   
    returned = 1;  
  }else{
    /*loop through the rest of args to see if options are specified*/
    for(i = 0; i < argc; i++){
      nums = 0; 
      if(!strcmp(argv[i],"-v")){
        *v = 1; 
      }else if(!strcmp(argv[i], "-a")){
        *a = 1; 
      }else if(!strcmp(argv[i],"-N")){
        *n = 1; 
      }else{
	for(j = 0; j < strlen(argv[i]); j++){
          if(isdigit(argv[i][j])){
            nums++;   
          }
        }
        if(nums == strlen(argv[i])){
	  *portnum = i; 
        }else{
          *hostname = i; 
        }       
      } 
    }
    if(*portnum == 0){
      print_usage_and_exit();
    }else if(*portnum != 0 && *hostname == 0){
      returned = 1;
    }else if(*portnum != 0 && *hostname != 0){
      returned = 0; 
    }else{
      print_usage_and_exit();
    }
  }
  return returned; 
}

void print_usage_and_exit(){
  fprintf(stderr, "mytalk [ -v ] [ -a ] [ -N ] [ hostname ] port\n"); 
  exit(-1); 
}
