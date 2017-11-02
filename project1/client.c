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

int sockid;
char channel[CHANNEL_MAX], text[SAY_MAX], buff;
struct sockaddr_in server_addr;

int request_handler(struct request req) {
    if (req.req_type == REQ_LOGIN) {
        struct request_login req_login = (struct request_login)req;
        strcpy(req_login.req_username, argv[3]);
        return sendto(sockid, (struct request *)&req_login, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));

    }
    else if (req.req_type == REQ_LOGOUT) {
        struct request_logout req_logout = (struct request_logout)req;
        return sendto(sockid, (struct request *)&req_logout, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_JOIN) {
        struct request_join req_join = (struct request_join)req;
        strcpy(req_join.req_channel, channel);
        return sendto(sockid, (struct request *)&req_join, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_LEAVE) {
        struct request_leave req_leave = (struct request_leave)req;
        strcpy(req_leave.req_channel, channel);
        return sendto(sockid, (struct request *)&req_leave, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_SAY) {
        struct request_say req_say = (struct request_say)req;
        strcpy(req_say.req_channel, channel);
        strcpy(req_say.req_text, text);
        return sendto(sockid, (struct request *)&req_say, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_LIST) {
        struct request_list req_list = (struct request_list)req;
        return sendto(sockid, (struct request *)&req_list, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    else if (req.req_type == REQ_WHO) {
        struct request_who req_who = (struct request_who)req;
        strcpy(req_who.req_channel, channel);
        return sendto(sockid, (struct request *)&req_who, sizeof(struct request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
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

void txt_handler(struct text txt) {
    int i;
    if (txt.txt_type == TXT_SAY) {
        struct text_say *txt_say = (struct text_say *)&txt;
        printf("[%s][%s]: %s", txt_say->txt_channel, txt_say->txt_username, txt_say->txt_text);
    }
    else if (txt.txt_type == TXT_LIST) {
        struct text_list *txt_list = (struct text_list *)&txt;
        printf("Existing channels:\n");
        for (i = 0; i < txt_list->txt_nchannels; i++) {
            printf("%s\n", txt_list->txt_channels[i].ch_channel);
        }
    }
    else if (txt.txt_type == TXT_WHO) {
        struct text_who *txt_who = (struct text_who *)&txt;
        printf("Users on channel %s:\n", txt_who->txt_channel);
        for (i = 0; i < txt_who->txt_nusernames; i++) {
            printf("%s\n", txt_who->txt_users[i].us_username);
        }
    }
    else if (txt.txt_type == TXT_ERROR) {
        struct text_error *txt_error = (struct text_error *)&txt;
        printf("%s\n", txt_error->txt_error);
    }
}

int main(int argc, char *argv[]) {
    int retcode, nread, addrlen, i;
    struct sockaddr_in my_addr;

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
    struct text txt;

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
        return -1;
    }

    while (1) {
        i = 0;
        while (1) {
            read(0, &buff, 1);
            if (buff == '\b' && i > 1) {
                text[--i] = '';
            }
            else if (buff == '\r') {
                break;
            }
            else {
                if (i == SAY_MAX) {
                    break;
                }
                strcpy(text[i], buff);
                i++;
            }
        }
        req.req_type = exception_handler(text);
        if (req.req_type == -1) {
            continue;
        }
        retcode = request_handler(req);
        if (retcode <= -1) {
            printf("Client: sendto failed: %d\n", errno);
            //return -1;
        }
        nread = recvfrom(sockid, txt, sizeof(struct text), 0, (struct sockaddr *) &server_addr, &addrlen);
        if (nread > 0) {
            txt_handler(txt);
        }
    }
    
    cooked_mode();
    close(sockid);
    return 0;
}
