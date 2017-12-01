#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <iostream>
#include <time.h>
#include <signal.h>
#include <sys/time.h>


using namespace std;



//#include "hash.h"
#include "duckchat.h"


#define MAX_CONNECTIONS 10
#define HOSTNAME_MAX 100
#define MAX_MESSAGE_LEN 65536
#define S_JOIN 10
#define S_LEAVE 11
#define S_SAY 12

//typedef map<string,string> channel_type; //<username, ip+port in string>
typedef map<string,struct sockaddr_in> channel_type; //<username, sockaddr_in of user>

int s; //socket for listening
struct sockaddr_in server;

int time_count = 0;

map<string,struct sockaddr_in> usernames; //<username, sockaddr_in of user>
map<string,int> active_usernames; //0-inactive , 1-active
//map<struct sockaddr_in,string> rev_usernames;
map<string,string> rev_usernames; //<ip+port in string, username>
map<string,channel_type> channels;
map<string, channel_type> subscribed_channels;

typedef struct server_list {
    int size;
    int pos;
    int isEmpty;
    sockaddr_in *buffer;
} SList;

SList *serverList;

void handle_socket_input();
void handle_login_message(void *data, struct sockaddr_in sock);
void handle_logout_message(struct sockaddr_in sock);
void handle_join_message(void *data, struct sockaddr_in sock);
void handle_server_join_message(void *data, strcut sockaddr_in sock);
void handle_leave_message(void *data, struct sockaddr_in sock);
void handle_server_leave_message(void *data, struct sockaddr_in sock);
void handle_say_message(void *data, struct sockaddr_in sock);
void handle_server_say_message(void *data, struct sockaddr_in sock);
void handle_list_message(struct sockaddr_in sock);
void handle_who_message(void *data, struct sockaddr_in sock);
void handle_keep_alive_message(struct sockaddr_in sock);
void send_error_message(struct sockaddr_in sock, string error_msg);
SList *initList(int size);
void destroyList(SList *list);
int insertList(SList *list, sockaddr_in server_addr);
SList *create_server_list(int arg, char *args[]);
int send_join_message(char* channel);
void timer_handler(int signum);



int main(int argc, char *argv[])
{
	
	if (argc != 3)
	{
		printf("Usage: ./server domain_name port_num\n");
		exit(1);
	}

	char hostname[HOSTNAME_MAX];
	int port;
	
	strcpy(hostname, argv[1]);
	port = atoi(argv[2]);
	
	
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		perror ("socket() failed\n");
		exit(1);
	}

	//struct sockaddr_in server;

	struct hostent     *he;

	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if ((he = gethostbyname(hostname)) == NULL) {
		puts("error resolving hostname..");
		exit(1);
	}
	memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);

	int err;

	err = bind(s, (struct sockaddr*)&server, sizeof server);

	if (err < 0)
	{
		perror("bind failed\n");
	}
	else
	{
		//printf("bound socket\n");
	}

	//testing maps end

	//create default channel Common
	string default_channel = "Common";
	map<string,struct sockaddr_in> default_channel_users;
	channels[default_channel] = default_channel_users;
	
    struct sigaction sa;
    struct itimerval timer;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    
    timer.it_value.tv_sec = 60;
    timer.it_value.tv_usec = 0;
    
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_VIRTUAL, &timer, NULL);

	while(1) //server runs for ever
	{

		//use a file descriptor with a timer to handle timeouts
		int rc;
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(s, &fds);
		


		rc = select(s+1, &fds, NULL, NULL, NULL);
		

		
		if (rc < 0)
		{
			printf("error in select\n");
            getchar();
		}
		else
		{
			int socket_data = 0;

			if (FD_ISSET(s,&fds))
			{
               
				//reading from socket
				handle_socket_input();
				socket_data = 1;
            }
		}
	}
	return 0;
}

void handle_socket_input()
{

	struct sockaddr_in recv_client;
	ssize_t bytes;
	void *data;
	size_t len;
	socklen_t fromlen;
	fromlen = sizeof(recv_client);
	char recv_text[MAX_MESSAGE_LEN];
	data = &recv_text;
	len = sizeof recv_text;


	bytes = recvfrom(s, data, len, 0, (struct sockaddr*)&recv_client, &fromlen);


	if (bytes < 0)
	{
		perror ("recvfrom failed\n");
	}
	else
	{
		//printf("received message\n");

		struct request* request_msg;
		request_msg = (struct request*)data;

		//printf("Message type:");
		request_t message_type = request_msg->req_type;

		//printf("%d\n", message_type);

		if (message_type == REQ_LOGIN)
		{
			handle_login_message(data, recv_client); //some methods would need recv_client
		}
		else if (message_type == REQ_LOGOUT)
		{
			handle_logout_message(recv_client);
		}
		else if (message_type == REQ_JOIN)
		{
			handle_join_message(data, recv_client);
		}
		else if (message_type == REQ_LEAVE)
		{
			handle_leave_message(data, recv_client);
		}
		else if (message_type == REQ_SAY)
		{
			handle_say_message(data, recv_client);
		}
		else if (message_type == REQ_LIST)
		{
			handle_list_message(recv_client);
		}
		else if (message_type == REQ_WHO)
		{
			handle_who_message(data, recv_client);
		}
        else if (message_type == S_JOIN) {
            handle_server_join_message(data, recv_client);
        }
        else if (message_type == S_LEAVE) {
            handle_server_leave_message(data, recv_client);
        }
        else if (message_type == S_SAY) {
            handle_server_say_message(data, recv_client);
        }
		{
			//send error message to client
			send_error_message(recv_client, "*Unknown command");
		}
	}
}

void handle_login_message(void *data, struct sockaddr_in sock)
{
	struct request_login* msg;
	msg = (struct request_login*)data;

	string username = msg->req_username;
	usernames[username]	= sock;
	active_usernames[username] = 1;

	//rev_usernames[sock] = username;

	//char *inet_ntoa(struct in_addr in);
	string ip = inet_ntoa(sock.sin_addr);
	//cout << "ip: " << ip <<endl;
	int port = sock.sin_port;
	//unsigned short short_port = sock.sin_port;
	//cout << "short port: " << short_port << endl;
	//cout << "port: " << port << endl;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	//cout << "port: " << port_str << endl;

	string key = ip + "." +port_str;
	//cout << "key: " << key <<endl;
	rev_usernames[key] = username;

	cout << "server: " << username << " logs in" << endl;
}

void handle_logout_message(struct sockaddr_in sock)
{

	//construct the key using sockaddr_in
	string ip = inet_ntoa(sock.sin_addr);
	//cout << "ip: " << ip <<endl;
	int port = sock.sin_port;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	//cout << "port: " << port_str << endl;

	string key = ip + "." +port_str;
	//cout << "key: " << key <<endl;

	//check whether key is in rev_usernames
	map <string,string> :: iterator iter;

	/*
    for(iter = rev_usernames.begin(); iter != rev_usernames.end(); iter++)
    {
        cout << "key: " << iter->first << " username: " << iter->second << endl;
    }
	*/

    
	iter = rev_usernames.find(key);
	if (iter == rev_usernames.end() )
	{
		//send an error message saying not logged in
		send_error_message(sock, "Not logged in");
	}
	else
	{
		//cout << "key " << key << " found."<<endl;
		string username = rev_usernames[key];
		rev_usernames.erase(iter);

		//remove from usernames
		map<string,struct sockaddr_in>::iterator user_iter;
		user_iter = usernames.find(username);
		usernames.erase(user_iter);

		//remove from all the channels if found
		map<string,channel_type>::iterator channel_iter;
		for(channel_iter = channels.begin(); channel_iter != channels.end(); channel_iter++)
		{
			//cout << "key: " << iter->first << " username: " << iter->second << endl;
			//channel_type current_channel = channel_iter->second;
			map<string,struct sockaddr_in>::iterator within_channel_iterator;
			within_channel_iterator = channel_iter->second.find(username);
			if (within_channel_iterator != channel_iter->second.end())
			{
				channel_iter->second.erase(within_channel_iterator);
			}

		}


		//remove entry from active usernames also
		//active_usernames[username] = 1;
		map<string,int>::iterator active_user_iter;
		active_user_iter = active_usernames.find(username);
		active_usernames.erase(active_user_iter);


		cout << "server: " << username << " logs out" << endl;
	}


	/*
    for(iter = rev_usernames.begin(); iter != rev_usernames.end(); iter++)
    {
        cout << "key: " << iter->first << " username: " << iter->second << endl;
    }
	*/


	//if so delete it and delete username from usernames
	//if not send an error message - later

}

void handle_join_message(void *data, struct sockaddr_in sock)
{
	//get message fields
	struct request_join* msg;
	msg = (struct request_join*)data;

	string channel = msg->req_channel;

	string ip = inet_ntoa(sock.sin_addr);

	int port = sock.sin_port;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	string key = ip + "." +port_str;


	//check whether key is in rev_usernames
	map <string,string> :: iterator iter;


	iter = rev_usernames.find(key);
	if (iter == rev_usernames.end() )
	{
		//ip+port not recognized - send an error message
		send_error_message(sock, "Not logged in");
	}
	else
	{
		string username = rev_usernames[key];

		map<string,channel_type>::iterator channel_iter;

		channel_iter = channels.find(channel);

		active_usernames[username] = 1;

		if (channel_iter == channels.end())
		{
			//channel not found
			map<string,struct sockaddr_in> new_channel_users;
			new_channel_users[username] = sock;
			channels[channel] = new_channel_users;
			//cout << "creating new channel and joining" << endl;

		}
		else
		{
			//channel already exits
			//map<string,struct sockaddr_in>* existing_channel_users;
			//existing_channel_users = &channels[channel];
			//*existing_channel_users[username] = sock;

			channels[channel][username] = sock;
			//cout << "joining exisitng channel" << endl;
		}
		cout << "server: " << username << " joins channel " << channel << endl;
	}
	//check whether the user is in usernames
	//if yes check whether channel is in channels
	//if channel is there add user to the channel
	//if channel is not there add channel and add user to the channel
}

void handle_sever_join_message(void *data, struct sockaddr_in sock) {
    time_count = 0;
    
    //get message fields
    struct request_join* msg;
    msg = (struct request_join*)data;
    
    string channel = msg->req_channel;
    
    string ip = inet_ntoa(sock.sin_addr);
    
    int port = sock.sin_port;
    
    char port_str[6];
    sprintf(port_str, "%d", port);
    string key = ip + "." +port_str;
    
    
    //check whether key is in rev_usernames
    /*map <string,string> :: iterator iter;
    
    
    iter = rev_usernames.find(key);
    if (iter == rev_usernames.end() )
    {
        //ip+port not recognized - send an error message
        send_error_message(sock, "Not logged in");
    }*/
    //else {
        //string username = rev_usernames[key];
    string username = key;
        
        map<string,channel_type>::iterator channel_iter;
        
        channel_iter = subscribed_channels.find(channel);
        
        //active_usernames[username] = 1;
        
        if (channel_iter == subscribed_channels.end()) {
            //channel not found
            map<string,struct sockaddr_in> new_channel_users;
            new_channel_users[username] = sock;
            subscribed_channels[channel] = new_channel_users;
            //cout << "creating new channel and joining" << endl;
            
        }
        else {
            //channel already exits
            //map<string,struct sockaddr_in>* existing_channel_users;
            //existing_channel_users = &channels[channel];
            //*existing_channel_users[username] = sock;
            
            subscribed_channels[channel][username] = sock;
            //cout << "joining exisitng channel" << endl;
        }
        cout << "server: " << username << " joins channel " << channel << endl;
    //}
    //check whether the user is in usernames
    //if yes check whether channel is in channels
    //if channel is there add user to the channel
    //if channel is not there add channel and add user to the channel
}

void handle_leave_message(void *data, struct sockaddr_in sock)
{

	//check whether the user is in usernames
	//if yes check whether channel is in channels
	//check whether the user is in the channel
	//if yes, remove user from channel
	//if not send an error message to the user


	//get message fields
	struct request_leave* msg;
	msg = (struct request_leave*)data;

	string channel = msg->req_channel;

	string ip = inet_ntoa(sock.sin_addr);

	int port = sock.sin_port;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	string key = ip + "." +port_str;


	//check whether key is in rev_usernames
	map <string,string> :: iterator iter;


	iter = rev_usernames.find(key);
	if (iter == rev_usernames.end() )
	{
		//ip+port not recognized - send an error message
		send_error_message(sock, "Not logged in");
	}
	else
	{
		string username = rev_usernames[key];

		map<string,channel_type>::iterator channel_iter;

		channel_iter = channels.find(channel);

		active_usernames[username] = 1;

		if (channel_iter == channels.end())
		{
			//channel not found
			send_error_message(sock, "No channel by the name " + channel);
			cout << "server: " << username << " trying to leave non-existent channel " << channel << endl;

		}
		else
		{
			//channel already exits
			//map<string,struct sockaddr_in> existing_channel_users;
			//existing_channel_users = channels[channel];
			map<string,struct sockaddr_in>::iterator channel_user_iter;
			channel_user_iter = channels[channel].find(username);

			if (channel_user_iter == channels[channel].end())
			{
				//user not in channel
				send_error_message(sock, "You are not in channel " + channel);
				cout << "server: " << username << " trying to leave channel " << channel  << " where he/she is not a member" << endl;
			}
			else
			{
				channels[channel].erase(channel_user_iter);
				//existing_channel_users.erase(channel_user_iter);
				cout << "server: " << username << " leaves channel " << channel <<endl;

				//delete channel if no more users
				if (channels[channel].empty() && (channel != "Common"))
				{
					channels.erase(channel_iter);
					cout << "server: " << "removing empty channel " << channel <<endl;
				}
			}
		}
	}
}

void handle_server_leave_message(void *data, struct sockaddr_in sock)
{
    
    //check whether the user is in usernames
    //if yes check whether channel is in channels
    //check whether the user is in the channel
    //if yes, remove user from channel
    //if not send an error message to the user
    
    
    //get message fields
    struct request_leave* msg;
    msg = (struct request_leave*)data;
    
    string channel = msg->req_channel;
    
    string ip = inet_ntoa(sock.sin_addr);
    
    int port = sock.sin_port;
    
    char port_str[6];
    sprintf(port_str, "%d", port);
    string key = ip + "." +port_str;
    
    
    //check whether key is in rev_usernames
    map <string,string> :: iterator iter;
    
    
    /*iter = rev_usernames.find(key);
    if (iter == rev_usernames.end() )
    {
        //ip+port not recognized - send an error message
        send_error_message(sock, "Not logged in");
    }
    else
    {*/
        //string username = rev_usernames[key];
    string username = key;
        
        map<string,channel_type>::iterator channel_iter;
        
        channel_iter = subscribed_channels.find(channel);
        
        active_usernames[username] = 1;
        
        if (channel_iter == subscribed_channels.end())
        {
            //channel not found
            send_error_message(sock, "No channel by the name " + channel);
            cout << "server: " << username << " trying to leave non-existent channel " << channel << endl;
            
        }
        else
        {
            //channel already exits
            //map<string,struct sockaddr_in> existing_channel_users;
            //existing_channel_users = channels[channel];
            map<string,struct sockaddr_in>::iterator channel_user_iter;
            channel_user_iter = subscribed_channels[channel].find(username);
            
            if (channel_user_iter == subscribed_channels[channel].end())
            {
                //user not in channel
                send_error_message(sock, "You are not in channel " + channel);
                cout << "server: " << username << " trying to leave channel " << channel  << " where he/she is not a member" << endl;
            }
            else
            {
                subscribed_channels[channel].erase(channel_user_iter);
                //existing_channel_users.erase(channel_user_iter);
                cout << "server: " << username << " leaves channel " << channel <<endl;
                
                //delete channel if no more users
                if (subscribed_channels[channel].empty() && (channel != "Common"))
                {
                    subscribed_channels.erase(channel_iter);
                    cout << "server: " << "removing empty channel " << channel <<endl;
                }
            }
        }
    }
}


void handle_say_message(void *data, struct sockaddr_in sock)
{
    
    //check whether the user is in usernames
	//if yes check whether channel is in channels
	//check whether the user is in the channel
	//if yes send the message to all the members of the channel
	//if not send an error message to the user

    int i;
    int not_server = 1;
    
    for (i = 0; i < serverList->pos; i++) {
        if (sock.sin_addr == serverList->buffer[i].sin_addr && sock.sin_port == serverList->buffer[i].sin_port) {
            not_server = 0;
        }
        else {
            if (send_say_message(data, serverList->buffer[i])) {
                printf("Failed to send say message to other servers\n");
            }
        }
    }
    if (not_server) {
        //get message fields
        struct request_say* msg;
        msg = (struct request_say*)data;

        string channel = msg->req_channel;
        string text = msg->req_text;


        string ip = inet_ntoa(sock.sin_addr);

        int port = sock.sin_port;

        char port_str[6];
        sprintf(port_str, "%d", port);
        string key = ip + "." +port_str;
        

        //check whether key is in rev_usernames
        map <string,string> :: iterator iter;


        iter = rev_usernames.find(key);
        if (iter == rev_usernames.end() ) {
            //ip+port not recognized - send an error message
            send_error_message(sock, "Not logged in ");
        }
        else {
            string username = rev_usernames[key];

            map<string,channel_type>::iterator channel_iter;

            channel_iter = channels.find(channel);

            active_usernames[username] = 1;

            if (channel_iter == channels.end()) {
                //channel not found
                send_error_message(sock, "No channel by the name " + channel);
                cout << "server: " << username << " trying to send a message to non-existent channel " << channel << endl;

            }
            else {
                //channel already exits
                //map<string,struct sockaddr_in> existing_channel_users;
                //existing_channel_users = channels[channel];
                map<string,struct sockaddr_in>::iterator channel_user_iter;
                channel_user_iter = channels[channel].find(username);

                if (channel_user_iter == channels[channel].end()) {
                    //user not in channel
                    send_error_message(sock, "You are not in channel " + channel);
                    cout << "server: " << username << " trying to send a message to channel " << channel  << " where he/she is not a member" << endl;
                }
                else {
                    map<string,struct sockaddr_in> existing_channel_users;
                    existing_channel_users = channels[channel];
                    for(channel_user_iter = existing_channel_users.begin(); channel_user_iter != existing_channel_users.end(); channel_user_iter++) {
                        //cout << "key: " << iter->first << " username: " << iter->second << endl;

                        ssize_t bytes;
                        void *send_data;
                        size_t len;

                        struct text_say send_msg;
                        send_msg.txt_type = TXT_SAY;

                        const char* str = channel.c_str();
                        strcpy(send_msg.txt_channel, str);
                        str = username.c_str();
                        strcpy(send_msg.txt_username, str);
                        str = text.c_str();
                        strcpy(send_msg.txt_text, str);
                        //send_msg.txt_username, *username.c_str();
                        //send_msg.txt_text,*text.c_str();
                        send_data = &send_msg;

                        len = sizeof send_msg;

                        //cout << username <<endl;
                        struct sockaddr_in send_sock = channel_user_iter->second;
                        
                        //bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, fromlen);
                        bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);

                        if (bytes < 0) {
                            perror("Message failed\n"); //error
                        }
                        else {
                            //printf("Message sent\n");
                        }
                    }
                    cout << "server: " << username << " sends say message in " << channel <<endl;
                }
            }
        }
    }
}

void handle_server_say_message(void *data, struct sockaddr_in sock)
{
    
    //check whether the user is in usernames
    //if yes check whether channel is in channels
    //check whether the user is in the channel
    //if yes send the message to all the members of the channel
    //if not send an error message to the user
    
    int i;
    int not_server = 1;
    
    for (i = 0; i < serverList->pos; i++) {
        if (sock.sin_addr == serverList->buffer[i].sin_addr && sock.sin_port == serverList->buffer[i].sin_port) {
            not_server = 0;
        }
        else {
            if (send_say_message(data, serverList->buffer[i])) {
                printf("Failed to send say message to other servers\n");
            }
        }
    }
    if (not_server) {
        //get message fields
        struct request_say* msg;
        msg = (struct request_say*)data;
        
        string channel = msg->req_channel;
        string text = msg->req_text;
        
        
        string ip = inet_ntoa(sock.sin_addr);
        
        int port = sock.sin_port;
        
        char port_str[6];
        sprintf(port_str, "%d", port);
        string key = ip + "." +port_str;
        
        
        //check whether key is in rev_usernames
        map <string,string> :: iterator iter;
        
        
        iter = rev_usernames.find(key);
        if (iter == rev_usernames.end() ) {
            //ip+port not recognized - send an error message
            send_error_message(sock, "Not logged in ");
        }
        else {
            string username = rev_usernames[key];
            
            map<string,channel_type>::iterator channel_iter;
            
            channel_iter = channels.find(channel);
            
            active_usernames[username] = 1;
            
            if (channel_iter == channels.end()) {
                //channel not found
                send_error_message(sock, "No channel by the name " + channel);
                cout << "server: " << username << " trying to send a message to non-existent channel " << channel << endl;
                
            }
            else {
                //channel already exits
                //map<string,struct sockaddr_in> existing_channel_users;
                //existing_channel_users = channels[channel];
                map<string,struct sockaddr_in>::iterator channel_user_iter;
                channel_user_iter = channels[channel].find(username);
                
                if (channel_user_iter == channels[channel].end()) {
                    //user not in channel
                    send_error_message(sock, "You are not in channel " + channel);
                    cout << "server: " << username << " trying to send a message to channel " << channel  << " where he/she is not a member" << endl;
                }
                else {
                    map<string,struct sockaddr_in> existing_channel_users;
                    existing_channel_users = channels[channel];
                    for(channel_user_iter = existing_channel_users.begin(); channel_user_iter != existing_channel_users.end(); channel_user_iter++) {
                        //cout << "key: " << iter->first << " username: " << iter->second << endl;
                        
                        ssize_t bytes;
                        void *send_data;
                        size_t len;
                        
                        struct text_say send_msg;
                        send_msg.txt_type = TXT_SAY;
                        
                        const char* str = channel.c_str();
                        strcpy(send_msg.txt_channel, str);
                        str = username.c_str();
                        strcpy(send_msg.txt_username, str);
                        str = text.c_str();
                        strcpy(send_msg.txt_text, str);
                        //send_msg.txt_username, *username.c_str();
                        //send_msg.txt_text,*text.c_str();
                        send_data = &send_msg;
                        
                        len = sizeof send_msg;
                        
                        //cout << username <<endl;
                        struct sockaddr_in send_sock = channel_user_iter->second;
                        
                        //bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, fromlen);
                        bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);
                        
                        if (bytes < 0) {
                            perror("Message failed\n"); //error
                        }
                        else {
                            //printf("Message sent\n");
                        }
                    }
                    cout << "server: " << username << " sends say message in " << channel <<endl;
                }
            }
        }
    }
}


void handle_list_message(struct sockaddr_in sock)
{
    
    //check whether the user is in usernames
	//if yes, send a list of channels
	//if not send an error message to the user



	string ip = inet_ntoa(sock.sin_addr);

	int port = sock.sin_port;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	string key = ip + "." +port_str;


	//check whether key is in rev_usernames
	map <string,string> :: iterator iter;


	iter = rev_usernames.find(key);
	if (iter == rev_usernames.end() )
	{
		//ip+port not recognized - send an error message
		send_error_message(sock, "Not logged in ");
	}
	else
	{
		string username = rev_usernames[key];
		int size = channels.size();
		//cout << "size: " << size << endl;

		active_usernames[username] = 1;

		ssize_t bytes;
		void *send_data;
		size_t len;


		//struct text_list temp;
		struct text_list *send_msg = (struct text_list*)malloc(sizeof (struct text_list) + (size * sizeof(struct channel_info)));


		send_msg->txt_type = TXT_LIST;

		send_msg->txt_nchannels = size;


		map<string,channel_type>::iterator channel_iter;



		//struct channel_info current_channels[size];
		//send_msg.txt_channels = new struct channel_info[size];
		int pos = 0;

		for(channel_iter = channels.begin(); channel_iter != channels.end(); channel_iter++)
		{
			string current_channel = channel_iter->first;
			const char* str = current_channel.c_str();
			//strcpy(current_channels[pos].ch_channel, str);
			//cout << "channel " << str <<endl;
			strcpy(((send_msg->txt_channels)+pos)->ch_channel, str);
			//strcpy(((send_msg->txt_channels)+pos)->ch_channel, "hello");
			//cout << ((send_msg->txt_channels)+pos)->ch_channel << endl;

			pos++;

		}

		//send_msg.txt_channels =
		//send_msg.txt_channels = current_channels;
		send_data = send_msg;
		len = sizeof (struct text_list) + (size * sizeof(struct channel_info));

					//cout << username <<endl;
		struct sockaddr_in send_sock = sock;


		//bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, fromlen);
		bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);

		if (bytes < 0)
		{
			perror("Message failed\n"); //error
		}
		else
		{
			//printf("Message sent\n");

		}

		cout << "server: " << username << " lists channels"<<endl;
	}
}


void handle_who_message(void *data, struct sockaddr_in sock)
{


	//check whether the user is in usernames
	//if yes check whether channel is in channels
	//if yes, send user list in the channel
	//if not send an error message to the user


	//get message fields
	struct request_who* msg;
	msg = (struct request_who*)data;

	string channel = msg->req_channel;

	string ip = inet_ntoa(sock.sin_addr);

	int port = sock.sin_port;

 	char port_str[6];
 	sprintf(port_str, "%d", port);
	string key = ip + "." +port_str;


	//check whether key is in rev_usernames
	map <string,string> :: iterator iter;


	iter = rev_usernames.find(key);
	if (iter == rev_usernames.end() )
	{
		//ip+port not recognized - send an error message
		send_error_message(sock, "Not logged in ");
	}
	else
	{
		string username = rev_usernames[key];

		active_usernames[username] = 1;

		map<string,channel_type>::iterator channel_iter;

		channel_iter = channels.find(channel);

		if (channel_iter == channels.end())
		{
			//channel not found
			send_error_message(sock, "No channel by the name " + channel);
			cout << "server: " << username << " trying to list users in non-existing channel " << channel << endl;

		}
		else
		{
			//channel exits
			map<string,struct sockaddr_in> existing_channel_users;
			existing_channel_users = channels[channel];
			int size = existing_channel_users.size();

			ssize_t bytes;
			void *send_data;
			size_t len;


			//struct text_list temp;
			struct text_who *send_msg = (struct text_who*)malloc(sizeof (struct text_who) + (size * sizeof(struct user_info)));


			send_msg->txt_type = TXT_WHO;

			send_msg->txt_nusernames = size;

			const char* str = channel.c_str();

			strcpy(send_msg->txt_channel, str);



			map<string,struct sockaddr_in>::iterator channel_user_iter;

			int pos = 0;

			for(channel_user_iter = existing_channel_users.begin(); channel_user_iter != existing_channel_users.end(); channel_user_iter++)
			{
				string username = channel_user_iter->first;

				str = username.c_str();

				strcpy(((send_msg->txt_users)+pos)->us_username, str);


				pos++;
			}

			send_data = send_msg;
			len = sizeof(struct text_who) + (size * sizeof(struct user_info));

						//cout << username <<endl;
			struct sockaddr_in send_sock = sock;


			//bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, fromlen);
			bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);

			if (bytes < 0)
			{
				perror("Message failed\n"); //error
			}
			else
			{
				//printf("Message sent\n");

			}

			cout << "server: " << username << " lists users in channnel "<< channel << endl;
			}
	}
}



void send_error_message(struct sockaddr_in sock, string error_msg)
{
	ssize_t bytes;
	void *send_data;
	size_t len;

	struct text_error send_msg;
	send_msg.txt_type = TXT_ERROR;

	const char* str = error_msg.c_str();
	strcpy(send_msg.txt_error, str);

	send_data = &send_msg;

	len = sizeof send_msg;


	struct sockaddr_in send_sock = sock;



	bytes = sendto(s, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);

	if (bytes < 0)
	{
		perror("Message failed\n"); //error
	}
	else
	{
		//printf("Message sent\n");
	}

}

SList *initList(int size) {
    SList *newList = (SList *)malloc(sizeof(SList));
    newList->buffer = (sockaddr_in *)malloc(size*sizeof(sockaddr_in));
    newList->size = size;
    newList->pos = 0;
    newList->isEmpty = 1;
    return newList;
}

void destroyList(SList *list) {
    free(list->buffer);
    free(list);
}

int insertList(SList *list sockaddr_in server_addr) {
    if (list->pos == list->size) {
        printf("Cannot add any more servers to list\n");
        return 0;
    }
    list->buffer[list->pos] = item;
    list->pos++;
    return 1;
}

SList *create_server_list(int arg, char *args[]) {
    int size, i;
    char hostname[HOSTNAME_MAX];
    int port;
    struct sockaddr_in server_addr
    
    size = ((arg-2) - (arg%2))/2
    SList *newList = initList(size);
    
    for (i = 3; i < arg; i++) {
        strcpy(hostname, args[i]);
        i++;
        port = atoi(args[i]);
    
        struct hostent     *he;
    
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
    
        if ((he = gethostbyname(hostname)) == NULL) {
            puts("error resolving hostname..");
            exit(1);
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
        if (insertList(newList, server_addr) == 0) {
            exit(1)
        }
    }
    return newList;
}

int send_join_message(char* channel, struct sockaddr_in server_addr) {
    ssize_t bytes;
    void *data;
    size_t len;
    
    struct request_join join_msg;
    join_msg.req_type = S_JOIN;
    strcpy(join_msg.req_channel,channel);
    data = &join_msg;
    
    len = sizeof join_msg;
    
    bytes = sendto(s, data, len, 0, (struct sockaddr*)&server_addr, sizeof server_addr);
    printf("Send S2S Join\n");
    if (bytes < 0) {
        return 1; //error
    }
    else {
        strcpy(active_channel,channel);
        
        string joined_channel = string(channel);
        subscribed_channels[joined_channel] = joined_channel; //add to the list of subscribed channels
        
        return 0;
    }
}

int send_leave_message(char* channel, struct sockaddr_in server_addr) {
    ssize_t bytes;
    void *data;
    size_t len;
    
    struct request_leave msg;
    msg.req_type = S_LEAVE;
    
    strcpy(msg.req_channel,channel);
    data = &msg;
    
    len = sizeof msg;
    
    bytes = sendto(s, data, len, 0, (struct sockaddr*)&server_addr, sizeof server_addr);
    printf("Send S2S Leave\n");
    if (bytes < 0) {
        return 1; //error
    }
    else {
        if (strcmp(channel,active_channel) == 0) {
            active_channel[0] = '\0';;
        }
        
        string leaving_channel = string(channel);
        map<string,string>::iterator channel_iter;
        channel_iter = subscribed_channels.find(leaving_channel);
        if (channel_iter != subscribed_channels.end()) {
            subscribed_channels.erase(channel_iter);
        }
        return 0;
    }
}

int send_say_message(void *data, struct sockaddr_in server_addr)
{
    ssize_t bytes;
    //void *data;
    size_t len;
    
    //get message fields
    struct request_say* msg;
    msg = (struct request_say*)data;
    
    //struct request_say say_msg;
    msg->req_type = S_SAY;
    /*strcpy(say_msg.req_channel,active_channel);
    strcpy(say_msg.req_text,text);
    data = &say_msg;*/
    
    len = sizeof(struct request_say);
    
    bytes = sendto(s, msg, len, 0, (struct sockaddr*)&server_addr, sizeof server_addr);
    printf("send S2S Say\n")
    if (bytes < 0)
    {
        return 1; //error
    }
    else
    {
        return 0;
    }
}

void timer_handler(int signum) {
    int i, j;
    static int count = 0;
    static int two_minute = 2;
    count++;
    for (i = 0; i < serverList->pos; i++) {
        for (auto const &ent1 : subscribed_channels) {
            if (send_join_message(ent1.first, serverList->buffer[i])) {
                printf("Unable to send join message to another server\n");
            }
        }
    }
    if (count == two_minute && time_count) {
        for (i = 0; i < serverList->pos; i++) {
            for (auto const &ent1 : subscribed_channels) {
                if (send_leave_message(ent1.first, serverList->buffer[i])) {
                    printf("Unable to send join message to another server\n");
                }
            }
        }
        count2 == 0;
    }
    if (time_count = 0) {
        time_count = 1;
        count = 0;
    }
}




