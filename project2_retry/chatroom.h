#ifndef chatroom_h
#define chatroom_h

#define packed __attribute__((packed))

#define USERNAME_MAX 32
#define CHANNEL_MAX 32
#define MESSAGE_MAX 64

#define LOGIN 0
#define LOGOUT 1
#define JOIN 2
#define LEAVE 3
#define SAY 4
#define LIST 5
#define WHO 6
#define SWITCH 7

#define S_SAY 0
#define S_LIST 1
#define S_WHO 2
#define S_ERROR 3

typedef int c_type;
typedef int s_type;

typedef struct client_data {
    c_type type;
    char username[USERNAME_MAX+1];
    char channel[CHANNEL_MAX+1];
    char message[MESSAGE_MAX+1];
} CData packed;

typedef struct server_data {
    s_type type;
    int size;
    char message[MESSAGE_MAX+1];
    char *user_list[USERNAME_MAX+1];
    char *channel_list[CHANNEL_MAX+1];
} SData packed;

#endif /* chatroom_h */
