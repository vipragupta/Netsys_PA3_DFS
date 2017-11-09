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
void getDefaultFileName(char *filename, char *users) {
	FILE *file;
	file = fopen(filename, "r");
	char data[1048576];
	bzero(data, sizeof(data));

    if (file) {
	    while(fgets(data, sizeof(data), file)) {
		    
	      	char *tokkk = strtok(data, " ");
	      	if (tokkk != NULL) {
	      		char *temppp = strtok(NULL, "\n");
	      		if (strlen(users) == 0) {
	      			strcpy(users, tokkk);
	      		} else {
	      			strcat(users, tokkk);
	      		}
	      		strcat(users, temppp);
	      		strcat(users, "\n");
	          	users[strlen(users)] = '\0';
	      	}
	    }
    } else {
    	printf("File Open Failed for *dfc.conf*\n");
    }
    fclose(file); 
}

int main (int argc, char **argv)
{
	char users[2000];
	bzero(users, sizeof(users));

	getDefaultFileName(argv[1], users);
	printf("%s\n\n", users);

}