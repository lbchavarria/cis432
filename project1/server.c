#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "duckchat.h"

typedef struct user {
    int subsize;
    int nsub;
    struct sockaddr_in client_addr;
    char username[USERNAME_MAX];
    char current_channel[CHANNEL_MAX];
    char **sub_channels;//[CHANNEL_MAX];
} User;

//added
typedef struct user_list {
    int size;
    User *list;
} UserList;

//added
typedef struct channel {
    int  user_size;
    char txt_channel[CHANNEL_MAX];
    UserList txt_users;
} Channel;

//added
typedef struct channel_list {
    int size;
    Channel *list;
} ChannelList;

int sockid;
struct sockaddr_in client_addr;
ChannelList channel_list;
UserList user_list;
struct request req

void destroy_user(User user) {
    int i;
    for (i = 0; i < user.nsub; i++) {
        free(user.sub_channels[i]);
    }
    free(user.sub_channels);
    //user = NULL;
}

void destroy_user_list(UserList list) {
    int i;
    for (i = 0; i < list.size; i++) {
        destroy_user(list.list[i]);
    }
    free(list.list);
}

void destroy_channel(Channel channel) {
    destroy_user_list(channel.txt_users);
}

void destroy_channel_list(ChannelList list) {
    int i;
    for (i = 0; i < list.size; i++) {
        destroy_channel(list.list[i]);
    }
    free(list.list);
}

void error_handler(char txt[]) {
    int retcode;
    struct text_error txt_error;
    strcpy(txt_error.txt_error, txt);
    retcode = sendto(sockid, (void *)&txt_error, sizeof(void *), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
    if (retcode <= -1) {
        printf("Server: sendto failed: %d\n", errno);
    }
}

void text_handler(struct text txt) {
    int i, j, k, ch_exist, retcode;
    if (txt.txt_type == TXT_SAY) {
        struct request_say *req_say = (struct request_say *)&req;
        struct text_say txt_say = (struct text_say)txt;
        strcpy(txt_say.txt_channel, req_say->req_channel);
        strcpy(txt_say.txt_text, req_say->req_text);
        for (i = 0; i < user_list.size; i++) {
            if (user_list.list[i]->client_addr == client_addr) {
                if (strcmp(user_list.list[i]->current_channel, txt_say.txt_channel) != 0) {
                    strcpy(user_list.list[i]->current_channel, txt_say.txt_channel);
                }
                strcpy(txt_say.txt_username, user_list.list[i]->username);
                break;
            }
        }
        for (i = 0; i < channel_list.size; i++) {
            if (strcmp(channel_list.list[i]->txt_channel, txt_say.txt_channel) == 0) {
                ch_exist = 1;
                for (j = 0; j < channel_list.list[i]->txt_users.size; j++) {
                    retcode = sendto(sockid, (void *)&txt_say, sizeof(void *), 0, (struct sockaddr *) &channel_list.list[i]->txt_users.list[j]->client_addr, sizeof(channel_list.list[i]->txt_users.list[j]->client_addr));
                    if (retcode <= -1) {
                        printf("Server: sendto failed to user %s: %d\n", channel_list.list[i]->txt_users.list[j]->username, errno);
                    }
                }
                break;
            }
            else {
                ch_exist = 0;
            }
        }
        if (!ch_exist) {
            //send error
            char errtxt[SAY_MAX];
            strcpy(errtxt, "Channel does not exist\n");
            error_handler(errtxt);
        }
        return;
    }
    else if (txt.txt_type == TXT_LIST) {
        struct request_list *req_list = (struct request_list *)&req;
        struct text_list txt_list = (struct text_list)txt;
        txt_list.txt_nchannels = channel_list.size;
        struct channel_info channels[txt_list.txt_nchannels];
        for (i = 0; i < txt_list.txt_nchannels; i++) {
            strcpy(channels[i].ch_channel, channel_list.list[i]->txt_channel);
        }
        txt_list.txt_channels = channels;
        retcode = sendto(sockid, (void *)&txt_list, sizeof(void *), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
        if (retcode <= -1) {
            printf("Server: sendto failed: %d\n", errno);
        }
        return;
    }
    else if (txt.txt_type == TXT_WHO) {
        k = 0;
        struct request_who *req_who = (struct request_who *)&req;
        struct text_who txt_who = (struct text_who)txt;
        strcpy(txt_who.txt_channel, req_who->req_channel);
        for (i = 0; i < channel_list.size; i++) {
            if (strcmp(txt_who.txt_channel, channel_list.list[i]->txt_channel) == 0) {
                ch_exist = 1;
                txt_who.txt_nusernames = channel_list.list[i]->user_size;
                struct user_info users[txt_who.txt_nusernames];
                for (j = 0; j < channel_list.list[i]->txt_users.size; j++) {
                    if (channel_list.list[i]->txt_users.list[j] != NULL && k < txt_who.txt_nusernames) {
                        strcpy(users[k++], channel_list.list[i]->txt_users.list[j]->username);
                    }
                    if (k >= txt_who.txt_nusernames) {
                        break;
                    }
                }
                txt_who.txt_users = users;
                break;
            }
            else {
                ch_exist = 0;
            }
        }
        if (!ch_exist) {
            //send error
            char errtxt[SAY_MAX];
            strcpy(errtxt, "Channel does not exist\n");
            error_handler(errtxt);
        }
        retcode = sendto(sockid, (void *)&txt_list, sizeof(void *), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
        if (retcode <= -1) {
            printf("Server: sendto failed: %d\n", errno);
        }
        return;
    }
    
}

int main(int argc, char *argv[]) {
    int nread, i, j, k, l, m, ch_exist;
    int err = 0;
    struct sockaddr_in my_addr;

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

    int nusers = 0;
    //struct request req;
    //struct channel_info current_channel;
    //struct user_info user_i;
    struct text txt;
    //struct text_list tlist;// = (struct text_list *)malloc(sizeof(struct text_list));
    //list->txt_nchannels = 1;
    //struct text_who who;// = (struct text_who *)malloc(sizeof(struct text_who));
    //who->txt_nusernames = 0;
    Channel channel;
    //channel.nchannels = 0;
    //channel.nusers = 0;
    channel.user_size = 10;
    channel.txt_users = (User *)malloc(sizeof(User)*channel.user_size);
    for (i = 0; i < channel.user_size; i++) {
        channel.txt_users[i] = NULL;
    }
    
    channel_list.size = 0;
    channel_list.list = (Channel *)malloc(sizeof(Channel));
    
    user_list.size = 1;
    user_list.list = (User *)malloc(sizeof(User)*user_list.size);
    for (i = 0; i < user_list.size; i++) {
        user_list.list[i] = NULL;
    }
    User user, temp_user;
    user.subsize = 1
    user.nsub = 0;
    user.sub_channels = (char **)malloc(sizeof(char *)*subsize);
    for (i = 0; i < subsize; i++) {
        user.sub_channels[i] = (char *)malloc(sizeof(char)*CHANNEL_MAX);
    }

    while(1) {
        nread = recvfrom(sockid, req, sizeof(struct request), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
        if (nread > 0) {
            if (req.req_type == REQ_LOGIN) {
                struct request_login *req_login = (struct request_login *)&req;
                strcpy(user.username, req_login->req_username);
                strcpy(user.current_channel, "Common");
                user.client_addr = client_addr;
                strcpy(user.sub_channels[0], "Common");
                user.nsub++;
                
                //strcpy(user_i.us_username, req_login.req_username);
                //strcpy(current_channel.ch_channel, "Common");
                //list->txt_channels[list->txt_nchannels-1] = current_channel;
                
                strcpy(channel.txt_channel, "Common");
                
                if (channel_list.size == 0) {
                    channel_list.size = 1;
                    channel_list.list[0] = channel;
                    //strcpy(channel[0]->txt_channel, "Common");
                }
                nusers++;
                if (nusers > user_list.size) {
                    user_list.size = nusers;
                    user_list.list = realloc(user_list.list, sizeof(Users)*user_list.size);
                    user_list.list[user_list.size-1] = NULL;
                }
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i] == NULL) {
                        user_list.list[i] = user;
                        break;
                    }
                }
                if (nusers > channel_list.list[0]->txt_users->size) {
                    channel_list.list[0]->user_size = nusers;
                    channel_list.list[0]->txt_users = realloc(channel_list.list[0]->txt_users, sizeof(Users)*channel_list.list[0]->user_size);
                    channel_list.list[0]->txt_users[user_size-1] = NULL;
                }
                for (i = 0; i < channel_list.list[0]->user_size; i++) {
                    if (channel_list.list[0]->txt_users[i] == NULL) {
                        channel_list.list[0]->txt_users[i] = user;
                        break;
                    }
                }
            }
            else if (req.req_type == REQ_LOGOUT) {
                struct request_logout *req_logout = (struct request_logout *)&req;
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i]->client_addr == client_addr) {
                        temp_user = user_list.list[i];
                        for (j = 0; j < temp_user.nsub; j++) {
                            for (k = 0; k < channel_list.size; k++) {
                                if (strcmp(channel_list.list[k]->txt_channel, temp_user.sub_channels[j]) == 0) {
                                    for (l = 0; l < channel_list.list[k]->txt_users->size; l++) {
                                        if (channel_list.list[k]->txt_users[l] == temp_user) {
                                            destroy_user(channel_list[k]->txt_users[l]);
                                            channel_list[k]->txt_users[l] = NULL;
                                        }
                                    }
                                }
                            }
                        }
                        destroy_user(user_list.list[i]);
                        user_list.list[i] = NULL;
                        destroy_user(temp_user);
                        temp_user = NULL;
                        nusers--;
                        break;
                    }
                }
            }
            else if (req.req_type == REQ_JOIN) {
                struct request_join *req_join = (struct request_join *)&req;
                ch_exist = 0;
                strcpy(channel.txt_channel, req_join->req_channel);
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i]->client_addr == client_addr) {
                        temp_user = user_list.list[i];
                        break;
                    }
                }
                for (i = 0; i < channel_list.size; i++) {
                    if (strcmp(channel_list.list[i]->txt_channel, channel.txt_channel) == 0) {
                        ch_exist = 1;
                        for (j = 0; j < channel_list.list[i]->txt_users.size; j++) {
                            if (channel_list.list[i]->txt_users.list[j]->client_addr == temp_user.client_addr) {
                                err = 1;
                                //send error
                                char errtxt[SAY_MAX];
                                strcpy(errtxt, "User already subscribed to channel\n");
                                error_handler(errtxt);
                                break;
                            }
                        }
                        if (!err) {
                            channel_list.list[i]->user_size++;
                            if (channel_list.list[i]->user_size > channel_list.list[i]->txt_users.size) {
                                channel_list.list[i]->txt_users.size = channel_list.list[i]->user_size;
                                channel_list.list[i]->txt_users = realloc(channel_list.list[i]->txt_users, sizeof(User)*channel_list.list[i]->txt_users.size);
                                channel_list.list[i]->txt_users.list[channel_list.list[i]->txt_users.size-1] = NULL;
                            }
                            for (j = 0; j < channel_list.list[i]->txt_users.size; j++) {
                                if (channel_list.list[i]->txt_users.list[j] == NULL) {
                                    channel_list.list[i]->txt_users.list[j] = temp_user;
                                    strcpy(channel_list.list[i]->txt_users.list[j].current_channel, channel.txt_channel);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else {
                        ch_exist = 0;
                    }
                }
                if (!ch_exist) {
                    for (i = 0; i < user_list.size; i++) {
                        if (user_list.list[i]->client_addr == client_addr) {
                            for (j = 0; user_list.list[i]->subsize; j++) {
                                if (strcmp(user_list.list[i]->sub_channels[j], channel.txt_channel) == 0) {
                                    err = 1;
                                    //send error
                                    char errtxt[SAY_MAX];
                                    strcpy(errtxt, "User already subscribed to channel\n");
                                    error_handler(errtxt);
                                    break;
                                }
                            }
                            if (!err) {
                                user_list.list[i]->nsub++;
                                if (user_list.list[i]->nsub > user_list.list[i]->subsize) {
                                    user_list.list[i]->subsize = user_list.list[i]->nsub;
                                    user_list.list[i]->sub_channels = realloc(user_list.list[i]->sub_channels, sizeof(char *)*user_list.list[i]->subsize);
                                    user_list.list[i]->sub_channels[subsize-1] = (char *)malloc(sizeof(char)*CHANNEL_MAX);
                                    strcpy(user_list.list[i]->sub_channels[subsize-1], channel.txt_channel);
                                }
                                else {
                                    for (j = 0; j < user_list.list[i]->subsize; j++) {
                                        if (strcmp(user_list.list[i]->sub_channels[j], "") == 0) {
                                            strcpy(user_list.list[i]->sub_channels[j], channel.txt_channel);
                                        }
                                    }
                                }
                            }
                            channel_list.size++;
                            channel_list.list = realloc(channel_list.list, sizeof(Channel)*channel_list.size);
                            channel_list.list[channel_list.size-1] = channel;
                            channel_list.list[channel_list.size-1]->txt_users[0] = temp_user;
                            strcpy(channel_list.list[channel_list.size-1]->txt_users[0]->current_channel, channel.txt_channel);
                            break;
                        }
                    }
                }
                err = 0;
                destroy_user(temp_user);
            }
            else if (req.req_type == REQ_LEAVE) {
                struct request_leave *req_leave = (struct request_leave *)&req;
                ch_exist = 0;
                strcpy(channel.txt_channel, req_leave->req_channel);
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i]->client_addr == client_addr) {
                        for (j = 0; j < channel_list.size; j++) {
                            if (strcmp(channel_list.list[j]->txt_channel, channel.txt_channel) == 0) {
                                ch_exist = 1;
                                for (k = 0; k < channel_list.list[j]->txt_users.size; k++) {
                                    if (channel_list.list[j]->txt_users.list[k]->client_addr == client_addr) {
                                        channel_list.list[j]->txt_users.list[k] = NULL;
                                        channel_list.list[j]->user_size--;
                                        break;
                                    }
                                }
                                break;
                            }
                            else {
                                ch_exist = 0;
                            }
                        }
                        if (!ch_exist) {
                            //send error
                            char errtxt[SAY_MAX];
                            strcpy(errtxt, "Channel does not exist\n");
                            error_handler(errtxt);
                            break;
                        }
                        for (j = 0; j < user_list.list[i]->subsize; j++) {
                            if (strcmp(user_list.list[i]->sub_channels[j], channel.txt_channel) == 0) {
                                strcpy(user_list.list[i]->sub_channels[j], "");
                                user_list.list[i]->nsub--;
                                ch_exist = 1;
                                break;
                            }
                            else {
                                ch_exist = 0;
                            }
                        }
                        if (!ch_exist) {
                            //send error
                            char errtxt[SAY_MAX];
                            strcpy(errtxt, "Channel does not exist\n");
                            error_handler(errtxt);
                            break;
                        }
                        if (strcmp(user_list.list[i]->current_channel, channel.txt_channel) == 0) {
                            strcpy(user_list.list[i]->current_channel, "");
                        }
                        break;
                    }
                }
            }
            else if (req.req_type == REQ_SAY) {
                
                txt.text_type = TXT_SAY;
                //retcode =
                text_handler(txt);
                
            }
            else if (req.req_type == REQ_LIST) {
                
                txt.text_type = TXT_LIST;
                //retcode =
                text_handler(txt);
            }
            else if (req.req_type == REQ_WHO) {
                
                txt.text_type = TXT_WHO;
                //retcode =
                text_handler(txt);
            }
        }
    }

    //free(list);
    //free(who);
    
    destroy_channel_list(channel_list);
    destroy_channel(channel);
    destroy_user_list(user_list);
    destroy_user(user);
    shutdown(sockid);
    return 0;
}
