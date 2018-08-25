#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXDATASIZE 100

void* get_addr_ptr(sockaddr *saddr) {
    if( saddr->sa_family == AF_INET )
        return &(((sockaddr_in*)saddr)->sin_addr);
    return &(((sockaddr_in6*)saddr)->sin6_addr);
}

void* get_port_ptr(sockaddr *saddr) {
    if( saddr->sa_family == AF_INET )
        return &(((sockaddr_in*)saddr)->sin_port);
    return &(((sockaddr_in6*)saddr)->sin6_port);
}

int main(int argc, char *argv[]){
    //Check input
    if ( argc != 3) {
        printf("Invalid arguments, please supply a valid port!\n");
        printf("\tE.g. `./client <server-ip> 8080`\n");
        exit(1);
    }
    
    int sockfd, rv;
    char buf[MAXDATASIZE], ipstr[INET6_ADDRSTRLEN];

    addrinfo hints, *server, *p;

    memset(&hints, 0, sizeof  hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo(argv[1], argv[2], &hints, &server)) != 0 ) {
        printf("getaddrinfo error: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = server; p != NULL; p = p->ai_next) {
        printf("Get socket...\t");
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if ( sockfd == -1 ) {
            printf("FAILED!\n");
            perror("\tSocket");
            continue;
        }
        printf("OK\n");
        
        printf("Connecting...\t");
        if ( connect(sockfd, p->ai_addr, p->ai_addrlen) == -1 ) {
            printf("FAILED!\n");
            perror("\tConnect");
            continue;
        }
        printf("OK\n");
        break;
    }

    if (p == NULL) {
        printf("Failed to connect on any interface!\n");
        exit(1);
    }

    freeaddrinfo(server);

    sockaddr local, peer;
    socklen_t local_len = sizeof local;
    socklen_t peer_len  = sizeof peer;
    in_port_t* port;

    if(getsockname(sockfd, &local, &local_len) != 0) {
        perror("getsockname");
        exit(1);
    }
    if(getpeername(sockfd, &peer,  &peer_len) != 0 ) {
        perror("getpeername");
        exit(1);
    }

    inet_ntop(local.sa_family, get_addr_ptr(&local),
              ipstr, sizeof ipstr);
    port = (in_port_t*)get_port_ptr(&local);
    printf("Local IP:\t%s\n", ipstr);
    printf("Local Port:\t%u\n", ntohs(*port));

    inet_ntop(peer.sa_family, get_addr_ptr(&peer),
              ipstr, sizeof ipstr);
    port = (in_port_t*)get_port_ptr(&peer);
    printf("Peer IP:\t%s\n", ipstr);
    printf("Peer Port:\t%u\n", ntohs(*port));

    rv = recv(sockfd, buf, MAXDATASIZE-1, 0);
    close(sockfd);
    if (rv == -1) {
        perror("Receive");
        exit(1);
    }

    buf[rv] = '\0'; // NULL terminate

    printf("Received:\t%s\n", buf);

    return 0;
}

