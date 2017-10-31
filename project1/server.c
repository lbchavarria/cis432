#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "duckchat.h"



int main(int argc, char *argv[]) {
    int sockid, nread, addrlen, i;
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

    struct request req;
    struct channel_info current_channel;
    struct user_info user_i;
    struct text txt;
    struct text_list *list = (struct text_list *)malloc(sizeof(struct text_list));
    list->txt_nchannels = 1;
    struct text_who *who = (struct text_who *)malloc(sizeof(struct text_who));
    who->txt_nusernames = 0;
    struct channels *channel = (struct channels *)malloc(sizeof(struct channels));
    channel->nchannels = 0;
    channel->nusers = 0;
    channel->user_maxsize = 10;
    channel->txt_users = (struct users *)malloc(sizeof(struct users)*user_maxsize);
    for (i = 0; i < user_maxsize; i++) {
        channel->txt_users[i]-> = NULL;
    }
    struct users *user_list = (struct users *)malloc(sizeof(struct users));
    struct users user;

    while(1) {
        nread = recvfrom(sockid, req, sizeof(request), 0, (struct sockaddr *) &client_addr, &addrlen);
        if (nread > 0) {
            if (req.req_type == REQ_LOGIN) {
                struct request_login *req_login = (request_login *)&req;
                strcpy(user.username, req_login.req_username);
                strcpy(user.current_channel, "Common");
                
                //strcpy(user_i.us_username, req_login.req_username);
                //strcpy(current_channel.ch_channel, "Common");
                //list->txt_channels[list->txt_nchannels-1] = current_channel;
                if (channel[0]->nchannels == 0) {
                    channel[0]->nchannels = 1;
                    strcpy(channel[0]->txt_channel, "Common");
                }
                channel[0]->nusers++;
                if (channel[0]->nusers > channel[0]->user_maxsize) {
                    channel[0]->user_maxsize = channel[0]->nusers;
                    channel[0]->txt_users = realloc(channel[0]->txt_users, sizeof(struct users)*channel[0]->user_maxsize);
                    channel[0]->txt_users[user_maxsize-1] = NULL;
                }
                for (i = 0; i < channel[0]->user_maxsize; i++) {
                    if (channel[0]->txt_users[i] == NULL) {
                        channel[0]->txt_users[i] = user;
                        break;
                    }
                }
                
                
            }
            else if (req.req_type == REQ_LOGOUT) {
                struct request_logout *req_logout = (request_logout *)&req;
            }
            else if (req.req_type == REQ_JOIN) {
                struct request_join *req_join = (request_join *)&req;
            }
            else if (req.req_type == REQ_LEAVE) {
                struct request_leave *req_leave = (request_leave *)&req;
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

    free(list);
    free(who);
    free(channel);
    close(sockid);
    return 0;
}
