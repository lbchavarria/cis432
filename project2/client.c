#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "duckchat.h"
#include "raw.h"

#define UNUSED __attribute__ ((unused))
#define M_MAXSIZE 65536

char r_txt[M_MAXSIZE];

int sockid;
char i_channel[CHANNEL_MAX], text[SAY_MAX], temp_channel[CHANNEL_MAX], buff;
struct sockaddr_in server_addr;


request_t exception_handler(char text[]) {
    size_t i;
    if (strcmp(text, "/exit") == 0) {
        return REQ_LOGOUT;
    }
    if (strncmp(text, "/join", strlen("/join")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/join ")); i++) {
            i_channel[i] = text[strlen("/join ")+i];
        }
        return REQ_JOIN;
    }
    if (strncmp(text, "/leave", strlen("/leave")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/leave ")); i++) {
            i_channel[i] = text[strlen("/leave ")+i];
        }
        return REQ_LEAVE;
    }
    if (strcmp(text, "/list") == 0) {
        return REQ_LIST;
    }
    if (strncmp(text, "/who", strlen("/who")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/who ")); i++) {
            temp_channel[i] = text[strlen("/who ")+i];
        }
        return REQ_WHO;
    }
    if (strncmp(text, "/switch", strlen("/switch")) == 0) {
        for (i = 0; i < (strlen(text) - strlen("/switch ")); i++) {
            i_channel[i] = text[strlen("/switch ")+i];
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
        printf("Error\n");
        struct text_error *txt_error = (struct text_error *)&txt;
        perror(txt_error->txt_error);
    }
}

int main(UNUSED int argc, char *argv[]) {
    int retcode, nread, i;
    struct sockaddr_in from;// my_addr;
    struct hostent *hp;

    //printf("Set up socket\n");
    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        perror("Client: socket failed");
        return -1;
    }
    
    /*
    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        printf("Client: bind fail: %d\n", errno);
        return -1;
    }
    
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = gethostbyname(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    */
    //printf("Set up server\n");
    server_addr.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        perror("Client: unknown host");
        return -1;
    }
    bcopy((char *)hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
    server_addr.sin_port = htons(atoi(argv[2]));
    unsigned int len = (unsigned int)sizeof(struct sockaddr_in);
     
    struct request req;
    struct text txt;

    //printf("Login\n");
    if (strlen(argv[3]) > USERNAME_MAX) {
        printf("Username too long. Max length is %d\n", USERNAME_MAX);
        return -1;
    }

    //req.req_type = REQ_LOGIN;
    struct request_login req_login;// = (struct request_login *)&req;
    strcpy(req_login.req_username, argv[3]);
    req_login.req_type = REQ_LOGIN;
    strcpy(i_channel, "Common");
    void *login = &req_login;
    retcode = sendto(sockid, login, sizeof(struct request_login), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (retcode <= -1) {
        perror("Client: sendto failed");
        return -1;
    }
    //printf("Raw mode\n");
    if (raw_mode() == -1) {
        printf("Raw mode failed\n");
        return -1;
    }

    ///printf("Start while loop\n");
    while (1) {
        i = 0;
        //printf("Start read\n");
        printf("> ");
        while (1) {
            read(0, &buff, 1);
            if (buff == '\b' && i >= 1) {
                text[--i] = '\0';
            }
            else if (buff == '\n') {
                printf("%c", buff);
                break;
            }
            else {
                if (i == SAY_MAX) {
                    break;
                }
                printf("%c", buff);
                text[i] = buff;
                i++;
            }
        }
        //printf("Check exceptions\n");
        req.req_type = exception_handler(text);
        //printf("Exception handled\n");
        //printf("%d\n", req.req_type);
        if (req.req_type == -1) {
            //printf("Switch\n");
            continue;
        }
        //Turn else if into functions
        else if (req.req_type == REQ_LOGOUT) {
            //printf("Logout\n");
            struct request_logout req_logout;// = (struct request_logout *)&req;
            req_logout.req_type = REQ_LOGOUT;
            void *logout = &req_logout;
            retcode = sendto(sockid, logout, sizeof(logout), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        else if (req.req_type == REQ_JOIN) {
            //printf("Join\n");
            struct request_join req_join;// = (struct request_join *)&req;
            strcpy(req_join.req_channel, i_channel);
            req_join.req_type = REQ_JOIN;
            void *join = &req_join;
            retcode = sendto(sockid, join, sizeof(join), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        else if (req.req_type == REQ_LEAVE) {
            //printf("Leave\n");
            struct request_leave req_leave;//= (struct request_leave *)&req;
            strcpy(req_leave.req_channel, i_channel);
            req_leave.req_type = REQ_LEAVE;
            void *leave = &req_leave;
            retcode = sendto(sockid, leave, sizeof(leave), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        else if (req.req_type == REQ_SAY) {
            //printf("Say\n");
            struct request_say req_say;// = (struct request_say *)&req;
            strcpy(req_say.req_channel, i_channel);
            strcpy(req_say.req_text, text);
            req_say.req_type = REQ_SAY;
            void *say = &req_say;
            retcode = sendto(sockid, say, sizeof(say), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        else if (req.req_type == REQ_LIST) {
            //printf("List\n");
            struct request_list req_list;// = (struct request_list *)&req;
            req_list.req_type = REQ_LIST;
            void *list = &req_list;
            retcode = sendto(sockid, list, sizeof(list), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        else if (req.req_type == REQ_WHO) {
            //printf("Who\n");
            struct request_who req_who;// = (struct request_who *)&req;
            strcpy(req_who.req_channel, i_channel);
            req_who.req_type = REQ_WHO;
            void *who = &req_who;
            retcode = sendto(sockid, who, sizeof(who), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        //retcode = request_handler(req);
        if (retcode <= -1) {
            perror("Client: sendto failed");
            //return -1;
        }
        if (req.req_type == REQ_LOGOUT && retcode > -1) {
            //printf("Break out\n");
            break;
        }
        //printf("Receive\n");
        if (req.req_type == REQ_JOIN || req.req_type == REQ_LEAVE) {
            continue;
        }
        nread = recvfrom(sockid, (struct text *)&txt, sizeof(r_txt), 0, (struct sockaddr *) &from, &len);
        if (nread > 0) {
            //printf("Text handle\n");
            txt_handler(txt);
        }
    }
    
    cooked_mode();
    shutdown(sockid, 2);
    return 0;
}
