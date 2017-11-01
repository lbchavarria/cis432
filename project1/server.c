#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "duckchat.h"

void destroy_user(User user) {
    int i;
    for (i = 0; i < user.nsub; i++) {
        free(user.sub_channels[i]);
    }
    free(user.sub_channels);
    //user = NULL;
}

void destroy_user_list(UserList user_list) {
    int i;
    for (i = 0; i < user_list.size; i++) {
        destroy_user(user_list.list[i]);
    }
    free(user_list.list);
}

void destroy_channel(Channel channel) {
    destroy_user_list(channel.txt_users);
}

void destroy_channel_list(ChannelList channel_list) {
    int i;
    for (i = 0; i < channel_list.size; i++) {
        destroy_channel(channel_list.list[i]);
    }
    free(channel_list.list);
}

int main(int argc, char *argv[]) {
    int sockid, nread, addrlen, i, j, k, l, m, ch_exist;
    int err = 0;
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

    int nusers = 0;
    struct request req;
    struct channel_info current_channel;
    struct user_info user_i;
    struct text txt;
    struct text_list tlist;// = (struct text_list *)malloc(sizeof(struct text_list));
    //list->txt_nchannels = 1;
    struct text_who who;// = (struct text_who *)malloc(sizeof(struct text_who));
    //who->txt_nusernames = 0;
    Channel channel;
    //channel.nchannels = 0;
    //channel.nusers = 0;
    channel.user_size = 10;
    channel.txt_users = (User *)malloc(sizeof(User)*channel.user_size);
    for (i = 0; i < channel.user_size; i++) {
        channel.txt_users[i] = NULL;
    }
    ChannelList channel_list;
    channel_list.size = 0;
    channel_list.list = (Channel *)malloc(sizeof(Channel));
    UserList user_list;
    user_list.size = 1;
    user_list.list = (User *)malloc(sizeof(User)*size);
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
        nread = recvfrom(sockid, req, sizeof(request), 0, (struct sockaddr *) &client_addr, &addrlen);
        if (nread > 0) {
            if (req.req_type == REQ_LOGIN) {
                struct request_login *req_login = (request_login *)&req;
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
                struct request_logout *req_logout = (request_logout *)&req;
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
                struct request_join *req_join = (request_join *)&req;
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
                struct request_leave *req_leave = (request_leave *)&req;
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i]->client_addr == client_addr) {
                        
                    }
                }
            }
            else if (req.req_type == REQ_SAY) {
                struct request_say *req_say = (request_say *)&req;
                txt.text_type = TXT_SAY;
            }
            else if (req.req_type == REQ_LIST) {
                struct request_list *req_list = (request_list *)&req;
                txt.text_type = TXT_LIST;
            }
            else if (req.req_type == REQ_WHO) {
                struct request_who *req_who = (request_who *)&req;
                txt.text_type = TXT_WHO;
            }
        }
    }

    //free(list);
    //free(who);
    
    destroy_channel_list(channel_list);
    destroy_channel(channel);
    destroy_user_list(user_list);
    destroy_user(user);
    close(sockid);
    return 0;
}
