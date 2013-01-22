#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

#define DEBUG

#define H_TEXT "mirror [-t|-u] [-s|-c] dst-addr port\n" \
"\n" \
"Parameters:\n" \
"-t TCP\n" \
"-u UDP\n" \
"-h This help text\n\n"

#define BUF_LEN 1024

#define DEBUG_NTOH(naddr,nport, msg, dir) {					\
    unsigned int server_addr_h = ntohl(naddr);				\
    unsigned short server_port_h = ntohs(nport);			\
    printf("message '%s' - %s:  %d.%d.%d.%d:%d\n",			\
	   msg,								\
	   dir,								\
	   server_addr_h >> 24 & 255,					\
	   server_addr_h >> 16 & 255,					\
	   server_addr_h >> 8 & 255,					\
	   server_addr_h >> 0 & 255,					\
	   server_port_h						\
	   );								\
  }


void helptext();
void readargs(int argc, char *argv[], char *proto, char *mode, unsigned int *address, unsigned short *port, char * default_port);
void udp_server(const unsigned short port);
void tcp_server(const unsigned short port);
void udp_client(const unsigned int addr, const unsigned short port, int fd);
void tcp_client(const unsigned int addr, const unsigned short port, int fd);
void chomp(char *s);

int main(int argc, char *argv[]) {
  char proto, mode;
  unsigned int addr;
  unsigned short port;

  //read arguments (options)
  readargs(argc, argv, &proto, &mode, &addr, &port, "10001");

  if(mode == 's') {
    if(proto == 'u') {
      udp_server(port);
    } else {
      tcp_server(port);
    }
  } else {
    if(proto == 'u') {
      udp_client(addr, port, 0);
    } else {
      tcp_client(addr, port, 0);
    }
  }

  return 0;
}

/*
 * Read args - -t/-u necessary, -p xxxx, if not default port 10001
 * -c destination 
 */
void readargs(int argc, char *argv[], char *proto, char *mode, unsigned int *address, unsigned short *port, char * default_port) {
  char c; //for getopt
  int protocount = 0, modecount = 0; //count how often a proto occurs, must be 1
  char *caddr = NULL; //character array for address
  char *cport = default_port; //character array for port
  unsigned int ip[4]; //helper that stores temp ip parts

  while( (c = getopt(argc, argv, "tuhsc:p:")) != -1) {
    switch(c) {
    case 't':
      *proto = 't';
      protocount++;
      break;
    case 'u':
      *proto = 'u';
      protocount++;
      break;
    case 's':
      *mode = 's';
      modecount++;
      break;
    case 'c':
      *mode = 'c';
      caddr = optarg;
      if ( sscanf(caddr, "%u.%u.%u.%u", 
		  &ip[0], 
		  &ip[1], 
		  &ip[2], 
		  &ip[3]) == EOF 
	   ) {
	fprintf(stderr, "Not a valid IP address\n");
      }
      *address = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
      modecount++;
      break;
    case 'p':
      cport = optarg;
      break;
    case 'h':
      helptext();
      break;
    default:
      break;
    }
  }
  //convert port to int
  *port = (unsigned short) atoi(cport);
  if(*port < 10000) {
    fprintf(stderr, "Port %d not allowed. Needs to be >= 10000\n", *port);
    exit(1);
  }
  
  //make sure that either protocol is specified
  if(protocount != 1) {
    fprintf(stderr, "Please specify either TCP (-t), or UDP (-u)\n");
    exit(1);
  }

  //make sure that either server or client is specified
  if(modecount != 1) {
    fprintf(stderr, "Please specify either server (-s), or client (-c) mode\n");
    exit(1);
  }
}

/*
 * Shows the help text
 */
void helptext() {
  printf(H_TEXT);
  exit(1);
}

/*
 * Create a UDP server
 */
void udp_server(const unsigned short port) {
  printf("creating udp server on port %d\n", port);

  int sock, addrlen;
  char buf[BUF_LEN];
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  unsigned int client_addr_size;

  unsigned short nport = htons(port); //port in network byte order
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = nport;
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if( (sock = socket(AF_INET, SOCK_DGRAM, 0) ) == - 1) { //open a UDP socket
    perror("Creating socket");
    exit(1);
  }
  if( bind(sock, (const struct sockaddr *) &server_addr, sizeof(server_addr)) == -1 ) {
    perror("Binding socket to name");
    exit(1);
  }

  for( ;; ) { //run until break
    bzero(buf, sizeof(buf));
    bzero(&client_addr, sizeof(client_addr));
    client_addr_size = sizeof(client_addr);
    addrlen = recvfrom(sock, buf, sizeof(buf), 0, 
	     (struct sockaddr *) &client_addr, &client_addr_size
	     );
    if( addrlen == -1 ) {
      perror("receiving message");
      exit(1);
    }
    chomp(buf);

#ifdef DEBUG
    printf("<=== ");
    DEBUG_NTOH(client_addr.sin_addr.s_addr, client_addr.sin_port, buf, "from host");
    printf("<=== ");
    DEBUG_NTOH(server_addr.sin_addr.s_addr, server_addr.sin_port, buf, "to host");
    printf("===> ");
    DEBUG_NTOH(server_addr.sin_addr.s_addr, server_addr.sin_port, buf, "from host");
    printf("===> ");
    DEBUG_NTOH(client_addr.sin_addr.s_addr, client_addr.sin_port, buf, "to host");
    printf("\n");
#endif

    //send back
    if( sendto(
	       sock, 
	       buf, 
	       sizeof(buf), 
	       0, 
	       (const struct sockaddr *) &client_addr, 
	       sizeof(client_addr)
	       ) == -1 ) {
      perror("mirroring message");
      exit(1);
    }
  }

  close(sock);
}

/*
 * Create a TCP server
 */
void tcp_server(const unsigned short port) {
  printf("creating tcp server on port %d\n", port);

  int sock; //original, listening socket
  int acc_sock; //accepted socket
  unsigned short nport = htons(port);
  struct sockaddr_in server_address;
  unsigned int server_address_l = sizeof(server_address);
  struct sockaddr_in client_address;
  unsigned int client_address_l = sizeof(client_address);

  if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
    perror("Opening TCP socket");
    exit(1);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = nport;
  server_address.sin_addr.s_addr = INADDR_ANY;
  if( bind(sock, (const struct sockaddr *) &server_address, server_address_l) == -1 ) {
    perror("Binding socket");
    exit(1);
  }

  if( listen(sock, 5) == -1 ) {
    perror("Listening on socket");
    exit(1);
  }

  /********************/
  /* loop until break */
  /********************/
  for(;;) {
    if( (acc_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_l)) == -1 ) {
      perror("Answering socket");
      exit(1);
    }

#ifdef DEBUG
    printf("Socket opened - client info: \n");
    DEBUG_NTOH(client_address.sin_addr.s_addr, client_address.sin_port, "", "-");
    printf("Socket opened - server info: \n ");
    DEBUG_NTOH(server_address.sin_addr.s_addr, server_address.sin_port, "", "-");
    printf("\n");
#endif

    close(acc_sock);
  }
  /*******************/
  /* end of for loop */
  /*******************/ 

  close(sock);
}

/*
 * Create a UDP client
 */
void udp_client(const unsigned int addr, const unsigned short port, int fd) {
  int sock; 
  //int addrlen;
  char buf[BUF_LEN];
  struct sockaddr_in server_addr;
  unsigned int server_addr_size = sizeof(server_addr);
  struct sockaddr_in client_addr;
  unsigned int client_addr_size = sizeof(client_addr);

  unsigned int naddr = htonl(addr);
  unsigned short nport = htons(port); //port in network byte order
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = nport;
  server_addr.sin_addr.s_addr = naddr;

  if( (sock = socket(AF_INET, SOCK_DGRAM, 0) ) == - 1) { //open a UDP socket
    perror("Creating socket");
    exit(1);
  }

  for(;;) {
    bzero(buf, sizeof(buf));
    if( read(fd, buf, sizeof(buf)) == -1 ) {
      printf("reading input");
      exit(1);
    }

    if( sendto(sock, buf, sizeof(buf), 0, (const struct sockaddr *) &server_addr, server_addr_size) == -1 ) {
      perror("Sending message to server");
      fprintf(stderr, "%d\n", errno);
      fprintf(stderr, "%x\n", server_addr.sin_addr.s_addr);
      exit(1);
    }

    if( getsockname(sock, (struct sockaddr *) &client_addr, &client_addr_size) == -1 ) {
      perror("getting name of socket\n");
      exit(1);
    }

#ifdef DEBUG
    chomp(buf);
    printf("===> ");
    DEBUG_NTOH(client_addr.sin_addr.s_addr, client_addr.sin_port,buf,"from host");    
    printf("===> ");
    DEBUG_NTOH(naddr, nport,buf,"to host");
#endif

    /*****************************************************************/
    /* fork a child process so that the user can continue with input */
    /*****************************************************************/
    int pid;
    signal(SIGCHLD, SIG_IGN); // ignore signals from children => avoid defunct processes
    if( (pid = fork()) == -1 ) {
      perror("Forking child process");
      exit(1);
    } else if( pid == 0 ) { //child
      /****************************/
      /* set timeout for recvfrom */
      /****************************/
      struct timeval tv;
      tv.tv_sec = 5; //max 5 sec
      tv.tv_usec = 0;
      setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));
      bzero(buf, sizeof(buf));
      //now, receive
      if( recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &server_addr, &server_addr_size) == -1 ) {
	perror("Receiving answer message");
	exit(1);
      }
      
#ifdef DEBUG
      printf("<=== ");
      DEBUG_NTOH(server_addr.sin_addr.s_addr, server_addr.sin_port, buf, "from host");
      printf("<=== ");
      DEBUG_NTOH(client_addr.sin_addr.s_addr, client_addr.sin_port, buf, "to host");
#endif
      exit(0);
    } //end of forked child
  } //end of loop
  
  close(sock);
}

/*
 * Create a TCP client
 */
void tcp_client(const unsigned int addr, const unsigned short port, int fd) {
  printf("creating tcp client, dst %d.%d.%d.%d and dport %d\n", addr >> 24, (addr >> 16) & 255, (addr >> 8) & 255,addr & 255, port);
}

void chomp (char* s) {
  int end = strlen(s) - 1;
  if (end >= 0 && s[end] == '\n')
    s[end] = '\0';
}
