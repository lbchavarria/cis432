#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "duckchat.h"

int main(int argc, char *argv[]) {
    int sockid, nread, addrlen;
    struct sockaddr_in my_addr, client_addr;

    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        printf("Server: socket error: %d\n", errno);
        exit(0);
    }

    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htons(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        printf("Server: bind fail: %d\n", errno);
        exit(0);
    }

    struct request req;

    while(1) {
        nread = recvfrom(sockid, req, sizeof(request), 0, (struct sockaddr *) &client_addr, &addrlen);
        if (nread > 0) {
            
        }
    }

    close(sockid);
}
