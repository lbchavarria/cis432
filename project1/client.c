#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include "duckchat.h"
#include "raw.h"

char channel[CHANNEL_MAX], text[SAY_MAX];

int request_handler(struct request req) {
    if (req.req_type == REQ_LOGIN) {
        struct request_login req_login = (request_login)req;
        strcpy(req_login.req_username, argv[3]);
        return sendto(sockid, (request *)&req_login, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));

    }
    else if (req.req_type == REQ_LOGOUT) {
        struct request_logout req_logout = (request_logout)req;
        return sendto(sockid, (request *)&req_logout, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_JOIN) {
        struct request_join req_join = (request_join)req;
        strcpy(req_join.req_channel, channel);
        return sendto(sockid, (request *)&req_join, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_LEAVE) {
        struct request_leave req_leave = (request_leave)req;
        strcpy(req_leave.req_channel, channel);
        return sendto(sockid, (request *)&req_leave, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_SAY) {
        struct request_say req_say = (request_say)req;
        strcpy(req_say.req_channel, channel);
        strcpy(req_say.req_text, text);
        return sendto(sockid, (request *)&req_say, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_LIST) {
        struct request_list req_list = (request_list)req;
        return sendto(sockid, (request *)&req_list, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_WHO) {
        struct request_who req_who = (request_who)req;
        strcpy(req_who.req_channel, channel);
        return sendto(sockid, (request *)&req_who, sizeof(request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
}

request_t exception_handler(char text[]) {
    int i;
    if (strcmp(text, "/exit") == 0) {
        return REQ_LOGOUT;
    }
    if (strncmp(text, "/join", strlen("/join")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/join ")); i++) {
            channel[i] = text[strlen("/join ")+i];
        }
        return REQ_JOIN;
    }
    if (strncmp(text, "/leave", strlen("/leave")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/leave ")); i++) {
            channel[i] = text[strlen("/leave ")+i];
        }
        return REQ_LEAVE;
    }
    if (strcmp(text, "/list") == 0) {
        return REQ_LIST;
    }
    if (strncmp(text, "/who", strlen("/who")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/who ")); i++) {
            channel[i] = text[strlen("/who ")+i];
        }
        return REQ_WHO;
    }
    if (strncmp(text, "/switch", strlen("/switch")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/switch ")); i++) {
            channel[i] = text[strlen("/switch ")+i];
        }
        return -1;
    }
    return REQ_SAY;
}

int main(int argc, char *argv[]) {
    int sockid, retcode;
    struct sockaddr_in my_addr, server_addr;

    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        printf("Client: socket failed: %d\n", errno);
        return -1;
    }

    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(argv[2]);
    if (bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        printf("Client: bind fail: %d\n", errno);
        return -1;
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = gethostbyname(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    struct request req;

    if (strlen(argv[3]) > USERNAME_MAX) {
        printf("Username too long. Max length is %d\n", USERNAME_MAX);
        return -1;
    }

    req.req_type = REQ_LOGIN;
    retcode = request_handler(req);
    if (retcode <= -1) {
        printf("Client: sendto failed: %d\n", errno);
        return -1;
    }
    
    if (raw_mode() == -1) {
        printf("Raw mode failed\n");
    }

    while (1) {
        req.req_type = exception_handler(text);
        if (req.req_type == -1) {
            continue;
        }
        retcode = request_handler(req);
        if (retcode <= -1) {
            printf("Client: sendto failed: %d\n", errno);
            return -1;
        }
    }
    
    cooked_mode();
    close(sockid);
    return 0;
}
