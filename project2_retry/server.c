/* The List struct and funtions, initList(), destroyList(), and writeList() are not mine
 * The above struct and functions are based on code written by Peter Dickman and revised by Joe Sventek
 */

#include <stdio.h>
#include <std.lib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netonet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <chatroom.h>

#define UNUSED __attribute__ ((unused))

typedef struct user {
    char username[USERNAME_MAX];
    struct sockaddr_in user_addr;
} User;

typedef struct list {
    int size;
    int wpos;
    int rpos; //might not need
    int isEmpty; // might not neeed
    void **buffer;
} List;

typedef struct channel {
    char name[CHANNEL_MAX];
    List *list;
} Channel;

// Global variables
int sockid;
struct sockaddr_in client_addr;

List *initList(int size) {
    /* Initialize list of length size
     * List will wither hold User items or Channel items
     */
    List *newlist = (List *)malloc(sizeof(List));
    newlist->buffer = (void **)malloc(size*sizeof(void *));
    
    newlist->size = size;
    newlist->wpos = 0;
    newlist->rpos = size-1; // might nit need
    newlist->isEmpty = 1; // might not need
    
    return newlist;
}

void destroyList(List *list) {
    /* Free buffer inside list
     * Free list
     */
    free(list->buffer);
    free(list);
}

int writeList(List list, void *item) {
    /* Write item into buffer of the list
     * Item will either be User or Channel
     * Successful write will return 1;
     * If list buffer is full, item will not be written to list and return 0
     */
    if (list->wpos == list->size) {
        printf("Cannot add any more items to list\n");
        return 0;
    }
    list->buffer[list->wpos++] = item;
    return 1;
}

void server_data_handler() {
    
}

int main(UNUSED int argc, char *argv[]) { // Multiserver implementation will have UNUSED be removed
    struct sockaddr_in my_addr;
    
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
    
    while (1) {
        
    }
    
    return 0;
}
