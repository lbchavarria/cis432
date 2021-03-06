#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "duckchat.h"

#define UNUSED __attribute__ ((unused))
#define C_MAXSIZE 20
#define U_MAXSIZE 100
#define M_MAXSIZE 65536

char r_txt[M_MAXSIZE];

typedef struct user {
    int isempty;
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
    int nchannels;
    int size;
    Channel *list;
} ChannelList;

int sockid;
struct sockaddr_in client_addr;
ChannelList channel_list;
UserList user_list;
void *r;
struct request *req;

void destroy_user(User user) {
    int i;
    for (i = 0; i < user.subsize; i++) {
        if (user.sub_channels[i] != NULL) {
            free(user.sub_channels[i]);
            user.sub_channels[i] = NULL;
        }
    }
    if (user.sub_channels != NULL) {
        free(user.sub_channels);
        user.sub_channels = NULL;
    }
    //user = NULL;
}

void destroy_user_list(UserList list) {
    int i;
    for (i = 0; i < list.size; i++) {
            destroy_user(list.list[i]);
    }
    if (list.list != NULL) {
        free(list.list);
        list.list = NULL;
    }
}

void destroy_channel(Channel channel) {
    destroy_user_list(channel.txt_users);
}

void destroy_channel_list(ChannelList list) {
    /*int i;
    for (i = 0; i < list.size; i++) {
        destroy_channel(list.list[i]);
    }*/
    if (list.list != NULL) {
        free(list.list);
        list.list = NULL;
    }
}

void error_handler(char *txt) {
    int retcode;
    //printf("Error\n");
    struct text_error *txt_error;
    strcpy(txt_error->txt_error, txt);
    txt_error->txt_type = TXT_ERROR;
    retcode = sendto(sockid, txt_error, sizeof(struct text_error *), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
    if (retcode <= -1) {
        perror("Server: sendto failed");
    }
}

int text_handler(struct text txt) {
    int i, j, k, ch_exist, retcode, err;
    if (txt.txt_type == TXT_SAY) {
        struct request_say *req_say = (struct request_say *)&req;
        struct text_say txt_say;// = (struct text_say *)&txt;
        txt_say.txt_type = TXT_SAY;
        strcpy(txt_say.txt_channel, req_say->req_channel);
        //printf("%s\n", txt_say.txt_channel);
        strcpy(txt_say.txt_text, req_say->req_text);
        //printf("%s\n", txt_say.txt_text);
        for (i = 0; i < user_list.size; i++) {
            if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                if (strcmp(user_list.list[i].current_channel, txt_say.txt_channel) != 0) {
                    err = 0;
                    strcpy(user_list.list[i].current_channel, txt_say.txt_channel);
                }
                strcpy(txt_say.txt_username, user_list.list[i].username);
                break;
            }
            else {
                err = 1;
            }
        }
        if (err) {
            return err;
        }
        //printf("%s\n", txt_say.txt_username);
        for (i = 0; i < channel_list.size; i++) {
            if (strcmp(channel_list.list[i].txt_channel, txt_say.txt_channel) == 0) {
                ch_exist = 1;
                for (j = 0; j < channel_list.list[i].txt_users.size; j++) {
                    if (channel_list.list[i].txt_users.list[j].isempty == 0) {
                        retcode = sendto(sockid, (struct text *)&txt_say, sizeof(struct text), 0, (struct sockaddr *) &(channel_list.list[i].txt_users.list[j].client_addr), sizeof(channel_list.list[i].txt_users.list[j].client_addr));
                        if (retcode <= -1) {
                            perror("Server: sendto failed to user");
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
            //send error
            char errtxt[SAY_MAX];
            strcpy(errtxt, "Channel does not exist\n");
            error_handler(errtxt);
        }
        return err;
    }
    else if (txt.txt_type == TXT_LIST) {
        //struct request_list *req_list = (struct request_list *)&req;
        struct text_list txt_list;// = (struct text_list *)&txt;
        for (i = 0; i < user_list.size; i++) {
            if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                err = 0;
            }
            else {
                err = 1;
            }
        }
        if (err) {
            return err;
        }
        txt_list.txt_type = TXT_LIST;
        txt_list.txt_nchannels = channel_list.size;
        struct channel_info i_channels[txt_list.txt_nchannels];
        for (i = 0; i < txt_list.txt_nchannels; i++) {
            strcpy(i_channels[i].ch_channel, channel_list.list[i].txt_channel);
        }
        memcpy(txt_list.txt_channels, i_channels, sizeof(i_channels));
        retcode = sendto(sockid, (struct text *)&txt_list, sizeof(struct text), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
        if (retcode <= -1) {
            perror("Server: sendto failed");
        }
        return err;
    }
    else if (txt.txt_type == TXT_WHO) {
        k = 0;
        struct request_who *req_who = (struct request_who *)&req;
        for (i = 0; i < user_list.size; i++) {
            if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                err = 0;
            }
            else {
                err = 1;
            }
        }
        if (err) {
            return err;
        }
        struct text_who txt_who;// = (struct text_who *)&txt;
        txt_who.txt_type = TXT_WHO;
        strcpy(txt_who.txt_channel, req_who->req_channel);
        for (i = 0; i < channel_list.size; i++) {
            if (strcmp(txt_who.txt_channel, channel_list.list[i].txt_channel) == 0) {
                ch_exist = 1;
                txt_who.txt_nusernames = channel_list.list[i].user_size;
                struct user_info users[txt_who.txt_nusernames];
                for (j = 0; j < channel_list.list[i].txt_users.size; j++) {
                    if (!channel_list.list[i].txt_users.list[j].isempty && k < txt_who.txt_nusernames) {
                        strcpy(users[k++].us_username, channel_list.list[i].txt_users.list[j].username);
                    }
                    if (k >= txt_who.txt_nusernames) {
                        break;
                    }
                }
                memcpy(txt_who.txt_users, users, sizeof(users));
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
        retcode = sendto(sockid, (struct text *)&txt_who, sizeof(struct text), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
        if (retcode <= -1) {
            perror("Server: sendto failed");
        }
        return err;
    }
    return 0;
}

int main(UNUSED int argc, char *argv[]) {
    int nread, i, j, k, l, ch_exist;
    int err = 0;
    struct sockaddr_in my_addr;

    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        perror("Server: socket error");
        exit(0);
    }

    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htons(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("Server: bind fail");
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
    channel.user_size = 0;
    channel.txt_users.size = U_MAXSIZE;
    channel.txt_users.list = (User *)malloc(sizeof(User)*channel.txt_users.size);
    for (i = 0; i < channel.txt_users.size; i++) {
        channel.txt_users.list[i].isempty = 1;
    }
    
    
    channel_list.size = C_MAXSIZE;
    channel_list.nchannels = 1;
    channel_list.list = (Channel *)malloc(sizeof(Channel));
    
    user_list.size = U_MAXSIZE;
    user_list.list = (User *)malloc(sizeof(User)*user_list.size);
    for (i = 0; i < user_list.size; i++) {
        user_list.list[i].isempty = 1;
    }
    User user;
    int temp_user;
    user.isempty = 0;
    user.subsize = C_MAXSIZE;
    user.nsub = 0;
    user.sub_channels = (char **)malloc(sizeof(char *)*user.subsize);
    for (i = 0; i < user.subsize; i++) {
        user.sub_channels[i] = (char *)malloc(sizeof(char)*CHANNEL_MAX);
    }
    unsigned int len = (unsigned int)sizeof(struct sockaddr_in);
    char errtxt[SAY_MAX];

    while(1) {
        //printf("Receive\n");
        r = &r_txt;
        nread = recvfrom(sockid, r, sizeof(r_txt), 0, (struct sockaddr *) &client_addr, &len);
        req = (struct request *)&r;
        printf("%d\n", nread);
        if (nread > 0) {
            //printf("Reveived\n");
            //printf("%d\n", req.req_type);
            //struct request *req = (struct request *)&reqs;
            //printf("%d\n", req->req_type);
            if (req->req_type == REQ_LOGIN) {
                //printf("Login\n");
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
                
                channel_list.list[0] = channel;
                /*if (channel_list.size == 0) {
                    channel_list.size = 1;
                    
                    //strcpy(channel[0]->txt_channel, "Common");
                }*/
                nusers++;
                if (nusers > user_list.size) {
                    //send error
                    err = 1;
                    strcpy(errtxt, "Max amount of users have logged in\n");
                    error_handler(errtxt);
                    /*user_list.size = nusers;
                    user_list.list = realloc(user_list.list, sizeof(User)*user_list.size);
                    user_list.list[user_list.size-1].isempty = 1;*/
                }
                if (!err) {
                    for (i = 0; i < user_list.size; i++) {
                        if (user_list.list[i].isempty) {
                            user_list.list[i] = user;
                            break;
                        }
                    }
                    if (nusers > channel_list.list[0].txt_users.size) {
                        err = 1;
                        strcpy(errtxt, "Max amount of users have logged in\n");
                        error_handler(errtxt);
                        /*channel_list.list[0].user_size = nusers;
                        channel_list.list[0].txt_users.list = realloc(channel_list.list[0].txt_users.list, sizeof(User)*channel_list.list[0].user_size);
                        channel_list.list[0].txt_users.list[channel_list.list[0].user_size-1].isempty = 1;*/
                    }
                    if (!err) {
                        for (i = 0; i < channel_list.list[0].txt_users.size; i++) {
                            if (channel_list.list[0].txt_users.list[i].isempty == 1) {
                                channel_list.list[0].txt_users.list[i] = user;
                                channel_list.list[0].user_size++;
                                break;
                            }
                        }
                    }
                }
            }
            else if (req->req_type == REQ_LOGOUT) {
                //printf("Logout\n");
                //struct request_logout *req_logout = (struct request_logout *)&req;
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                        //temp_user = user_list.list[i];
                        err = 0;
                        for (j = 0; j < user_list.list[i].nsub; j++) {
                            for (k = 0; k < channel_list.size; k++) {
                                if (strcmp(channel_list.list[k].txt_channel, user_list.list[i].sub_channels[j]) == 0) {
                                    for (l = 0; l < channel_list.list[k].txt_users.size; l++) {
                                        if (channel_list.list[k].txt_users.list[l].client_addr.sin_addr.s_addr == user_list.list[i].client_addr.sin_addr.s_addr) {
                                            //destroy_user(channel_list.list[k].txt_users.list[l]);
                                            channel_list.list[k].txt_users.list[l].isempty = 1;
                                        }
                                    }
                                }
                            }
                        }
                        //destroy_user(user_list.list[i]);
                        user_list.list[i].isempty = 1;
                        //destroy_user(temp_user);
                        //temp_user.isempty = 1;
                        nusers--;
                        break;
                    }
                    else {
                        err = 1;
                    }
                }
                if (err) {
                    //send error
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
            }
            else if (req->req_type == REQ_JOIN) {
                //printf("Join\n");
                struct request_join *req_join = (struct request_join *)&req;
                ch_exist = 0;
                strcpy(channel.txt_channel, req_join->req_channel);
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                        temp_user = i;
                        err = 0;
                        break;
                    }
                    else {
                        err = 1;
                    }
                }
                if (err) {
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
                for (i = 0; i < channel_list.size; i++) {
                    if (strcmp(channel_list.list[i].txt_channel, channel.txt_channel) == 0) {
                        ch_exist = 1;
                        for (j = 0; j < channel_list.list[i].txt_users.size; j++) {
                            if (channel_list.list[i].txt_users.list[j].client_addr.sin_addr.s_addr == user_list.list[temp_user].client_addr.sin_addr.s_addr) {
                                err = 1;
                                //send error
                                strcpy(errtxt, "User already subscribed to channel\n");
                                error_handler(errtxt);
                                break;
                            }
                        }
                        if (!err) {
                            channel_list.list[i].user_size++;
                            if (channel_list.list[i].user_size > channel_list.list[i].txt_users.size) {
                                //err = 1;
                                strcpy(errtxt, "Max amount of users have subscribed\n");
                                error_handler(errtxt);
                                break;
                                /*channel_list.list[i].txt_users.size = channel_list.list[i].user_size;
                                channel_list.list[i].txt_users.list = realloc(channel_list.list[i].txt_users.list, sizeof(User)*channel_list.list[i].txt_users.size);
                                    channel_list.list[i].txt_users.list[channel_list.list[i].txt_users.size-1].isempty = 1;*/
                            }
                            for (j = 0; j < channel_list.list[i].txt_users.size; j++) {
                                if (channel_list.list[i].txt_users.list[j].isempty) {
                                    channel_list.list[i].txt_users.list[j] = user_list.list[temp_user];
                                    strcpy(channel_list.list[i].txt_users.list[j].current_channel, channel.txt_channel);
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
                        if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                            for (j = 0; user_list.list[i].subsize; j++) {
                                if (strcmp(user_list.list[i].sub_channels[j], channel.txt_channel) == 0) {
                                    err = 1;
                                    //send error
                                    strcpy(errtxt, "User already subscribed to channel\n");
                                    error_handler(errtxt);
                                    break;
                                }
                            }
                            if (!err) {
                                user_list.list[i].nsub++;
                                if (user_list.list[i].nsub > user_list.list[i].subsize) {
                                    //err = 1;
                                    strcpy(errtxt, "Max amount of channels user can subscribe to\n");
                                    error_handler(errtxt);
                                    break;
                                    /*user_list.list[i].subsize = user_list.list[i].nsub;
                                    user_list.list[i].sub_channels = realloc(user_list.list[i].sub_channels, sizeof(char *)*user_list.list[i].subsize);
                                    user_list.list[i].sub_channels[user_list.list[i].subsize-1] = (char *)malloc(sizeof(char)*CHANNEL_MAX);
                                    strcpy(user_list.list[i].sub_channels[user_list.list[i].subsize-1], channel.txt_channel);*/
                                }
                                else {
                                    for (j = 0; j < user_list.list[i].subsize; j++) {
                                        if (strcmp(user_list.list[i].sub_channels[j], "") == 0) {
                                            strcpy(user_list.list[i].sub_channels[j], channel.txt_channel);
                                        }
                                    }
                                }
                            }
                            /*channel_list.size++;
                            channel_list.list = realloc(channel_list.list, sizeof(Channel)*channel_list.size);*/
                            channel_list.nchannels++;
                            channel_list.list[channel_list.nchannels-1] = channel;
                            channel_list.list[channel_list.nchannels-1].txt_users.list[0] = user_list.list[temp_user];
                            strcpy(channel_list.list[channel_list.nchannels-1].txt_users.list[0].current_channel, channel.txt_channel);
                            break;
                        }
                    }
                }
                err = 0;
                //destroy_user(temp_user);
            }
            else if (req->req_type == REQ_LEAVE) {
                //printf("Leave\n");
                struct request_leave *req_leave = (struct request_leave *)&req;
                ch_exist = 0;
                strcpy(channel.txt_channel, req_leave->req_channel);
                for (i = 0; i < user_list.size; i++) {
                    if (user_list.list[i].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                        err = 0;
                        for (j = 0; j < channel_list.size; j++) {
                            if (strcmp(channel_list.list[j].txt_channel, channel.txt_channel) == 0) {
                                ch_exist = 1;
                                for (k = 0; k < channel_list.list[j].txt_users.size; k++) {
                                    if (channel_list.list[j].txt_users.list[k].client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                                        channel_list.list[j].txt_users.list[k].isempty = 1;
                                        channel_list.list[j].user_size--;
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
                            strcpy(errtxt, "Channel does not exist\n");
                            error_handler(errtxt);
                            break;
                        }
                        for (j = 0; j < user_list.list[i].subsize; j++) {
                            if (strcmp(user_list.list[i].sub_channels[j], channel.txt_channel) == 0) {
                                strcpy(user_list.list[i].sub_channels[j], "");
                                user_list.list[i].nsub--;
                                ch_exist = 1;
                                break;
                            }
                            else {
                                ch_exist = 0;
                            }
                        }
                        if (!ch_exist) {
                            //send error
                            strcpy(errtxt, "Channel does not exist\n");
                            error_handler(errtxt);
                            break;
                        }
                        if (strcmp(user_list.list[i].current_channel, channel.txt_channel) == 0) {
                            strcpy(user_list.list[i].current_channel, "");
                        }
                        break;
                    }
                    else {
                        err = 1;
                    }
                }
                if (err) {
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
            }
            else if (req->req_type == REQ_SAY) {
                //printf("Say\n");
                txt.txt_type = TXT_SAY;
                //retcode =
                text_handler(txt);
                if (err) {
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
            }
            else if (req->req_type == REQ_LIST) {
                //printf("List\n");
                txt.txt_type = TXT_LIST;
                //retcode =
                text_handler(txt);
                if (err) {
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
            }
            else if (req->req_type == REQ_WHO) {
                //printf("Who\n");
                txt.txt_type = TXT_WHO;
                //retcode =
                text_handler(txt);
                if (err) {
                    strcpy(errtxt, "User has not logged in");
                    error_handler(errtxt);
                    break;
                }
            }
        }
        if (nusers == 0) {
            break;
        }
    }

    //free(list);
    //free(who);
    
    destroy_channel_list(channel_list);
    //destroy_channel(channel);
    //destroy_user_list(user_list);
    //destroy_user(user);
    shutdown(sockid, 2);
    return 0;
}
