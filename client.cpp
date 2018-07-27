#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
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
#include <sstream>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

void error(const char *msg) {
    fprintf(stderr, msg);
    exit(1);
}

char* get_ip() {
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
        error("Couldn't get what your IP is :o\n");

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
                    error("Function called 'getnameinfo', but I too don't know why :/\n");

                sprintf(ret, "%s", host);
            }
        }
    }

    freeifaddrs(ifaddr);

    return ret;
}

char* get_hostname() {
    char *hostname = (char*) malloc(64 * sizeof(char));
    gethostname(hostname, 64);

    return hostname;
}

int get_port(const char* host, const char* hostname, const char* ip, const char* role) {
    char* url = (char*) malloc(sizeof(char) * 256);
    curlpp::Cleanup cu;
    std::ostringstream os;

    sprintf(url, "http://%s:51000/?ip=%s&name=%s&role=%s", host, ip, hostname, role);

    os << curlpp::options::Url(std::string(url));

    return atoi(os.str().c_str());
}

int main(int argc, char *argv[]) {
    if(argc < 2)
        error("Oh shoot, it looks like you didn't specify a hostname.\n");

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = get_port(argv[1], get_hostname(), get_ip(), "CLIENT");

    printf("Port: %d\n-----\n", portno);

    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("There was a problem while opening the socket :(\n");

    server = gethostbyname(argv[1]);

    if (server == NULL)
        error("Apparently there's no such host :^(\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    usleep(750 * 1000);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("Beep boop, we couldn't connect to given host :(\n");

    n = 1;

    while(n > 0) {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);

        n = write(sockfd, buffer, strlen(buffer));

        if (n < 0)
            error("There was an error while writing to socket :(\n");

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n < 0)
            error("There was an error while reading from socket :(\n");

        printf("\t%s\n", buffer);
    }

    close(sockfd);

    return 0;
}
