#include <iostream>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 10

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
    if ( argc != 2) {
        printf("Invalid arguments, please supply a valid port!\n");
        printf("\tE.g. `./server 8080`\n");
        exit(1);
    }

    addrinfo  hints, *res, *p;
    char      ipstr[INET6_ADDRSTRLEN];
    int       sockfd, rv;
    
    memset(&hints, 0, sizeof hints); // Zero out our struct
    hints.ai_family   = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = AI_PASSIVE;  // Serves on all, Ie ::

    rv = getaddrinfo(NULL, argv[1], &hints, &res);
    if ( rv != 0 ) {
        printf("getaddrinfo failed!\n");
        printf("\t%s\n",gai_strerror(rv));
        exit(1);
    }

    for ( p = res; p != NULL; p = p->ai_next ) {
        inet_ntop(p->ai_family, get_addr_ptr(p->ai_addr), ipstr, sizeof ipstr);
        printf("Found IP: %s\n",ipstr);
        
        printf("\tGetting socket...\t");
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if ( sockfd == -1 ) {
            printf("FAILED!\n");
            perror("\t\tSocket");
            continue;
        }
        printf("OK\n");

        printf("\tBind Socket to Port...\t");
        if ( bind(sockfd, p->ai_addr, p->ai_addrlen) == -1 ) {
            close(sockfd);
            printf("FAILED!\n");
            perror("\t\tBind");
            continue;
        }
        printf("OK\n");

        printf("\tListen on socket...\t");
        if ( listen(sockfd, BACKLOG) == -1 ){
            printf("FAILED!\n");
            perror("\t\tError");
            continue;
        }
        printf("OK\n");
        break;
    }

    freeaddrinfo(res);

    if ( p == NULL ) {
        printf("Failed to listen on any interface");
        exit(1);
    }

    while (1) {
        sockaddr_storage remote_addr;
        socklen_t addr_sz = sizeof remote_addr;
        int remotefd = accept(sockfd, (sockaddr*)&remote_addr, &addr_sz);
        if ( remotefd == -1 ) {
            perror("Accept");
            continue;
        }

        inet_ntop(remote_addr.ss_family, 
                  get_addr_ptr((sockaddr *)&remote_addr),
                  ipstr, 
                  sizeof ipstr);

        printf("----------------------------\n");
        printf("New Connection...\n");

        sockaddr local, peer;
        socklen_t local_len = sizeof local;
        socklen_t peer_len  = sizeof peer;
        in_port_t* port;

        if(getsockname(remotefd, &local, &local_len) != 0) {
            perror("\tgetsockname");
            exit(1);
        }
        if(getpeername(remotefd, &peer,  &peer_len) != 0 ) {
            perror("\tgetpeername");
            exit(1);
        }

        inet_ntop(local.sa_family, get_addr_ptr(&local),
                  ipstr, sizeof ipstr);
        port = (in_port_t*)get_port_ptr(&local);
        printf("\tLocal IP:\t%s\n", ipstr);
        printf("\tLocal Port:\t%u\n", ntohs(*port));

        inet_ntop(peer.sa_family, get_addr_ptr(&peer),
                  ipstr, sizeof ipstr);
        port = (in_port_t*)get_port_ptr(&peer);
        printf("\tPeer IP:\t%s\n", ipstr);
        printf("\tPeer Port:\t%u\n", ntohs(*port));
        
        if ( send(remotefd, "Hello World", 11, 0) == -1 ) {
            perror("Send");
        }
        close(remotefd);
    }

    return 0;
}

