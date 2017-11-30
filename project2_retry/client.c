#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "chatroom.h"
#include "raw.h"

#define UNUSED __attribute__ ((unused))

int sockid;
struct sockaddr_in server_addr, from;
char current_channel[CHANNEL_MAX+1], temp_channel[CHANNEL_MAX+1], user_name[USERNAME_MAX+1];

void remove_spaces(char src[], char dst[]) {
    /* A funtion to remove white space
     * Used in setType()
     */
    int i;
    int j = 0;
    for (i = 0; src[i] != '\0'; i++) {
        if (src[i] != ' ') {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

void client_login() {
    /* Sends login data to the server
     * Server uses data to log client in
     */
    int retcode;
    CData *cd = (CData *)malloc(sizeof(CData));
    cd->type = LOGIN;
    strcpy(cd->username, user_name);
    strcpy(current_channel, "Common");
    retcode = sendto(sockid, cd, sizeof(CData), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retcode <= -1) {
        printf("Login data failed to send\n");
    }
    free(cd);
}

void get_char_input(char dest[]) {
    /* Read the input from client
     * Store input into char array and return char array
     */
    char c = '\0';
    char txt[MESSAGE_MAX+1];
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
            if (i == MESSAGE_MAX) {
                txt[i] = '\0';
                break;
            }
            putchar(c);
            txt[i] = c;
            i++;
        }
    }
    strcpy(dest, txt);
}

c_type setType(char txt[]) {
    /* Reads txt for exceptions
     * Returns c_type based on exception
     * Returns SAY if there's certain exceptions are missing a channel name
     * Otherwise returns SAY
     */
    size_t i;
    if (strcmp(txt, "/exit") == 0) {
        return LOGOUT;
    }
    if (strncmp(txt, "/join", strlen("/join")) == 0) {
        char temp[MESSAGE_MAX+1];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/join")) {
            return SAY;
        }
        strcpy(current_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/join ")); i++) {
            if (i == CHANNEL_MAX) {
                current_channel[i] == '\0';
                break;
            }
            current_channel[i] = txt[strlen("/join ")+i];
        }
        return JOIN;
    }
    if (strncmp(txt, "/leave", strlen("/leave")) == 0) {
        char temp[MESSAGE_MAX+1];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/leave")) {
            return SAY;
        }
        strcpy(current_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/leave ")); i++) {
            if (i == CHANNEL_MAX) {
                current_channel[i] == '\0';
                break;
            }
            current_channel[i] = txt[strlen("/leave ")+i];
        }
        return LEAVE;
    }/*
    if (strcmp(txt, "/list") == 0) {
        return LIST;
    }
    if (strncmp(txt, "/who", strlen("/who")) == 0) {
        char temp[MESSAGE_MAX+1];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/who")) {
            return SAY;
        }
        strcpy(temp_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/who ")); i++) {
            if (i == CHANNEL_MAX) {
                temp_channel[i] == '\0';
                break;
            }
            temp_channel[i] = txt[strlen("/who ")+i];
        }
        return WHO;
    }*/
    if (strncmp(txt, "/switch", strlen("/switch")) == 0) {
        char temp[MESSAGE_MAX+1];
        remove_spaces(txt, temp);
        if (strlen(temp) == strlen("/switch")) {
            return SAY;
        }
        strcpy(current_channel, "");
        for (i = 0; i < (strlen(txt) - strlen("/switch ")); i++) {
            if (i == CHANNEL_MAX) {
                current_channel[i] == '\0';
                break;
            }
            current_channel[i] = txt[strlen("/switch ")+i];
        }
        return SWITCH;
    }
    return SAY;
}

void client_data_handler(char txt[], c_type ct) {
    /* Stores appropriate data into cd
     * Sends cd to server
     */
    int retcode;
    CData *cd = (CData *)malloc(sizeof(CData));
    //if (ct = WHO) {
        //strcpy(cd->channel, temp_channel);
    //}
    //else {
        strcpy(cd->channel, current_channel);
    //}
    strcpy(cd->username, user_name);
    strcpy(cd->message, txt);
    printf("%d\n", ct);
    cd->type = ct;
    printf("Send\n");
    retcode = sendto(sockid, cd, sizeof(CData), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retcode <= -1) {
        printf("Client failed to send appropriate data to server\n");
    }
    free(cd);
}

void server_data_handler() {
    /* Receives data from server
     * Handles sd depending on type
     */
    int nread;
    unsigned int len = (unsigned int)sizeof(struct sockaddr_in);
    SData sd;
    printf("Start receive\n");
    nread = recvfrom(sockid, &sd, sizeof(SData), 0, (struct sockaddr *)&from, &len);
    printf("Finished\n");
    if (nread > 0) {
        if (sd.type == S_SAY) {
            printf("[%s][%s]%s", sd.channel, sd.username, sd.message);
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
        else if (sd.type == S_ERROR) {
            printf("Error: %s", sd.message);
        }
    }
    else {
        printf("Error in receive from server\n");
    }
    //free(sd);
}

int main(UNUSED int argc, char *argv[]) {
    char txt[MESSAGE_MAX+1];
    c_type ct;
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
    
    strcpy(user_name, argv[3]);
    
    client_login();
    
    if (raw_mode() == -1) {
        printf("Raw mode failed\n");
        return -1;
    }
    
    while (1) {
        get_char_input(txt);
        ct = setType(txt);
        if (ct == SWITCH) {
            continue;
        }
        client_data_handler(txt, ct);
        if (ct == LOGOUT) {
            break;
        }
        server_data_handler();
    }
    
    cooked_mode();
    shutdown(sockid, 2);
    return 0;
}
