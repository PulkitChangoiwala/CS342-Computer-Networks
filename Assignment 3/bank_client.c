#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define RESPONSE_SIZE 512
#define REQUEST_SIZE 512

char* get_msg_from_server(int sock_fd);
void send_msg_to_server(int sock_fd, char *send_str);
void close_client_socket(int sock_fd);
void close_socket_error(int sock_fd);



char* get_msg_from_server(int sock_fd) {
    int count_recvd_pkts = 0;
    int n = read(sock_fd, &count_recvd_pkts, sizeof(int)); 
    if(n<=0)
    	return "\0";
    char *str = (char*)malloc(count_recvd_pkts*RESPONSE_SIZE);
    memset(str, 0, count_recvd_pkts*RESPONSE_SIZE);
    char *str_x = str;
    for(int i = 0; i < count_recvd_pkts; i++) {
        n = read(sock_fd, str, RESPONSE_SIZE);
        str = str+RESPONSE_SIZE;
    }
    return str_x;
}


void send_msg_to_server(int sock_fd,char* send_str){
	if(send_str == "\0") 
		return;
	int count_pkts_to_send = 0;
	count_pkts_to_send = (int)(strlen(send_str) - 1)/REQUEST_SIZE + 1;
	int n;
	n = write(sock_fd, &count_pkts_to_send, sizeof(int));
	char* msg_to_send = (char*)malloc(count_pkts_to_send * REQUEST_SIZE);
	strcpy(msg_to_send, send_str);
	for(int i=0;i<count_pkts_to_send;i++){
		n= write(sock_fd,msg_to_send,REQUEST_SIZE);
		msg_to_send = msg_to_send + REQUEST_SIZE;		
	}
}

void close_client_socket(int sock_fd){
	send_msg_to_server(sock_fd,"Terminate");
	shutdown(sock_fd, SHUT_WR); 
	char *msg_from_server;
	while(1) {
        msg_from_server = get_msg_from_server(sock_fd); 

        if(msg_from_server == "\0")
        {    
        	break;
        }
        printf("%s\n", msg_from_server);
        free(msg_from_server);
    }
    shutdown(sock_fd, SHUT_RD);
   	close(sock_fd); 
   	printf("Closed socket gracefully\n");
}

void close_socket_error(int sock_fd){
		shutdown(sock_fd, SHUT_RD);
		shutdown(sock_fd, SHUT_WR);
		close(sock_fd);
		printf("Closed socket gracefully\n");
}

int main(int argc, char** argv)
{
	int sock_fd,port_no;
	struct sockaddr_in serv_addr;
	if(argc < 3){                                  
		printf("Please input the server ip address and port no \n");
		return -1;
	}	  
	char *msg_from_server;
    char msg_to_server[RESPONSE_SIZE];
	

	sock_fd=socket(AF_INET, SOCK_STREAM, 0);
	port_no = atoi(argv[2]);

	memset(&serv_addr, 0, sizeof(serv_addr));  
	serv_addr.sin_family = AF_INET;         //setting DOMAIN
    serv_addr.sin_port = htons(port_no);     //setting server port number passed as an argument

    if(!(inet_aton(argv[1], &serv_addr.sin_addr)))   
    	printf("Server address is not valid\n"); // setting server host name passed as an argument
  
    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
     	printf("Server not reachable\n");
     	return -1;
    }
    else{
     	printf("Connection successfully established\nLogin to avail the facilities\n");
    }

    while(1){

     	msg_from_server = get_msg_from_server(sock_fd);
     	if(msg_from_server == NULL){
     		close_socket_error(sock_fd);
     		break;
     	}
     	printf("%s",msg_from_server);
		if(strncmp(msg_from_server, "INTERNAL_ERROR", 14 ) == 0){
			close_socket_error(sock_fd);
			break;
		}
		scanf("%s",msg_to_server);	
		if(strncmp(msg_to_server,"Terminate",9) == 0){
			close_client_socket(sock_fd);
			break;
		}
		send_msg_to_server(sock_fd,msg_to_server);

    }
    return 0;
}