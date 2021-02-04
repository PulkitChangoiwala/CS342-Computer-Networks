#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	printf("Enter username\n");
	char* username = (char*)malloc(50*sizeof(char));
	char* password =  (char*)malloc(30*sizeof(char));
	scanf("%s",username);
	printf("Enter password\n");
	scanf("%s",password);
	int mode;
	printf("Enter user type: 1 for user, 2 for police, 3 for admin\n");
	scanf("%d",&mode);
	char* buff = (char*)malloc(80 *sizeof(char));
	buff[0]='\0';
	strcat(buff,username);
	strcat(buff," ");
	strcat(buff,password);
	strcat(buff," ");

	if(mode== 1){
		strcat(buff,"C\n");
	}
	else if(mode == 2){
		strcat(buff,"P\n");
	}
	else if(mode == 3){
		strcat(buff,"A\n");
	}
	printf("%s\n",buff );

	FILE * fp = fopen("login_file","a");
	if(fp == NULL){
		printf("Could not open login_file\n");
		return -1;
	}
	fprintf(fp, "%s", buff); 
	fclose(fp);
	if(mode == 1){
		FILE * fp1 = fopen(username,"a");
		if(fp1 != NULL ){fclose(fp1);}
		else{
			printf("Could not create the file\n");
		}
	}
	return 0;
}