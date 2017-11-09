#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define FILEPACKETSIZE 5*1024	


//The packet struct that will be passed between the client and servers.
struct packet
{
	char username[20];
	char password[20];
	char command[10];	//will contain the command that user entered
	char message[100];	//If command failed, then this will contain the message from server.
	int code;			//200:Pass,  500:Fail
	
	char firstFileName[50];			//First chunk
	char firstFile[FILEPACKETSIZE];
	int firstFileSize;

	char secondFileName[50];		//Second Chunk
	char secondFile[FILEPACKETSIZE];	
	int secondFileSize;
};


//This function parses the dfc.conf to find the configurations.
void getDefaultFileName(char *dfs1, char *dfs2, char *dfs3, char *dfs4, char *user, char *pwd) {
	FILE *file;
	file = fopen("./dfc.conf", "r");
	char data[1048576];
	bzero(data, sizeof(data));
	int i = 0;

    if (file) {
	    while(fgets(data, sizeof(data), file)) {
		    if (i < 4) {
		        char *tok = strtok(data, " ");
		        if (tok != NULL) {
		          char *temp = strtok(NULL, " ");
		          if (strcmp(temp, "DFS1") == 0){
		          	char *tempdouble = strtok(NULL, "\n");
		          	strcpy(dfs1, tempdouble);
		          	dfs1[strlen(dfs1)] = '\0';
		          } else if (strcmp(temp, "DFS2") == 0){
		          	char *tempdouble = strtok(NULL, "\n");
		          	strcpy(dfs2, tempdouble);
		          	dfs2[strlen(dfs2)] = '\0';
		          } else if (strcmp(temp, "DFS3") == 0){
		          	char *tempdouble = strtok(NULL, "\n");
		          	strcpy(dfs3, tempdouble);
		          	dfs3[strlen(dfs3)] = '\0';
		          } else if (strcmp(temp, "DFS4") == 0){
		          	char *tempdouble = strtok(NULL, "\n");
		          	strcpy(dfs4, tempdouble);
		          	dfs4[strlen(dfs4)] = '\0';
		          }
		        }
		    } else if (i == 4) {
		      	char *tokk = strtok(data, ":");
		      	if (tokk != NULL) {
		      		char *tempp = strtok(NULL, "\n");
		      		strcpy(user, tempp);
		          	user[strlen(user)] = '\0';
		      	}
		    } else if (i == 5) {
		      	char *tokkk = strtok(data, ":");
		      	if (tokkk != NULL) {
		      		char *temppp = strtok(NULL, "\n");
		      		strcpy(pwd, temppp);
		          	pwd[strlen(pwd)] = '\0';
		      	}
		    }
	    	i++;
	    }
    } else {
    	printf("File Open Failed for *dfc.conf*\n");
    }

 //    printf("**dfs1:%s:\n", dfs1);
	// printf("**dfs2:%s:\n", dfs2);
	// printf("**dfs3:%s:\n", dfs3);
	// printf("**dfs4:%s:\n", dfs4);
	// printf("**username:%s:\n", user);
	// printf("**password:%s:\n", pwd);
    fclose(file); 
}

int main (int argc, char **argv)
{
	char dfs1[20];
	char dfs2[20];
	char dfs3[20];
	char dfs4[20];

	char username[20];
	char password[20];

	getDefaultFileName(dfs1, dfs2, dfs3, dfs4, username, password);
	printf("dfs1:%s:\n", dfs1);
	printf("dfs2:%s:\n", dfs2);
	printf("dfs3:%s:\n", dfs3);
	printf("dfs4:%s:\n", dfs4);
	printf("username:%s:\n", username);
	printf("password:%s:\n\n", password);

}