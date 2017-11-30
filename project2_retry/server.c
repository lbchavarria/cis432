/* The List struct and funtions, initList(), destroyList(), and insertList() are not mine
 * The above struct and functions are based on code written by Peter Dickman and revised by Joe Sventek
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "chatroom.h"

#define UNUSED __attribute__ ((unused))

typedef struct user {
    char username[USERNAME_MAX];
    struct sockaddr_in user_addr;
    int isActive;
} User;

typedef struct server_info {
    sockaddr_in server_addr;
} SInfo;

typedef struct list {
    int size;
    int pos;
    int rpos; //might not need
    int isEmpty;
    void **buffer;
} List;

typedef struct channel {
    char name[CHANNEL_MAX];
    List *user_list;
} Channel;

// Global variables
int sockid;
struct sockaddr_in client_addr;
List *channel_list, *server_list;

List *initList(int size) {
    /* Initialize list of length size
     * List will wither hold User items or Channel items
     */
    List *newlist = (List *)malloc(sizeof(List));
    newlist->buffer = (void **)malloc(size*sizeof(void *));
    
    newlist->size = size;
    newlist->pos = 0;
    newlist->rpos = size-1; // might nit need
    newlist->isEmpty = 1;
    
    return newlist;
}

void destroyList(List *list) {
    /* Free buffer inside list
     * Free list
     */
    free(list->buffer);
    free(list);
}

int insertList(List *list, void *item) {
    /* Write item into buffer of the list
     * Item will either be User or Channel
     * Successful write will return 1;
     * If list buffer is full, item will not be written to list and return 0
     */
    if (list->pos == list->size) {
        printf("Cannot add any more items to list\n");
        return 0;
    }
    list->buffer[list->pos++] = item;
    return 1;
}

List *initServerList(int arg, char *args[]) {
    int size, i;
    SInfo s_info;
    size = ((arg-2) - (arg%2))/2;
    List *newlist = initList(size);
    bzero((char *)&(s_info.server_addr), sizeof(s_info.server_addr));
    s_info.server_addr.sin_family = AF_INET;
    s_info.server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    for (i = 3; i < arg; i++) {
        s_info.server_addr.sin_port = htons(atoi(args[++i]));
        insertList(newlist, &s_info);
    }
}

void timer_handler(int signum) {
    static int count = 0;
    static char temp_channel[CHANNEL_MAX];
    
}

void send_error(SData *sd) {
    
}

void send_say(SData *sd) {
    printf("Start send\n");
    int retcode, i, j;
    for (i = 0; i < channel_list->pos; i++) {
        if (strcmp(sd->channel, ((Channel *)channel_list->buffer[i])->name) == 0) {
            printf("Found channel\n");
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->pos; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive) {
                    printf("Attempting to send\n");
                    retcode = sendto(sockid, sd, sizeof(SData), 0, (struct sockaddr *)&(((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr), sizeof(((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr));
                    if (retcode <= -1) {
                        printf("Message failed to send to user %s\n", ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->username);
                    }
                    printf("Sent\n");
                }
            }
            break;
        }
    }
    printf("Done\n");
}

void user_login(CData *cd) {
    int i;
    User new_user;
    strcpy(new_user.username, cd->username);
    new_user.user_addr = client_addr;
    new_user.isActive = 1;
    if (channel_list->isEmpty) {
        Channel new_channel;
        strcpy(new_channel.name, "Common");
        new_channel.user_list = initList(50);
        if (insertList(channel_list, &new_channel) == 0) {
            printf("Cannot join default channel Common since max number of channels have been created\n");
            //Attempt to join other channels
            //If cannot join any channels send error message and return
        }
    }
    for (i = 0; i < channel_list->pos; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, "Common") == 0) {
            if (insertList(((Channel *)channel_list->buffer[i])->user_list, &new_user) == 0) {
                //send error
            }
            break;
        }
    }
}

void user_logout() {
    int i, j;
    for (i = 0; i < channel_list->pos; i++) {
        for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->pos; j++) {
            if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive = 0;
            }
        }
    }
}

void user_join(CData *cd) {
    int i, j, k;
    int done = 0;
    User *temp_user;
    for (i = 0; i < channel_list->pos; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, cd->channel) == 0) {
            for (j = 0; j < channel_list->pos; j++) {
                for (k = 0; k < ((Channel *)channel_list->buffer[j])->user_list->pos; k++) {
                    if (((User *)((Channel *)channel_list->buffer[j])->user_list->buffer[k])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                        temp_user = ((User *)((Channel *)channel_list->buffer[j])->user_list->buffer[k]);
                        done = 1;
                        break;
                    }
                }
                if (done) {
                    break;
                }
            }
            if (insertList(((Channel *)channel_list->buffer[i])->user_list, temp_user) == 0) {
                //send error of channel being full
            }
            return;
        }
    }
    Channel new_channel;
    strcpy(new_channel.name, cd->channel);
    new_channel.user_list = initList(50);
    for (i = 0; i < channel_list->pos; i++) {
        for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->pos; j++) {
            if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                temp_user = ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j]);
                done = 1;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    if (insertList(new_channel.user_list, temp_user) == 0) {
        //send error
    }
    if (insertList(channel_list, &new_channel) == 0) {
        //send error that channel_list is full
    }
}

void user_leave(CData *cd) {
    int i, j;
    for (i = 0; i < channel_list->pos; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, cd->channel) == 0) {
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->pos; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                    ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive = 0;
                    return;
                }
            }
        }
    }
}

void user_say(CData *cd) {
    printf("Start Say\n");
    int i, j;
    int done = 0;
    SData *sd = (SData *)malloc(sizeof(SData));
    sd->type = S_SAY;
    strcpy(sd->channel, cd->channel);
    strcpy(sd->message, cd->message);
    printf("Enter for loop\n");
    for (i = 0; i < channel_list->pos; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, cd->channel) == 0) {
            printf("Found channel\n");
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->pos; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                    strcpy(sd->username, ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->username);
                    done = 1;
                    printf("Obtained\n");
                    break;
                }
            }
            if (done) {
                break;
            }
        }
    }
    printf("Send Say\n");
    send_say(sd);
}

void client_data_handler() {
    int nread;
    void *cd;
    unsigned int len = (unsigned int)sizeof(struct sockaddr_in);
    
    nread = recvfrom(sockid, (CData *)&cd, sizeof(CData), 0, (struct sockaddr *)&client_addr, &len);
    if (nread > 0) {
        if ((CData *)&cd->type == LOGIN) {
            user_login(cd);
        }
        else if ((CData *)&cd->type == LOGOUT) {
            user_logout();
        }
        else if ((CData *)&cd->type == JOIN) {
            user_join(cd);
        }
        else if ((CData *)&cd->type == LEAVE) {
            user_leave(cd);
        }
        else if ((CData *)&cd->type == SAY) {
            user_say(cd);
        }
    }
    else {
        printf("Failed to receive data from client");
    }
    free(cd);
}

int main(UNUSED int argc, char *argv[]) { // Multiserver implementation will have UNUSED be removed
    struct sockaddr_in my_addr;
    struct sigaction sa;
    struct itimerval timer;
    channel_list = initList(20);
    
    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        printf("Socket setup failed\n");
        return -1;
    }
    
    bzero((char *)&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htons(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        printf("Server bind failed");
        return -1;
    }
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    
    timer.it_value.tv_sec = 60;
    timer.it_value.tv_usec = 0;
    
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    
    while (1) {
        client_data_handler();
    }
    
    return 0;
}
