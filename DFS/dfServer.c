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
	char subfolder[100];

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
    			//printf("\nUser is valid.\n");
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

//Gives the size of the file in bytes.
size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
  	size_t file_size = ftell(file);
  	return file_size;
}

size_t getFileChunk(char *fileBuffer, char *directory, char *username, char *addrDir, char *fileName, char *chunkName, int chunkNum) {
	//printf("directory: %s\tusername: %s\tfileName: %s\tchunkName: %s\tchunkNum: %d\n", directory, username, fileName, chunkName, chunkNum);
	FILE *file;
	
	size_t chunkSize;
	int byte_read;

    char ch[1];
	sprintf(ch, "%d", chunkNum);
    bzero(chunkName, sizeof(chunkName));
    
    strcpy(chunkName, directory);
    strcat(chunkName, username);
    strcat(chunkName, "/");
    if (strlen(addrDir) > 0) {
    	strcat(chunkName, addrDir);
    }
    strcat(chunkName, ".");
    strcat(chunkName, fileName);
    strcat(chunkName, ".");
    strcat(chunkName, ch);

    printf("chunkName: %s\n", chunkName);

    file = fopen(chunkName, "rb");

	if(file == NULL)
    {
      return -1;
    }

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);

	bzero(fileBuffer, sizeof(fileBuffer));
	byte_read = fread(fileBuffer, 1, file_size, file);

	fclose(file);
	
	printf("Chunk:%d     FileSize:%lu\n\n", chunkNum, file_size);
	return file_size;
}


int createDir(char *userDir) {

    DIR *dir;
	if ((dir = opendir (userDir)) == NULL) {
		printf("%s directory doesn't exist, creating...\n", userDir);
		int st = mkdir(userDir, 0700);
		if (st != 0) {
			printf("mkdir Failed\n");
			closedir (dir);
			return 0;
		} else {
			closedir (dir);
			return 1;
		}
	} else {
		closedir (dir);
		return 1;
	}
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

	DIR *dir;
	if ((dir = opendir (defaultDir)) == NULL) {
		printf("%s directory doesn't exist, creating...\n", defaultDir);
		int st = mkdir(defaultDir, 0700);
		if (st != 0) {
			printf("mkdir for %s Failed\n", defaultDir);
			return 0;
		}
	}

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
			printf("********************************************************\n");
			printf ("%s\n","Child created for dealing with client requests");

			//close listening socket
			close (listenfd);
			remote_length = sizeof(cliaddr);
			struct packet clientPacket;

			while (1) {
				printf("\n---------------------------------------------------------------------------\n");
				
				if (recvfrom(connfd, &clientPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, &remote_length) < 0)
			    {
			    	printf("%s\n", "Error in Receive From.");
			    	break;
			    }
			    printf("*****************************Received******************************\n");
			    printf("Username:  %s\n", clientPacket.username);
			    printf("password:  %s\n\n", clientPacket.password);
			    printf("command:   %s\n", clientPacket.command);
			    printf("subfolder: %s\n", clientPacket.subfolder);
			    
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

			    	if (strcmp(clientPacket.command, "put") == 0) {
			    		printf("Inside put command\n");
				    	
				    	int dirPresent = createDir(userDir);
				    	if (strlen(clientPacket.subfolder) > 0 &&  dirPresent == 1) {
				    		strcat(userDir, clientPacket.subfolder);
			    			strcat(userDir, "/");
				    		dirPresent = createDir(userDir);
				    	}
				    	printf("userDir: %s\n", userDir);
				    	if (dirPresent == 1) {
				    		printf("Writing file.\n");
				    		if (clientPacket.firstFileSize > 0 && clientPacket.secondFileSize > 0) {
						    	writeFile(clientPacket.firstFileName, userDir, clientPacket.firstFile, clientPacket.firstFileSize);
						    	writeFile(clientPacket.secondFileName, userDir, clientPacket.secondFile, clientPacket.secondFileSize);
						    	struct packet sendPacket;
						    	strcpy(sendPacket.message, "Successful\n");
					    		sendPacket.code = 200;
					    		strcpy(sendPacket.command, "put");
					    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
								if (nbytes < 0){
									printf("Error in sendto\n");
								}
					    		//printf("Leaving writing if.\n");
					    	} else {
					    		struct packet sendPacket;
					    		printf("Size of files sent was 0. Please try again.\n");
					    		strcpy(sendPacket.message, "Size of files sent was 0. Please try again.\n");
					    		sendPacket.code = 500;

					    		printf("\n*********Sending***********\n");
							    printf("code:      %d\n", sendPacket.code);
							    printf("message:   %s\n", sendPacket.message); 
							    strcpy(sendPacket.command, "put");
					    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
								if (nbytes < 0){
									printf("Error in sendto\n");
								}
					    	}
				    	} else {
				    		struct packet sendPacket;
				    		printf("Directory failed.\n");
				    		strcpy(sendPacket.message, "Directory Failed\n");
				    		sendPacket.code = 500;
				    		printf("\n*********Sending***********\n");
						    printf("code:      %d\n", sendPacket.code);
						    printf("message:   %s\n", sendPacket.message); 
						    strcpy(sendPacket.command, "put");

				    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
							if (nbytes < 0){
								printf("Error in sendto\n");
							}
				    	}

			    	} else if (strcmp(clientPacket.command, "get") == 0) {
			    		struct packet sendPacket;
			    		printf("Inside Get Command\n\n");
			    		int isOne = 0;
			    		int addedFiles = 0;

			    		char getFileName[50];
		    			bzero(getFileName, sizeof(getFileName));
		    			strcpy(getFileName, clientPacket.firstFileName);
		    			getFileName[strlen(getFileName)] = '\0';

			    		for (int chunkNum = 0; chunkNum < 4; chunkNum++) {
				    		char chunkName[50];
				    		bzero(chunkName, sizeof(chunkName));
				    		
				    		char fileBuffer[FILEPACKETSIZE];
				    		bzero(fileBuffer, sizeof(fileBuffer));

				    		size_t chunkSize = -1;
				    		chunkSize = getFileChunk(fileBuffer, defaultDir, clientPacket.username, clientPacket.subfolder, getFileName, chunkName, chunkNum + 1);
				    		if (chunkSize != -1) {
				    			if (isOne == 0) {
				    				bzero(sendPacket.firstFileName, sizeof(sendPacket.firstFileName));
				    				bzero(sendPacket.firstFile, sizeof(sendPacket.firstFile));
				    				
				    				sendPacket.firstFileSize = chunkSize;
				    				strcpy(sendPacket.firstFileName, chunkName);
				    				memcpy(sendPacket.firstFile, fileBuffer, chunkSize);
				    				isOne = 1;
				    			} else {
				    				bzero(sendPacket.secondFileName, sizeof(sendPacket.secondFileName));
				    				bzero(sendPacket.secondFile, sizeof(sendPacket.secondFile));
				    				
				    				sendPacket.secondFileSize = chunkSize;
				    				strcpy(sendPacket.secondFileName, chunkName);
				    				memcpy(sendPacket.secondFile, fileBuffer, chunkSize);
				    			}
				    			addedFiles++;
				    		}

			    		}
			    		if (addedFiles > 0) {
			    			strcpy(sendPacket.message, "Successful");
			    			sendPacket.code = 200;

			    			printf("\n*********Sending***********\n");
						    printf("Username:  %s\n", sendPacket.username);
						    printf("password:  %s\n\n", sendPacket.password);
						    printf("command:   %s\n", sendPacket.command); 
						    printf("code:      %d\n", sendPacket.code);
						    printf("message:   %s\n", sendPacket.message); 
						    
						    printf("FileName1: %s\n", sendPacket.firstFileName);
						    printf("FileSize1: %d\n\n", sendPacket.firstFileSize);
						    printf("File1:     %s\n\n", sendPacket.firstFile);
						    
						    printf("FileName2: %s\n", sendPacket.secondFileName);
						    printf("FileSize2: %d\n\n", sendPacket.secondFileSize);
						    printf("File2:     %s\n\n", sendPacket.secondFile);

						    printf("Size OF Packet: %lu\n", sizeof(sendPacket));
						    strcpy(sendPacket.command, "get");
			    			nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
							if (nbytes < 0){
								printf("Error in sendto\n");
							}
			    		} else {
			    			strcpy(sendPacket.message, "Fail");
			    			sendPacket.code = 500;
			    			printf("********************Sending*****************************\n");
							printf("code:      %d\n", sendPacket.code);
				 		    printf("message:   %s\n", sendPacket.message);
				 		    strcpy(sendPacket.command, "get");
			    			nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
							if (nbytes < 0){
								printf("Error in sendto\n");
							}
			    		}

			    		
			    	} else if (strcmp(clientPacket.command, "mkdir") == 0) {
				    	char userDir[100];
					    bzero(userDir, sizeof(userDir));
					    strcpy(userDir, defaultDir);
					    strcat(userDir, clientPacket.username);
					    strcat(userDir, "/");
					    
					    int dirPresent = createDir(userDir);
					    if (dirPresent == 1) {
					    	strcat(userDir, clientPacket.firstFileName);
					    	strcat(userDir, "/");
					    	int dirPresent = createDir(userDir);
					    	
					    	if (dirPresent == 1) {
					    		struct packet sendPacket;
					    		printf("Directory Created.\n");
					    		strcpy(sendPacket.message, "Directory Created");
					    		sendPacket.code = 200;
					    		printf("********************Sending*****************************\n");
								printf("code:      %d\n", sendPacket.code);
					 		    printf("message:   %s\n", sendPacket.message);
					 		    strcpy(sendPacket.command, "mkdir");
					    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
								if (nbytes < 0){
									printf("Error in sendto\n");
								}
					    	} else {
					    		struct packet sendPacket;
					    		printf("Directory failed.\n");
					    		strcpy(sendPacket.message, "Directory Failed\n");
					    		sendPacket.code = 500;
					    		printf("********************Sending*****************************\n");
								printf("code:      %d\n", sendPacket.code);
					 		    printf("message:   %s\n", sendPacket.message);
					 		    strcpy(sendPacket.command, "mkdir");
					    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
								if (nbytes < 0){
									printf("Error in sendto\n");
								}
					    	}
					    } else {
					    	struct packet sendPacket;
					    	printf("Directory failed.\n");
				    		strcpy(sendPacket.message, "Directory Failed\n");
				    		sendPacket.code = 500;
				    		printf("********************Sending*****************************\n");
							printf("code:      %d\n", sendPacket.code);
				 		    printf("message:   %s\n", sendPacket.message);
				 		    strcpy(sendPacket.command, "mkdir");
				    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
							if (nbytes < 0){
								printf("Error in sendto\n");
							}
					    }
			     	} else if(strcmp(clientPacket.command, "list") == 0) {
			     		DIR *dir;
						struct dirent *ent;
						struct packet pack;
						
						char userDir[100];
					    bzero(userDir, sizeof(userDir));
					    strcpy(userDir, defaultDir);
					    strcat(userDir, clientPacket.username);
					    strcat(userDir, "/");

					    if (strlen(clientPacket.firstFileName) > 0) {
					    	strcat(userDir, clientPacket.firstFileName);
					    	strcat(userDir, "/");
					    }

					    char files[300];
					    bzero(files, sizeof(files));

						int fileNumber = 0;
						//Check if directory exist.
						if ((dir = opendir (userDir)) != NULL) {
						    while ((ent = readdir (dir)) != NULL) {
						    	//Add the file name only if its not . or ..
						    	if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 ) {
							    	if (fileNumber == 0) {
							    		strcpy((char *) files, ent->d_name);
							    	} else {
							    		strcat((char *) files, ent->d_name);
							    	}
							    	//Concatenate all filenames separated by #
							    	strcat((char *)files, "#");

									fileNumber++;
								}
							}
							closedir (dir);
						}

						struct packet sendPacket;
						bzero(sendPacket.firstFile, sizeof(sendPacket.firstFile));
						strcpy(sendPacket.firstFile, files);
						sendPacket.code = 200;
						strcpy(sendPacket.command, "list");
						strcpy(sendPacket.message, "Successful");
						printf("********************Sending*****************************\n");
						printf("code:      %d\n", sendPacket.code);
			 		    printf("message:   %s\n", sendPacket.message);
			 		    printf("files:     %s\n", sendPacket.firstFile);

						printf("\n");
						nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
						
						if (nbytes < 0){
							printf("Error in sendto\n");
						}

			        } else {
			        	struct packet sendPacket;
			    		printf("Invalid command.\n");
			    		strcpy(sendPacket.message, "Invalid Command\n");
			    		sendPacket.code = 500;
			    		printf("********************Sending*****************************\n");
						printf("code:      %d\n", sendPacket.code);
			 		    printf("message:   %s\n", sendPacket.message);
			 		    strcpy(sendPacket.command, clientPacket.command);
			    		nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
						if (nbytes < 0){
							printf("Error in sendto\n");
						}
			    	}

			    } else {
			    	struct packet sendPacket;
			    	printf("Invalid User/Password. Please try again.\n");
			    	strcpy(sendPacket.message, "Invalid User/Password. Please try again.\n");
			    	sendPacket.code = 500;
			    	printf("********************Sending*****************************\n");
					printf("code:      %d\n", sendPacket.code);
		 		    printf("message:   %s\n", sendPacket.message);
			    	nbytes = sendto(connfd, &sendPacket, sizeof(struct packet), 0, (struct sockaddr *)&cliaddr, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
			    }
			}
			exit(0);
		}
	 	//close socket of the server
		close(connfd);
	}

}