#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "duckchat.h"
#include "raw.h"

#define UNUSED __attribute__ ((unused))

int sockid;
struct sockaddr_in server_addr, from;
char current_channel[CHANNEL_MAX], temp_channel[CHANNEL_MAX];

void remove_spaces(char src[], char dst[]) {
    /* A funtion to remove white space
     * Used in setType()
     */
    int i;
    int j = 0;
    for (i = 0; i < strlen(src); i++) {
        if (src[i] != ' ') {
            dst[j++] = src[i];
        }
    }
    //dst[j] = '\0';
}

void client_login(char *args[]) {
    /* Sends login data to the server
     * Server uses data to log client in
     */
    int retcode;
    request_login r_login;
    r_login.req_type = REQ_LOGIN;
    strcpy(r_login.req_username, args[3]);
    retcode = sendto(sockid, (void *)&r_login, sizeof(r_login), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retcode <= -1) {
        printf("Login data failed to send\n");
    }
}

void get_char_input(char dest[]) {
    /* Read the input from client
     * Store input into char array and return char array
     */
    char c = '\0';
    char txt[SAY_MAX];
    int i = 0;
    printf("> ");
    while (1) {
        c = getchar();
        if (c == '\b' && i >= 1) {
            txt[--i] = '\0';
        }
        else if (c == '\n') {
            printf("\n");
            break;
        }
        else {
            if (i == SAY_MAX) {
                //txt[i] = '/0';
                break;
            }
            putchar(c);
            txt[i] = c;
            i++;
        }
    }
    strcpy(dest, txt);
}

request_t setType(char txt[]) {
    /* Reads txt for exceptions
     * Returns request_t based on exception
     * Returns SAY if there's certain exceptions are missing a channel name
     * Otherwise returns SAY
     */
    size_t i;
    if (strcmp(txt, "/exit") == 0) {
        return REQ_LOGOUT;
    }
    if (strncmp(txt, "/join", strlen("/join")) == 0) {
        char temp[SAY_MAX];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/join")) {
            return REQ_SAY;
        }
        strcpy(current_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/join ")); i++) {
            if (i == CHANNEL_MAX) {
                //current_channel[i] == '/0';
                break;
            }
            current_channel[i] = txt[strlen("/join ")+i];
        }
        return REQ_JOIN;
    }
    if (strncmp(txt, "/leave", strlen("/leave")) == 0) {
        char temp[SAY_MAX];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/leave")) {
            return REQ_SAY;
        }
        strcpy(temp_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/leave ")); i++) {
            if (i == CHANNEL_MAX) {
                //current_channel[i] == '/0';
                break;
            }
            temp_channel[i] = txt[strlen("/leave ")+i];
        }
        return REQ_LEAVE;
    }
    /*if (strcmp(txt, "/list") == 0) {
        return LIST;
    }
    if (strncmp(txt, "/who", strlen("/who")) == 0) {
        char temp[SAY_MAX];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/who")) {
            return SAY;
        }
        strcpy(temp_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/who ")); i++) {
            if (i == CHANNEL_MAX) {
                //temp_channel[i] == '/0';
                break;
            }
            temp_channel[i] = txt[strlen("/who ")+i];
        }
        return WHO;
    }*/
    if (strncmp(txt, "/switch", strlen("/switch")) == 0) {
        char temp[SAY_MAX];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/switch")) {
            return REQ_SAY;
        }
        strcpy(current_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/switch ")); i++) {
            if (i == CHANNEL_MAX) {
                //current_channel[i] == '/0';
                break;
            }
            current_channel[i] = txt[strlen("/switch ")+i];
        }
        return -1;
    }
    return REQ_SAY;
}

void client_data_handler(char txt[], request_t rt) { // if send fails, try casting data as void*
    /* Stores appropriate data into cd
     * Sends cd to server
     */
    int retcode;
    if (rt == REQ_LOGOUT) {
        struct request_logout r_logout;
        r_logout.req_type = rt;
        retcode = sendto(sockid, &r_logout, sizeof(struct request_logout), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (retcode <= -1) {
            printf("Client failed to send appropriate data to server\n");
        }
    }
    else if (rt == REQ_JOIN) {
        struct request_join r_join;
        r_join.req_type = rt;
        strcpy(r_join.req_channel, current_channel);
        retcode = sendto(sockid, &r_join, sizeof(struct request_join), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (retcode <= -1) {
            printf("Client failed to send appropriate data to server\n");
        }
    }
    else if (rt == REQ_LEAVE) {
        struct request_leave r_leave;
        r_leave.req_type = rt;
        strcpy(r_leave.req_channel, temp_channel);
        retcode = sendto(sockid, &r_leave, sizeof(struct request_leave), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (retcode <= -1) {
            printf("Client failed to send appropriate data to server\n");
        }
    }
    else if (rt == REQ_SAY) {
        struct request_say r_say;
        r_say.req_type = rt;
        strcpy(r_say.req_text, txt);
        retcode = sendto(sockid, &r_say, sizeof(struct request_say), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (retcode <= -1) {
            printf("Client failed to send appropriate data to server\n");
        }
    }
}

void server_data_handler() {
    /* Receives data from server
     * Handles sd depending on type
     */
    int nread;
    char r_txt[65536];
    unsigned int len = (unsigned int)sizeof(struct sockaddr_in);
    void *data;
    nread = recvfrom(sockid, data, sizeof(r_txt), 0, (struct sockaddr *)&from, &len);
    if (nread > 0) {
        if (((struct text *)data)->txt_type == TXT_SAY) {
            printf("[%s][%s]%s", ((struct text_say *)data)->txt_channel, ((struct text_say *)data)->txt_username, ((struct text_say *)data)->txt_text);
        }
        /*else if (sd->type == S_LIST) {
            int i;
            printf("Existing channels:\n");
            for (i = 0; i < sd->size; i++) {
                printf("%s\n", sd->channel_list[i]);
            }
        }
        else if (sd->type == S_WHO) {
            int i;
            printf("Users on channel %s:\n", sd->channel_list[0]);
            for (i = 0; i < sd->size; i++) {
                printf("%s\n", sd->user_list[i]);
            }
        }*/
        else if (((struct text *)data)->txt_type == TXT_ERROR) {
            printf("Error: %s", ((struct text_error *)data)->txt_error);
        }
    }
    else {
        printf("Error in receive from server\n");
    }
}

int main(UNUSED int argc, char *argv[]) {
    char txt[SAY_MAX];
    request_t rt;
    struct hostent *hp;
    
    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        printf("Socket setup failed\n");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        printf("Unknown host\n");
        return -1;
    }
    bcopy((char *)hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    if (strlen(argv[3]) > USERNAME_MAX) {
        printf ("Username too long. Max length is %d\n", USERNAME_MAX);
        return -1;
    }
    
    client_login(argv);
    
    if (raw_mode() == -1) {
        printf("Raw mode failed\n");
        return -1;
    }
    
    while (1) {
        get_char_input(txt);
        rt = setType(txt);
        if (rt == -1) {
            continue;
        }
        client_data_handler(txt, rt);
        if (rt == REQ_LOGOUT) {
            break;
        }
        server_data_handler();
    }
    
    cooked_mode();
    shutdown(sockid, 2);
    return 0;
}
