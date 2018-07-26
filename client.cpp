#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h> 

/*void connect_w_to(s) { 
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 
  int errno;

  // Create socket 
  soc = socket(AF_INET, SOCK_STREAM, 0); 
  if (soc < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(2000); 
  addr.sin_addr.s_addr = inet_addr("192.168.0.1"); 

  // Set non-blocking 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // Trying to connect with timeout 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
        do { 
           tv.tv_sec = 15; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(soc, &myset); 
           res = select(soc+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) ); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              exit(0); 
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg &= (~O_NONBLOCK); 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // I hope that is all 
}*/

void error(const char *msg) {
    perror(msg);
    exit(0);
}

char *get_ip() {
    FILE *f;
    char line[100] , *p , *c;

    f = fopen("/proc/net/route" , "r");

    while(fgets(line , 100 , f))
    {
        p = strtok(line , " \t");
        c = strtok(NULL , " \t");

        if(p!=NULL && c!=NULL)
        {
            if(strcmp(c , "00000000") == 0)
            {
                //printf("Default interface is : %s \n" , p);
                break;
            }
        }
    }

    //which family do we require , AF_INET or AF_INET6
    int fm = AF_INET;
    struct ifaddrs *ifaddr, *ifa;
    int family , s;
    char host[NI_MAXHOST];

    char *ret = (char*) malloc(NI_MAXHOST * sizeof(char));

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        if(strcmp( ifa->ifa_name , p) == 0)
        {
            if (family == fm)
            {
                s = getnameinfo( ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6) , host , NI_MAXHOST , NULL , 0 , NI_NUMERICHOST);

                if (s != 0)
                {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                }

                sprintf(ret, "%s", host);
            }
        }
    }

    freeifaddrs(ifaddr);

    return ret;
}

char *get_hostname() {
    char *hostname = (char*) malloc(64 * sizeof(char));
    gethostname(hostname, 64);

    return hostname;
}

int port_connection(char *hostname) {
    int sockfd, portno = 51717, n;
    int ports_to_connect = 16;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(hostname);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /*int connresult;

    for(int i = portno; i < portno + ports_to_connect; i++) {
        serv_addr.sin_port = htons(i);

	    if(i != portno)
	        printf("\n");

	    printf("Trying to bind on %i... ", i);

	    //SET A TIMER HERE
        connresult = connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr));

        if(connresult != EINPROGRESS && connresult != 0)
            error("Couldn't connect :(");
            
        while(sockfd)
    }*/

    connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr));



    printf("Bound.\n");

    bzero(buffer, 256);

    strcpy(buffer, get_hostname());
    strcat(buffer, "/");
    strcat(buffer, get_ip());
    strcat(buffer, "/CLIENT");

    n = write(sockfd, buffer, strlen(buffer));

    if (n < 0)
        error("ERROR writing to socket");

    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);

    if (n < 0)
        error("ERROR reading from socket");

    close(sockfd);

    return atoi(buffer);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("%s\n", "Hostname not specified.");

        return 0;
    }

    //int sockfd, portno = port_connection(argv[1]), n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    int sockfd, portno = atoi(argv[2]), n;

    printf("Port: %d\n-----\n", portno);

    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    usleep(750 * 1000);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    while(n > 0) {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);

        n = write(sockfd, buffer, strlen(buffer));

        if (n < 0)
            error("ERROR writing to socket");

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n < 0)
            error("ERROR reading from socket");

        printf("\t%s\n", buffer);
    }

    close(sockfd);

    return 0;
}
