#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "duckchat.h"

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
    int isEmpty;
    void **buffer;
} List;

typedef struct channel {
    char name[CHANNEL_MAX];
    List *user_list;
} Channel;

// Global Variables
int sockid;
struct sockaddr_in client_addr;
List *channel_list, *server_list;

List *initList(int size) {
    List *newlist = (List *)malloc(sizeof(List));
    newlist->buffer = (void **)malloc(size*sizeof(void *));
    newlist->size = size;
    newlist->pos = 0;
    newlist->isEmpty = 1;
    return newlist;
}

void destroyList(List *list) {
    free(list->buffer);
    free(list);
}

int insertList(List *list, void *item) {
    if (list->size == list->pos) {
        printf("List is full and could not insert item\n");
        return 0;
    }
    list->buffer[(list->pos)++] = item;
    list->isEmpty = 0;
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

void send_error(struct text_error t_error) {
    
}

void send_say(struct text_say t_say) {
    int retcode, i, j;
    for (i = 0; i < channel_list->size; i++) {
        if (strcmp(t_say.txt_channel, ((Channel *)channel_list->buffer[i])->name) == 0) {
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->size; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive) {
                    retcode = sendto(sockid, &t_say, sizeof(struct text_say), 0, (struct sockaddr *)&(((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr), sizeof(((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr));
                    if (retcode <= -1) {
                        printf("Message failed to send to user %s\n", ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->username);
                    }
                }
            }
            break;
        }
    }
}

void user_login(request_login *data) {
    int i;
    User new_user;
    strcpy(new_user.username, data->req_username);
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
    for (i = 0; i < channel_list->size; i++) {
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
    for (i = 0; i < channel_list->size; i++) {
        for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->size; j++) {
            if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive = 0;
            }
        }
    }
}

void user_join(request_join *data) {
    int i, j, k;
    int done = 0;
    User *temp_user;
    for (i = 0; i < channel_list->size; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, data->req_channel) == 0) {
            for (j = 0; j < channel_list->size; j++) {
                for (k = 0; k < ((Channel *)channel_list->buffer[j])->user_list->size; k++) {
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
    strcpy(new_channel.name, data->req_channel);
    new_channel.user_list = initList(50);
    for (i = 0; i < channel_list->size; i++) {
        for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->size; j++) {
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

void user_leave(request_leave *data) {
    int i, j;
    for (i = 0; i < channel_list->size; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, data->req_channel) == 0) {
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->size; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                    ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->isActive = 0;
                    return;
                }
            }
        }
    }
}

void user_say(request_say *data) {
    int i, j;
    int done = 0;
    struct text_say t_say;
    t_say.txt_type = TXT_SAY;
    strcpy(t_say.txt_channel, data->req_channel);
    strcpy(t_say.txt_text, data->req_text);
    for (i = 0; i < channel_list->size; i++) {
        if (strcmp(((Channel *)channel_list->buffer[i])->name, data->req_channel) == 0) {
            for (j = 0; j < ((Channel *)channel_list->buffer[i])->user_list->size; j++) {
                if (((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->user_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                    strcpy(t_say.txt_username, ((User *)((Channel *)channel_list->buffer[i])->user_list->buffer[j])->username);
                    done = 1;
                    break;
                }
            }
            if (done) {
                break;
            }
        }
    }
    send_say(t_say);
    
}

/*void user_list(request_list *data) {
    
}

void user_who(request_who *data) {
    
}*/

void client_data_handler() {
    int nread;
    char r_txt[65536];
    void *data;
    int len = sizeof(struct sockaddr_in);
    
    nread = recvfrom(sockid, data, sizeof(r_txt), 0, (struct sockaddr *)&client_addr, &len);
    if (nread > 0) {
        if (((struct request *)&data)->req_type == REQ_LOGIN) {
            user_login((struct request_login *)&data);
        }
        else if (((struct request *)&data)->req_type == REQ_LOGOUT) {
            user_logout();
        }
        else if (((struct request *)&data)->req_type == REQ_JOIN) {
            user_join((struct request_join *)&data);
        }
        else if (((struct request *)&data)->req_type == REQ_LEAVE) {
            user_leave((struct request_leave *)&data);
        }
        else if (((struct request *)&data)->req_type == REQ_SAY) {
            user_say((struct request_say *)&data);
        }
        /*else if (((struct request *)&data)->req_type == REQ_LIST) {
            user_list((struct request_list *)&data);
        }
        else if (((struct request *)&data)->req_type == REQ_WHO) {
            user_who((struct request_who *)&data);
        }*/
    }
    else {
        printf("Failed to receive data from client");
    }
}

int main(int argc, char *argv[]) {
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
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockid, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        printf("Server bind failed\n");
        return -1;
    }
    
    if (argc > 2) {
        server_list = initServerList(argc, argv);
    }
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    
    timer.it_value.tv_sec = 60;
    timer.it_value.tv_usec = 0;
    
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    
    while(1) {
        client_data_handler();
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return 0;
}
