//		gcc dfServer.c -o server
//		./server DFS1/ 8886


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
#include <dirent.h>


#define FILEPACKETSIZE 50*1024	
#define MAXLINE 4096
#define LISTENQ 8 /*maximum number of client connections*/


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
		    		strcat(users, ":");
		        	users[strlen(users)] = '\0';
		    	}
		  }
    } else {
    	printf("File Open Failed for %s\n", filename);
    }
    fclose(file); 
}

int isValidUser(char *userReceived, char *validUserList) {
	
	char tempList[1000];
	bzero(tempList, sizeof(tempList));
	strcpy(tempList, validUserList);
	tempList[strlen(tempList)] = '\0';

    char *tokkk = strtok(tempList, ":");
    while (1) {
    	if (tokkk != NULL) {
    		if (strcmp (tokkk, userReceived) == 0) {
    			printf("\nUser is valid.\n");
    			return 1;
    		}
    	} else {
    		return 0;
    	}
    	tokkk = strtok(NULL, ":");
	}
}


int writeFile(char *fileName, char *dir, char *data, int size) {
	FILE *file;
	char fileNameW[50];
	bzero(fileNameW, sizeof(fileNameW));

	strcpy(fileNameW, dir);
	strcat(fileNameW, fileName);
	//printf("fileName:%s\tdir:%s\n", fileName, dir);

	file = fopen(fileNameW,"wb");
	
	int fileSize = fwrite(data , sizeof(unsigned char), size, file);

	if(fileSize < 0)
    {
    	printf("Error writting file\n");
        exit(1);
    }
    printf("File Write Successful for:  %s\n", fileNameW);
    fclose(file);
}

int main (int argc, char **argv)
{
	char users[2000];
	bzero(users, sizeof(users));

	char defaultDir[100];
	bzero(defaultDir, sizeof(defaultDir));
	strcpy(defaultDir, argv[1]);
	
	printf("defaultDir: %s\n", defaultDir);
	if (defaultDir[strlen(defaultDir) - 1] != '/') {
		defaultDir[strlen(defaultDir)] = '/';	
	}
	defaultDir[strlen(defaultDir)] = '\0';

	getDefaultFileName("dfs.conf", users);
	printf("%s\n\n", users);

	int listenfd, connfd, n;
	pid_t childpid;
	socklen_t clilen;
	char buf[MAXLINE];
	struct sockaddr_in cliaddr, servaddr;

	 //Create a socket for the soclet
	 //If sockfd<0 there was an error in the creation of the socket
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
	    perror("Problem in creating the socket");
		exit(2);
 	}

	//preparation of the socket address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[2]));
	int nbytes;
	unsigned int remote_length;

 	//bind the socket
	bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

 	//listen to the socket by creating a connection queue, then wait for clients
	listen (listenfd, LISTENQ);

	printf("%s %s\n","Server running...waiting for connections.", argv[2]);

	while (1) {

		clilen = sizeof(cliaddr);
		//accept a connection
		connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

		printf("%s\n","Received request...");

		if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process

			printf ("%s\n","Child created for dealing with client requests");

			//close listening socket
			close (listenfd);
			remote_length = sizeof(cliaddr);
			struct packet clientPacket;

			while (1) {
				printf("\n---------------------------------------\n");
				
				if (recvfrom(connfd, &clientPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, &remote_length) < 0)
			    {
			    	printf("%s\n", "Read error");
			    	break;
			    }
			    printf("***Received***\n");
			    printf("Username:  %s\n", clientPacket.username);
			    printf("password:  %s\n\n", clientPacket.password);
			    printf("FileName1: %s\n", clientPacket.firstFileName);
			    printf("FileSize1: %d\n\n", clientPacket.firstFileSize);
			    printf("FileName2: %s\n", clientPacket.secondFileName);
			    printf("FileSize2: %d\n\n", clientPacket.secondFileSize);
			    
			    char userReceived[1000];
			    bzero(userReceived, sizeof(userReceived));

			    strcpy(userReceived, clientPacket.username);
			    strcat(userReceived, clientPacket.password);
			    int validUser = isValidUser(userReceived, users);

			    char userDir[100];
			    bzero(userDir, sizeof(userDir));
			    strcpy(userDir, defaultDir);
			    strcat(userDir, clientPacket.username);
			    strcat(userDir, "/");

			    //printf("UserList: %s\n", users);
			    if (validUser == 1) {
			    	printf("User is Valid.\n");
			    	DIR *dir;
			    	int ready = 1;
			    	if ((dir = opendir (userDir)) == NULL) {
			    		ready = 0;
			    		printf("%s directory doesn't exist, creating...\n", userDir);
			    		int st = mkdir(userDir, 0700);
			    		if (st != 0) {
			    			printf("mkdir Failed\n");
			    		} else {
			    			ready = 1;
			    		}
			    	}
			    	if (ready == 1) {
			    		printf("Writing file.\n");
			    		if (clientPacket.firstFileSize > 0 && clientPacket.secondFileSize > 0) {
					    	writeFile(clientPacket.firstFileName, userDir, clientPacket.firstFile, clientPacket.firstFileSize);
					    	writeFile(clientPacket.secondFileName, userDir, clientPacket.secondFile, clientPacket.secondFileSize);
					    	strcpy(clientPacket.message, "Successful\n");
				    	} else {
				    		printf("Size of files sent was 0. Please try again.\n");
				    		strcpy(clientPacket.message, "Size of files sent was 0. Please try again.\n");
				    	}
			    	} else {
			    		printf("Directory failed.\n");
			    		strcpy(clientPacket.message, "Directory Failed\n");
			    	}
			    	closedir (dir);
			    } else {
			    	printf("User is Invalid.\n");
			    	strcpy(clientPacket.message, "Invalid User\n");
			    }

			    
			    nbytes = sendto(connfd, &clientPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
				if (nbytes < 0){
					printf("Error in sendto\n");
				}
			}

			// while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
			// 	printf("%s","String received from and resent to the client:");
			// 	puts(buf);
			//  	send(connfd, buf, n, 0);
			// }

			// if (n < 0)
			// 	printf("%s\n", "Read error");
			
			exit(0);
		}
	 	//close socket of the server
		close(connfd);
	}
}