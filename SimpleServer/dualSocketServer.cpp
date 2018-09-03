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

int main(int argc, char *argv[]){
    //Check input
    if ( argc != 2) {
        printf("Invalid arguments, please supply a valid port!\n");
        printf("\tE.g. `./server 8080`\n");
        exit(1);
    }

    fd_set master_fds, read_fds;
    int fdmax = 0;

    addrinfo         hints, *res, *p;
    sockaddr_storage remote_addr;

    char      ipstr[INET6_ADDRSTRLEN];
    int       sockfd, remotefd, rv;
    socklen_t addr_sz;

    memset(&hints, 0, sizeof hints); // Zero out our struct
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = AI_PASSIVE;  // Serves on all interfaces

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

        // Don't let AF_INET6 sockets dual stack!
        if ( p->ai_family == AF_INET6 ) {
            int zero = 1;
            setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero));
        }

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

        FD_SET(sockfd, &master_fds);
        fdmax = sockfd+1;
    }

    freeaddrinfo(res);

    if ( fdmax == 0 ) {
        printf("Failed to listen on any interface");
        exit(1);
    }

    while (1) {
        addr_sz = sizeof remote_addr;

        read_fds = master_fds;
        if ( select(fdmax, &read_fds, NULL, NULL, NULL) == -1 ) {
            perror("Select");
            exit(1);
        }

        // Go through all FDs finding those set in read_fds
        for ( int i=0; i < fdmax; i++) {
            if ( FD_ISSET(i, &read_fds) ) {
                remotefd = accept(i, (sockaddr*)&remote_addr, &addr_sz);
                if ( remotefd == -1 ) {
                    perror("Accept");
                    continue;
                }

                inet_ntop(remote_addr.ss_family,
                          get_addr_ptr((sockaddr *)&remote_addr),
                          ipstr,
                          sizeof ipstr);
                printf("Connection from: %s\n", ipstr);

                if ( send(remotefd, "Hello World", 11, 0) == -1 ) {
                    perror("Send");
                }
                close(remotefd);
            }
        }
    }

    return 0;
}

