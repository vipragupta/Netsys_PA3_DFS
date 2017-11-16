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
		    		strcat(users, "\n");
		        	users[strlen(users)] = '\0';
		    	}
		  }
    } else {
    	printf("File Open Failed for %s\n", filename);
    }
    fclose(file); 
}

int main (int argc, char **argv)
{
	char users[2000];
	bzero(users, sizeof(users));

	getDefaultFileName(argv[1], users);
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

			while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
				printf("%s","String received from and resent to the client:");
				puts(buf);
			 	send(connfd, buf, n, 0);
			}

			if (n < 0)
				printf("%s\n", "Read error");
			
			exit(0);
		}
	 	//close socket of the server
		close(connfd);
	}
}