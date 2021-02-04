#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define USER 0
#define POLICE 1
#define ADMIN 2
#define UNAUTH_USER -1
#define INTERNAL_ERROR -2
#define RESPONSE_SIZE 512
#define REQUEST_SIZE 512
#define linesInMS 8
#define EXIT -3
#define SUCCESS 3

//List of all the function prototypes used in the file
void send_msg_to_client(int clientFD, char* str);
char* get_msg_from_client(int clientFD);
void get_user_pwd(char* username, char* password, int clientFD);
char* print_statement(char* username, int clientFD);
char* print_balance(char* username);
void user_requests(char* username, char* password, int clientFD);
int check_user(char* user);
void update_transaction(char* username, int choice, double balance);
int query(char* username, int clientFD);
void admin_query(int clientFD);
char* get_all_balance();
void police_queries(int clientFD);
int authorization(char* username, char* password);
void close_client(int clientFD, char* str);
void talk_to_client(int clientFD);
int is_terminate_msg(char* msg_from_client);
void close_server_socket(int clientFD);
void close_error(int clientFD);

//for closing server socket gracefully
void close_server_socket(int clientFD){
		shutdown(clientFD, SHUT_RD); // client has already stopped sending messages so stop listening
		send_msg_to_client(clientFD,"Thanks...! Please Visit Again!\n");
		shutdown(clientFD, SHUT_WR); // give up writing
		close(clientFD);// free up resources 
		printf("%d  Socket closed gracefully\n",clientFD);
}

//for closing socket when any error occurs
void close_error(int clientFD){
		printf("An internal error occurred\n");
		send_msg_to_client(clientFD,"INTERNAL_ERROR occurred. Try again later\n");
		char* msg_from_client;
		shutdown(clientFD, SHUT_WR);  // no more packets will be written from server side
		   while(1) {
        		msg_from_client = get_msg_from_client(clientFD); // receive the packets cient has already sent
        		if(msg_from_client == NULL)
            		break;
        		free(msg_from_client);
    		}
    	shutdown(clientFD, SHUT_RD); // give up receiving more messages 
    	close(clientFD);  // frees up resources 
    	printf("%d Closed gracefully due to internal error\n",clientFD);
}

//for interacting with the client
void talk_to_client(int clientFD)
{

	int authorisation = UNAUTH_USER;
	int try =0;
	char* username;
    char* password;
    char* msg_from_client;
    while(1){
    	if(try){
       			msg_from_client = get_msg_from_client(clientFD);  // if its current clients 2nd try to login
       			if(is_terminate_msg(msg_from_client)){
       	 			close_server_socket(clientFD);return; // termination during login 
       	 		}
       	 	}
       	 	username = (char*)malloc(150*sizeof(char));
       	 	password = (char*)malloc(150*sizeof(char));
       	 	send_msg_to_client(clientFD,"Enter username:\n");
       	 	msg_from_client = get_msg_from_client(clientFD);
       	 	int i=0;
       	 	if(is_terminate_msg(msg_from_client)){
       	 		close_server_socket(clientFD);return;
       	 	}
       	 	else{
       	 		i=0;
       	 		while(msg_from_client[i]!='\0' && msg_from_client[i]!='\n'){
       	 			username[i]=msg_from_client[i];
       	 			i++;
       	 		}
       	 	}
       	 	username[i] = '\0';
       	 	if(msg_from_client != NULL)
       	 		free(msg_from_client);

       	 	send_msg_to_client(clientFD,"Enter password:\n");
       	 	msg_from_client = get_msg_from_client(clientFD);

       	 	if(is_terminate_msg(msg_from_client)){
       	 		close_server_socket(clientFD);
       	 		return;
       	 	}
       	 	else{
       	 		i =0;
       	 		while(msg_from_client[i]!='\0' && msg_from_client[i]!='\n'){
       	 			password[i]=msg_from_client[i];
       	 			i++;
       	 		}
       	 	}
       	 	password[i] = '\0';
       	 	if(msg_from_client != NULL) 
       	 		free(msg_from_client);

       	 	authorisation = authorization(username,password);
       	 	printf("New user logged in with authorisation %d\n",authorisation );
       	 	try = 1;
       	 	if(authorisation == UNAUTH_USER){
       	 		send_msg_to_client(clientFD,"Unathorised Login.Type Terminate to terminate or any key to try again.\n");
       	 		if(username != NULL) 
       	 			free(username);
       	 		if(password != NULL) 
       	 			free(password);
       	 	}
       	 	else if(authorisation == INTERNAL_ERROR){
       			close_error(clientFD);  // closing socket due to internal error 
       			if(username != NULL) 
       				free(username);
       			if(password != NULL) 
       				free(password);
       			return;
       		}
       		else{
       			break;
       		}
       	}
       	switch(authorisation){
			case USER:
			    user_requests(username,password,clientFD);
				break;
			case ADMIN:
			    admin_query(clientFD);
				break;
			case POLICE:
			    police_queries(clientFD);
				break;
			default:
				close_error(clientFD);
				break;
		}
}

// void close_client(int clientFD, char* str){
// 	send_msg_to_client(clientFD, str);
// 	shutdown(clientFD, SHUT_RDWR);
// }

//for checking user authentication
int authorization(char* username,char *password)
{
	printf("Authorizing access\n");
	char* line = NULL;
    size_t len = 0;
    ssize_t read;

	FILE *fp=fopen("login_file","r");
	if(fp == NULL){
		printf("There's a problem in opening login_file file\n"); // if user login file is corrupted 
		return INTERNAL_ERROR;
	}
	while((read = getline(&line, &len, fp)) != -1) 
	{
		char *token=strtok(line," ");
		if(token == NULL){
			fclose(fp); 
			printf(" problem in opening user_login file entries\n"); 
			return INTERNAL_ERROR;
		}
		if(!(strcmp(token,username)))
		{
			token=strtok(NULL," ");
			if(token == NULL){
				fclose(fp); 
				printf(" problem in opening user_login file entries\n"); 
				return INTERNAL_ERROR;
			}
			if(strcmp(token,password)==0)
			{
				token=strtok(NULL," ");
				if(token == NULL){
					fclose(fp); 
					printf(" problem in opening user_login file entries\n"); 
					return INTERNAL_ERROR;
				}
                if(token[0]=='C')
                {
					fclose(fp);
                    return USER;
                }
                else if(token[0]=='A')
                {
                    fclose(fp);
                    return ADMIN;
                }
                else if(token[0]=='P')
                {
                    fclose(fp);
                    return POLICE;
                }
            }
        }
    }
    if(line!=NULL)
    {
    	free(line);
    }
    fclose(fp);
	return UNAUTH_USER;
}

//for checking if user exists or not
int check_user(char* user)
{
	FILE *fp=fopen("login_file","r");
	char* line = NULL;
    size_t len = 0;
    ssize_t read;

	while((read = getline(&line, &len, fp)) != -1) 
	{
		char* token=strtok(line," ");
		if(token == NULL){
			fclose(fp); 
			printf("There's a problem in login_file file entries\n"); 
			return 0;
		}
		if(strcmp(token,user)==0)
		{
			token=strtok(NULL," ");
			token=strtok(NULL," ");
			if(token[0]=='C')
			{
				fclose(fp);
				return 1;
			}
        }
    }
    fclose(fp);
    return 0;
}

//for interaction with customer type user
void user_requests(char* username,char* password,int clientFD)
{
	int flag=1;
	send_msg_to_client(clientFD,"You are logged in as user. \nEnter your choice\n1. Generate Mini Statement\n2. Check Available Balance\nWrite 'Terminate' for quitting.");
	char *buff=NULL;
	while(flag)
	{
		if(buff!=NULL)
		{
			buff=NULL;
		}

		buff=get_msg_from_client(clientFD);

		int choice;

		if(strncmp(buff,"Terminate",9)==0)
			choice=3;
		else
		    choice=atoi(buff);
		char *bal,*str;
		bal=(char *)malloc(1000*sizeof(char));
		str=(char *)malloc(10000*sizeof(char));
		strcpy(str,"------------------\nMini Statement: \n");
		strcpy(bal,"------------------\nAvailable Balance: \n");
		switch(choice)
		{
			case 1:
			 	strcat(str,print_statement(username,clientFD));
				send_msg_to_client(clientFD,strcat(str,"\n------------------\n\nEnter your choice\n1. Generate Mini Statement\n2. Check Available Balance\nWrite Terminate for quitting."));
				if(str!=NULL)
					free(str);
				break;
			case 2:
				strcat(bal,print_balance(username));
				send_msg_to_client(clientFD,strcat(bal,"\n------------------\n\nEnter your choice\n1. Generate Mini Statement\n2. Check Available Balance\nWrite Terminate for quitting."));
				if(bal != NULL) 
					free(bal);
				break;
			case 3:
				flag=0;
				break;
			default:
				send_msg_to_client(clientFD, "Unknown Query\nEnter your choice\n1. Generate Mini Statement\n2. Check Available Balance\nWrite 'Terminate' for quitting.");
				break;
		}
	}
	close_server_socket(clientFD);
}

//for updating transaction after admin query
void update_transaction(char* username,int choice,double balance)
{
	FILE *fp=fopen(username,"a");
	
	char *line = NULL;
	char c;
	if(choice==1)
		c='C';
	else
		c='D';
	char *line1=(char *)malloc(sizeof(char)*10000);
    size_t len = 0;
    ssize_t read;
	time_t ltime=time(NULL);

	sprintf(line1,"%.*s %c %f\n",(int)strlen(asctime(localtime(&ltime)))-1,asctime(localtime(&ltime)),c,balance);
	
	/*
	while((read = getline(&line, &len, fp)) != -1)
	{
		strcat(line1,line);
	} 
	fclose(fp);
	fp=fopen(username,"w");
	*/
	fwrite(line1, sizeof(char), strlen(line1), fp);
	fclose(fp);
}

//admin actions for credit and debit
int query(char* username, int clientFD)
{
	send_msg_to_client(clientFD,"\n1) Type `Credit` to credit balance to user account.\n2) Type `Debit` to debit balance from an account.\n3) Type `Terminate` to quit\n");
	int query_flag = -1;
	char* buff = NULL;

	while(1){  
		buff = get_msg_from_client(clientFD);
		if(buff == NULL){
			return EXIT;                                         // termination request from admin
		}
		else if(strncmp(buff,"Terminate",9)== 0){
			return EXIT;
		}
		else if(strncmp(buff,"Credit",6)== 0) {   // credit requests
			query_flag = 1;
		}
		else if(strncmp(buff,"Debit",5)== 0){
			query_flag = 2;                                  // debit requsests
		}
		else{
			query_flag = -1;
		}

		if(buff!= NULL)free(buff);

		char* temp = print_balance(username);
		double balance;
		if(strncmp(temp,"Could",5) == 0 && query_flag!=1){
			send_msg_to_client(clientFD, "Your transaction file does not exist\n");
			return INTERNAL_ERROR;        // transaction history file is not found
		}
		else if(strncmp(temp,"Could",5) != 0){
			
			balance=strtod(temp,NULL);

		}

		switch(query_flag) {
			case 1:
				send_msg_to_client(clientFD, "Enter Amount to be credited\n");
				while(1){
					buff = get_msg_from_client(clientFD);
					double amount = strtod(buff, NULL);

					if(amount<0){
						send_msg_to_client(clientFD, "Enter valid non negative amount\n"); // checking the balance is not negative
					}
					else {
						balance += amount; 
						update_transaction(username, query_flag, balance);
						send_msg_to_client(clientFD, "Credit successful\n1) Type username of account holder to perform transactions.\n2) Type `Terminate` to quit\n");
	    				return SUCCESS;     // messages for next query 
					}
				}
			case 2:
				send_msg_to_client(clientFD, "Enter Amount to be debited\n");
				while(1){
					buff = get_msg_from_client(clientFD);
					double amount = strtod(buff, NULL);
										// messages for next query
					if(amount<0){
						send_msg_to_client(clientFD, "Enter valid amount\n"); // checking the balance is not negative
					}
					else if(amount > balance) {
						char* temp1 = (char*) malloc(150*sizeof(char));
						strcpy(temp1,"Insufficient Balance!\n Enter valid amount which is non negative and <= ");
						strcat(temp1,temp); //  checking there is enough money availble or not
						strcat(temp1," \n");
						send_msg_to_client(clientFD,temp1);
						if(temp1 != NULL) free(temp1);
					}							
					else {
						balance -= amount;
						update_transaction(username, query_flag, balance);
						send_msg_to_client(clientFD, "Debit successful\n1) Type username of account holder to perform transactions.\n2) Type `Terminate` to quit\n");
	    				return SUCCESS;
					}							// messages for next query
				}
			default:
				send_msg_to_client(clientFD, "Unknown Query\n1) Type `Credit` to credit balance to an account.\n2) Type `Debit` to debit balance from an account.\n3) Type `Terminate` to quit\n");
		}
	}
}

//admin access
void admin_query(int clientFD)
{
	send_msg_to_client(clientFD,"You are logged in as admin.\n1) Type username of account holder to perform transactions.\n2) Type `Terminate` to quit\n");

	while(1)
	{
		char *msg_from_client = NULL;
		msg_from_client = get_msg_from_client(clientFD);

		if(msg_from_client == NULL) {
			break;
		}
		else if(strncmp(msg_from_client,"Terminate",9)== 0) {
			break;
		}
		else if(check_user(msg_from_client))
		{
			char *username=(char *)malloc(40*sizeof(char));
			strcpy(username,msg_from_client);

			int query_result = query(username, clientFD); // handles debits and credit from 
			if(query_result == INTERNAL_ERROR){ 
				close_error(clientFD);   // closing connection due to internal error at server side
				return;
			}	
			if(query_result == EXIT ) break;
		}
		else{
			send_msg_to_client(clientFD,"\n\nWrong Username. Please enter a valid user\n\n");
		}
	}
	close_server_socket(clientFD);
}

//police interaction
void police_queries(int clientFD)
{
	send_msg_to_client(clientFD, "You are logged in as police. \nEnter your choice\n1. Print Balance of all users\n2. Get Mini Statement\nWrite Terminate to terminate");
	int flag=1;

	while(flag)
	{
		char *buff=NULL;
		buff=get_msg_from_client(clientFD);
		char *bal,*str;
		bal=(char *)malloc(1000*sizeof(char));
		str=(char *)malloc(10000*sizeof(char));
		strcpy(bal,"------------------\nAvailable Balance: \n");
		strcpy(str,"------------------\nMini Statement: \n");
		if(strcmp(buff,"Terminate")==0)
			break;
		else
		{
			int choice=atoi(buff);
			if(choice==1)
			{
				strcpy(bal,"\nResponse: balance of all users -----------------\n");
				strcat(bal,get_all_balance());
				send_msg_to_client(clientFD,strcat(bal,"\n--------------------\n\nEnter your choice\n1. Print Balance of all users\n2. Get mini Statement\nWrite Terminate to terminate"));
				if(bal != NULL) 
					free(bal);
			}
			else if(choice==2)
			{
				send_msg_to_client(clientFD,"Enter Username or 'Terminate' to terminate");

				while(1)
				{
					buff=get_msg_from_client(clientFD);

					if(strcmp(buff,"Terminate")==0)
					{
						flag=1;
						break;
					}
					else if(check_user(buff))
					{
						char *username=(char *)malloc(sizeof(char)*40);
						strcpy(username,buff);
						strcpy(str,"\nResponse: Mini Statement of ");
						strcat(str,username);
						strcat(str," -----------------------\n\n");
						strcat(str,print_statement(username,clientFD));
						send_msg_to_client(clientFD,strcat(str,"\n--------------------\n\nEnter your choice\n1. Print Balance of all users\n2. Get Mini Statement\nWrite Terminate to terminate"));
						if(str != NULL) free(str);
						if(username != NULL) free(username);
						if(buff != NULL) free(buff);
						break;
					}
					else
						send_msg_to_client(clientFD,"Wrong Username. Please enter a valid user");
				}
			}
		}
	}
	close_server_socket(clientFD);
}

//for getting list of all balances for police query
char* get_all_balance()
{
	FILE *fp=fopen("login_file","r");
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char* retstr=(char *)malloc(10000*sizeof(char));
    retstr[0]='\0';
    if(fp == NULL){
		strcpy(retstr,"Could not open the user login file. Please terminate the connection\n");
		return retstr;
	}
	while((read = getline(&line, &len, fp)) != -1) 
	{
		char *token=strtok(line," ");
		char *token1=strtok(NULL," ");
		char *token2=strtok(NULL," ");
		if(token == NULL || token1 == NULL || token2 == NULL){
			strcat(retstr,"There's a problem with this entry\n");
			continue;
		}
		if(token2[0]=='C')
		{
			strcat(retstr,token);
			strcat(retstr," ");
			strcat(retstr,print_balance(token));
			strcat(retstr,"\n");
        }
    }
    return retstr;
}

//for finding out balance of single user
char* print_balance(char* username)
{
	FILE* fp=fopen(username,"r");
	char* line = NULL;
    size_t len = 0;
    ssize_t read;

    if(fp == NULL){   // problem with trasaction history file 
    	char *bal=(char *)malloc(100*sizeof(char));
		strcpy(bal,"Could not find the file having your transaction history.\n");
		return bal;
	}


/*
char tmp[1024];

fp = fopen("tmp.dat", "r");

while(!feof(fp))
    fgets(tmp, 1024, fp);

prinf("%s", tmp)
*/	
read =0;
while(!feof(fp))
	{
	read =1;
    getline(&line, &len, fp);
	}
    if(read)
    {
    	char *token,*prevtoken;
    	prevtoken=(char *)malloc(400*sizeof(char));
    	token=strtok(line," \n");
    	while(token!=NULL)
    	{
    		strcpy(prevtoken,token);
    		token=strtok(NULL," \n");
    	}
    	fclose(fp);
    	return prevtoken;
    }
    else
    {
    	fclose(fp);
    	char *bal=(char *)malloc(2*sizeof(char));
    	bal[0]='0';
    	bal[1]='\0';
    	return bal;
    }
}

//for printing the mini statement of a user
char* print_statement(char* username,int clientFD)
{
	FILE *fp = fopen(username,"r");

	char *miniStatement = NULL;

	miniStatement = (char *)malloc(10000*sizeof(char));
	if(fp == NULL){
		strcpy(miniStatement,"Could not find the file having your transaction history.\n");
		return miniStatement;
	}
    miniStatement[0] = '\0';

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int count=0;

	while(count<linesInMS && (read = getline(&line, &len, fp)) != -1)
	{
		strcat(miniStatement,line);
		count++;
	}

	fclose(fp);

	if(strlen(miniStatement)==0)
		strcpy(miniStatement,"You have not had any transactions yet!");

	return miniStatement;
}

//for getting username and password from client
void get_user_pwd(char* username,char* password,int clientFD)
{
	char *ruser,*rpass;
	send_msg_to_client(clientFD,"Enter Username: ");
	ruser=get_msg_from_client(clientFD);

	send_msg_to_client(clientFD,"Enter Password: ");
	rpass=get_msg_from_client(clientFD);

	int i=0;
	while(ruser[i]!='\0' && ruser[i]!='\n')
	{
		username[i]=ruser[i];
		i++;
	}

	username[i]='\0';

	i=0;
	while(rpass[i]!='\0' && rpass[i]!='\n')
	{
		password[i]=rpass[i];
		i++;
	}
	password[i]='\0';
}

//for sending message to client
void send_msg_to_client(int clientFD, char* str) {
	if(str==NULL)
		return;
    int count_packets_send = (strlen(str)-1)/RESPONSE_SIZE + 1;
    int n = write(clientFD, &count_packets_send, sizeof(int));
    char *msgToSend = (char*)malloc(count_packets_send*RESPONSE_SIZE);
    strcpy(msgToSend, str);
    for(int i = 0; i < count_packets_send; ++i) {
        int n = write(clientFD, msgToSend, RESPONSE_SIZE);
        msgToSend += RESPONSE_SIZE;
    }
}

//for getting message from client
char* get_msg_from_client(int clientFD) {
    int count_packets_receive = 0;
    int n = read(clientFD, &count_packets_receive, sizeof(int));
    if(n <= 0) {
        // shutdown(clientFD, SHUT_WR);
        return NULL;
    }
    char *str = (char*)malloc(count_packets_receive*REQUEST_SIZE);
    memset(str, 0, count_packets_receive*REQUEST_SIZE);
    char *str_p = str;
    for(int i = 0; i < count_packets_receive; ++i) {
        int n = read(clientFD, str, REQUEST_SIZE);
        str = str+REQUEST_SIZE;
    }
    return str_p;
}

//for checking if terminate is called
int is_terminate_msg(char* msg_from_client){
	if(msg_from_client == NULL){
		return 1;
	}
	else if(strncmp(msg_from_client,"Terminate",9) == 0){
		return 1;
	}
	else{
		return 0;
	}
}

//main function
int main(int argc, char **argv){

	int sock_fd,clientFD,port_no;
	struct sockaddr_in serv_addr, cli_addr;
	if(argc < 2){
		printf("Input the desired port no \n");  // check correct no of argument is passed 
		return -1;
	}

	memset((void*)&serv_addr, 0, sizeof(serv_addr));
	port_no=atoi(argv[1]);

	sock_fd=socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_port = htons(port_no);         //seting the port number passed as argument
	serv_addr.sin_family = AF_INET;              //setting DOMAIN
	serv_addr.sin_addr.s_addr = INADDR_ANY;      //permits any incoming IP

	if(bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
	    printf("Error on binding.\n");
	    return -1; 
	}

	int socket_reuse=1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &socket_reuse, sizeof(int));

	listen(sock_fd, 7);
	int client_size=sizeof(cli_addr);

	int count = 0;
	while(1) {
	    memset(&cli_addr, 0, sizeof(cli_addr));
	    if((clientFD = accept(sock_fd, (struct sockaddr*)&cli_addr, &client_size)) < 0) {
	        printf("Error on accept.\n");
	        exit(EXIT_FAILURE);
	    }
	    switch(fork()){ // creating a new child process to handle each connection 
	        case -1:
	            printf("Error in fork.\n");
	            close_error(clientFD);
	            break;
	        case 0:
	            close(sock_fd); // in child process don't use original socket fd 
	            printf("%d Accepted\n",clientFD);
	            talk_to_client(clientFD);
	            exit(EXIT_SUCCESS);
	            break;
	        default:
	            break;
	    }
	}

}