#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define H_TEXT "mirror [-t|-u] [-s|-c] dst-addr port\n" \
"\n" \
"Parameters:\n" \
"-t TCP\n" \
"-u UDP\n" \
"-h This help text\n\n"

void helptext();
void readargs(int argc, char *argv[], char *proto, char *mode, unsigned int *address, unsigned short *port, char * default_port);
void udp_server(const unsigned short port);
void tcp_server(const unsigned short port);
void udp_client(const unsigned int addr, const unsigned short port);
void tcp_client(const unsigned int addr, const unsigned short port);

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
      udp_client(addr, port);
    } else {
      tcp_client(addr, port);
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

  int sock;
  struct sockaddr_in server_addr;
  unsigned short nport = htons(port); //port in network byte order
  printf("%x, %x\n",port, nport);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = nport;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  
  if( (sock = socket(AF_INET, SOCK_DGRAM, 0) ) == - 1) { //open a UDP socket
    perror("Creating socket");
  }
  if( bind(sock, (const struct sockaddr *) &server_addr, sizeof(server_addr)) == -1 ) {
    perror("Binding socket to name");
  }
  
  sleep(10);

}

/*
 * Create a TCP server
 */
void tcp_server(const unsigned short port) {
  printf("creating tcp server on port %d\n", port);
}

/*
 * Create a UDP client
 */
void udp_client(const unsigned int addr, const unsigned short port) {
  printf("creating udp client, dst %d.%d.%d.%d and dport %d\n", addr >> 24 & 255, addr >> 16 & 255, addr >> 8 & 255, addr & 255, port);
}

/*
 * Create a TCP client
 */
void tcp_client(const unsigned int addr, const unsigned short port) {
  printf("creating tcp client, dst %d.%d.%d.%d and dport %d\n", addr >> 24, (addr >> 16) & 255, (addr >> 8) & 255,addr & 255, port);
}

