//			gcc dfClient.c -w -o dfc -lcrypto -lssl
//			./dfc Client_2/dfc.conf

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
#include <openssl/md5.h>

#define FILEPACKETSIZE 50*1024	
#define FILEBUFFERSIZE 1048576
#define MAXLINE 4096
char *key = "vipra";
static const struct packet EmptyStruct;

int firstChunkMap[4][4] = {{1,2,3,4}, {4,1,2,3}, {3,4,1,2}, {2,3,4,1}};
int secondChunkMap[4][4] = {{2,3,4,1}, {1,2,3,4}, {4,1,2,3}, {3,4,1,2}};

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

void getIpandPort(char *str, char *ip, char *port) {
	char *tok = strtok(str, ":");
	char *temp = strtok(NULL, "\0");

	strcpy(ip, tok);
	ip[strlen(ip)] = '\0';

	strcpy(port, temp);
	port[strlen(port)] = '\0';
}


//This function parses the dfc.conf to find the configurations.
int getDefaultFileName(char *filename, char *defaultPath, char *dfs1, char *dfs2, char *dfs3, char *dfs4, char *user, char *pwd) {
	FILE *file;
	file = fopen(filename, "r");
	char data[FILEBUFFERSIZE];
	bzero(data, sizeof(data));
	int i = 0;

	char *filePath = strtok(filename, "/");

	strncpy(defaultPath, filePath, strlen(filePath));
	strcat(defaultPath, "/");
	defaultPath[strlen(defaultPath)] = '\0';

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
		fclose(file); 
		return 1;
    } else {
    	printf("File Open Failed for *dfc.conf*\n");
    	return 0;
    }
}

//Gives the size of the file in bytes.
size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
  	size_t file_size = ftell(file);
  	return file_size;
}

int getDecimalOfHexChar(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c == 'a') {
		return 10;
	} else if (c == 'b') {
		return 11;
	} else if (c == 'c') {
		return 12;
	} else if (c == 'd') {
		return 13;
	} else if (c == 'e') {
		return 14;
	} else if (c == 'f') {
		return 15;
	}
}

int getSixteenDigits(int i) {
	int index = i % 5;
	if (index == 1) {
		return 16;
	} else if (index == 2) {
		return 56;
	} else if (index == 3) {
		return 96;
	} else if (index == 4) {
		return 36;
	} else if (index == 0) {
		return 76;
	}
}

long getDecimalValue(char *hex) {
	long decimal = 0;
	decimal = getDecimalOfHexChar(hex[(strlen(hex) - 1)]);
	int index = 1;

	for (int i = strlen(hex) - 2 ; i >= 0 ; i--) {
		
		int d = getDecimalOfHexChar(hex[i]);
		int multiplier = getSixteenDigits(index);
		int product = d * multiplier;
		product = product % 100;
		decimal = decimal + product;
		index++;
	}
	return decimal % 100;
}

long int getMd5Sum(char *fileName) {
	FILE *file;
	char fileBuffer[FILEBUFFERSIZE];
	file = fopen(fileName, "rb");
	if(file == NULL)
    {
      printf("Given File Name does not exist: %s\n", fileName);
      return -1;
    }

    printf("Inside Md5 for file: %s\n", fileName);
    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);

	unsigned char c[64];
	bzero(c, sizeof(c));
	unsigned char c_new[32];
	bzero(c_new, sizeof(c_new));

	bzero(c, sizeof(c));
	MD5_CTX mdContext;
	MD5_Init (&mdContext);

	bzero(fileBuffer, sizeof(fileBuffer));
	int byte_read = fread(fileBuffer, 1, file_size, file);
	MD5_Update (&mdContext, fileBuffer, byte_read);
	MD5_Final (c, &mdContext);
	
	for(int i = 0; i < strlen(c); i++) {
		char temp[3];
		sprintf(temp, "%0x", c[i]);
		//printf("%s", temp);
		strcat(c_new, temp);
	}

	printf("\n%s\n", c_new);
	
	long md51 = getDecimalValue(c_new);

	fclose(file);
	return md51;
}


void getEncryptedData(char *original, char *converted, int size) {
	int i = 0;
	int len = strlen(key);

	while (i < size) {
		converted[i] = original[i] ^ key[i % len];
		i++;
	}
	converted[i] = '\0';
}

int writeFile(char *fileName, char *additionalFileName, char *data, int size, int append) {
	FILE *file;
	char fileNameW[50];
	bzero(fileNameW, sizeof(fileNameW));

	strcpy(fileNameW, fileName);
	strcat(fileNameW, additionalFileName);

	if (append == 0) {
		file = fopen(fileNameW,"wb");
	} else {
		file = fopen(fileNameW,"ab");
	}
	
	int fileSize = fwrite(data , sizeof(unsigned char), size, file);

	if(fileSize < 0)
    {
    	printf("Error writting file\n");
        exit(1);
    }
    fclose(file);
}

/*
chunkNumber range: 1-4

*/
size_t getFileChunk(char *encryptedData, char *fileName, int chunkNum) {

	FILE *file;
	char fileBuffer[FILEBUFFERSIZE];
	size_t chunkSize;
	int byte_read;
	int i = 1;

	file = fopen(fileName, "rb");

	if(file == NULL)
    {
      printf("Given File Name \"%s\" does not exist.\n", fileName);
      return;
    }

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);

	while (1) {
		if (i < 4) {
			chunkSize = file_size / 4;
		} else {
			chunkSize = file_size - (file_size/ 4) * 3;
		}
		bzero(fileBuffer, sizeof(fileBuffer));
		byte_read = fread(fileBuffer, 1, chunkSize, file);
		if (i == chunkNum) {
			break;
		}
		i ++;
	}

	fclose(file);
	
	printf("Chunk:%d     FileSize:%d\t\tchunkSize:%d\n\n", chunkNum, file_size, chunkSize);
	getEncryptedData(fileBuffer, encryptedData, byte_read);
	return chunkSize;
}

void getChunkName(char *chunkName, char *fileName, int chunkNum) {
	char ch[1];
	sprintf(ch, "%d", chunkNum);
	bzero(chunkName, sizeof(chunkName));
	strcpy(chunkName, ".");
	strcat(chunkName, fileName);
	strcat(chunkName, ".");
	strcat(chunkName, ch);
}

void DecryptDataAndWrite(char *encryptedData, char *fileName, char *additionalFileName, int size, int append) {

	char decryptedData[FILEBUFFERSIZE];
	bzero(decryptedData, sizeof(decryptedData));
	
	getEncryptedData(encryptedData, decryptedData, size);
	writeFile(fileName, additionalFileName, decryptedData, size, append);
}

void constructPacketToSend(struct packet *pack, char *username, char *password, char *fileName, int chunkNum1, size_t chunkSize1, char *data1, int chunkNum2, size_t chunkSize2, char *data2) {

	char chunkName[100];
	strcpy(pack->username, username);
	strcpy(pack->password, password);
	
	getChunkName(chunkName, fileName, chunkNum1);
	strcpy(pack->firstFileName, chunkName);
	pack->firstFileSize = chunkSize1;
	bzero(pack->firstFile, sizeof(pack->firstFile));
	memcpy(pack->firstFile, data1, chunkSize1);
	
	getChunkName(chunkName, fileName, chunkNum2);
	strcpy(pack->secondFileName, chunkName);
	pack->secondFileSize = chunkSize2;
	bzero(pack->secondFile, sizeof(pack->secondFile));
	memcpy(pack->secondFile, data2, chunkSize2);
}



int getChunkToSend(int serverNum, long md5, int itemNum) {
	int modVal = md5 % 4;
	if (itemNum == 1) {
		return firstChunkMap[modVal][serverNum] - 1;
	} else if (itemNum == 2) {
		return secondChunkMap[modVal][serverNum] - 1;
	} else {
		return -1;
	}
}


int main (int argc, char **argv)
{
	
	char dfs1[20];
	char dfs2[20];
	char dfs3[20];
	char dfs4[20];
	char defaultPath[30];

	char dfs1Ip[20];
	char dfs2Ip[20];
	char dfs3Ip[20];
	char dfs4Ip[20];

	char dfs1Port[20];
	char dfs2Port[20];
	char dfs3Port[20];
	char dfs4Port[20];

	char username[20];
	char password[20];

	int fileParsed = getDefaultFileName(argv[1], defaultPath, dfs1, dfs2, dfs3, dfs4, username, password);
	
	static const struct packet EmptyStruct;

	if (fileParsed == 0) {
		return 0;
	}
	// printf("dfs1:%s:\n", dfs1);
	// printf("dfs2:%s:\n", dfs2);
	// printf("dfs3:%s:\n", dfs3);
	// printf("dfs4:%s:\n", dfs4);
	printf("username:%s:\n", username);
	printf("password:%s:\n", password);
	printf("defaultPath:%s:\n\n", defaultPath);

/*
	DecryptDataAndWrite(encryptedDataOne, fileName, "_Final", chunkSizeOne, 0);
	DecryptDataAndWrite(encryptedDataTwo, fileName, "_Final", chunkSizeTwo, 1);
	DecryptDataAndWrite(encryptedDataThree, fileName, "_Final", chunkSizeThree, 1);
	DecryptDataAndWrite(encryptedDataFour, fileName, "_Final", chunkSizeFour, 1);
*/
	int sock[4];
	struct sockaddr_in servaddr[4];
	unsigned int serverLength[4];

	char sendline[MAXLINE];
	char recvline[MAXLINE];
	char *fileName;

	//Create a socket for the client
	//If sockfd<0 there was an error in the creation of the socket
	for (int i = 0; i < 4; i++) {
		if ((sock[i] = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("Problem in creating the socket for Server %d.\n", i);
			exit(2);
		}
	}

	//Creation of the socket
	for (int i = 0; i < 4; i++) {
		memset(&servaddr[i], 0, sizeof(servaddr[i]));
	}

	getIpandPort(dfs1, dfs1Ip, dfs1Port);
	getIpandPort(dfs2, dfs2Ip, dfs2Port);
	getIpandPort(dfs3, dfs3Ip, dfs3Port);
	getIpandPort(dfs4, dfs4Ip, dfs4Port);
	
	printf("1. IP:%s\t\tPort:%s\n", dfs1Ip, dfs1Port);
	printf("2. IP:%s\t\tPort:%s\n", dfs2Ip, dfs2Port);
	printf("3. IP:%s\t\tPort:%s\n", dfs3Ip, dfs3Port);
	printf("4. IP:%s\t\tPort:%s\n", dfs4Ip, dfs4Port);

	servaddr[0].sin_family = AF_INET;
	servaddr[0].sin_addr.s_addr= inet_addr(dfs1Ip);
	servaddr[0].sin_port =  htons(atoi(dfs1Port));  //convert to big-endian order

	servaddr[1].sin_family = AF_INET;
	servaddr[1].sin_addr.s_addr= inet_addr(dfs2Ip);
	servaddr[1].sin_port =  htons(atoi(dfs2Port));  //convert to big-endian order

	servaddr[2].sin_family = AF_INET;
	servaddr[2].sin_addr.s_addr= inet_addr(dfs3Ip);
	servaddr[2].sin_port =  htons(atoi(dfs3Port));  //convert to big-endian order

	servaddr[3].sin_family = AF_INET;
	servaddr[3].sin_addr.s_addr= inet_addr(dfs4Ip);
	servaddr[3].sin_port =  htons(atoi(dfs4Port));  //convert to big-endian order

	//Connection of the client to the socket
	for (int i = 0; i < 4; i++) {
		if (connect(sock[i], (struct sockaddr *) &servaddr[i], sizeof(servaddr[i])) < 0) {
			printf("Problem in connecting to the server %d.\n", i);
			//exit(3);
		} else {
			printf("Connection Successful for Server %d.\n", i);
		}
	}

	
	for (int i = 0; i < 4; i++) {
		serverLength[i] = sizeof(servaddr[i]);
	}
	
	while (1) {
		bzero(sendline, sizeof(sendline));
		printf("\n*********************** MENU ***********************\n");
		printf(". put [FileName]\n. get [FileName]\n. mkdir [subfolder]\n. list\n. exit\n\n");
		printf("Enter the operation you want to perform: ");
		
		if(fgets(sendline, MAXLINE, stdin) == NULL) {
			printf("Please enter a valid command.\n");
			continue;
		}

		char *option;
		option = strtok(sendline, "\n");
		printf("--------------------------------------\n");
		printf("You selected:%s:\n\n", option);


		//bzero(fileName, sizeof(fileName));
		char *command;
		//char *filename;
		command = strtok(option, " ");
		printf("command: %s\n", command);
		//Move on only if user has entered something.
		if (command && command != NULL) {
			if (strcmp(command, "get") == 0 || strcmp(command, "put") == 0 || strcmp(command, "mkdir") == 0 ) {
				fileName = strtok(NULL, "");
				if (!fileName) {
					printf("No File Name Entered. Please try again.\n");
					continue;
				}
			}
		} else {
			continue;
		}

		printf("FileName:%s\n", fileName);

		if (strcmp(command, "put") == 0) {
			char absoluteFile[100];
		    bzero(absoluteFile, sizeof(absoluteFile));
		    strcpy(absoluteFile, defaultPath);
		    strcat(absoluteFile, fileName);

		    printf("absoluteFile: %s\n", absoluteFile);

		    long md5 = getMd5Sum(absoluteFile);
			printf("MD5: %ld\n\n", md5);

			if (md5 == -1) {
				printf("File is invalid. Please try again.\n");
				continue;
			}
			char *something[4];
			something[0] = calloc(FILEBUFFERSIZE, sizeof(char));
			something[1] = calloc(FILEBUFFERSIZE, sizeof(char));
			something[2] = calloc(FILEBUFFERSIZE, sizeof(char));
			something[3] = calloc(FILEBUFFERSIZE, sizeof(char));

			size_t chunkSize[4];
			for (int i = 0; i < 4; i++) {
				chunkSize[i] = getFileChunk(something[i], absoluteFile, i + 1);
			}
			
			struct packet pack;
			int nbytes;
			for (int serverIndex = 0; serverIndex < 4; serverIndex++) {
				printf("---------------------Server %d----------------------\n\n", serverIndex);
				int firstChunkNum = getChunkToSend(serverIndex, md5, 1);
				int secondChunkNum = getChunkToSend(serverIndex, md5, 2);

				pack = EmptyStruct;

				constructPacketToSend(&pack, username, password, fileName, firstChunkNum + 1, chunkSize[firstChunkNum], something[firstChunkNum], secondChunkNum + 1, chunkSize[secondChunkNum], something[secondChunkNum]);
				printf("pack.username: %s\n", pack.username);
				printf("pack.password: %s\n", pack.password);
				
				printf("pack.firstFileName: %s\n", pack.firstFileName);
				printf("pack.firstFileSize: %d\n", pack.firstFileSize);
				
				printf("pack.secondFileName: %s\n", pack.secondFileName);
				printf("pack.secondFileSize: %d\n\n", pack.secondFileSize);

				strcpy(pack.command, "put");
				pack.command[strlen(pack.command)] = '\0';

				nbytes = sendto(sock[serverIndex], &pack, sizeof(struct packet), 0, (struct sockaddr *)&servaddr[serverIndex], sizeof(servaddr[serverIndex]));
				if (nbytes < 0){
					printf("Error in sendto to server %d.\n", serverIndex);
				}
				printf("Waiting for server %d ACK..\n", serverIndex);
				struct packet receivedPacket;
				nbytes = recvfrom(sock[serverIndex], &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&servaddr[serverIndex], &serverLength[serverIndex]);  
				
				if (nbytes > 0) {
					printf("Server %d Sent:", serverIndex);
					fputs(receivedPacket.message, stdout);
					printf("\n\n");
					
				} else {
					printf("Negative bytes received from server %d.\n", serverIndex);
				}
			}
		} else if (strcmp(command, "get") == 0) {
			printf("Inside Get command\n");
		} else {
			printf("Invalid command.\n");
		}
	}

	exit(0);
}



// COMPILE: gcc dfClient.c -w -o dfc -lcrypto -lssl
// RUN: ./dfc Client_1/dfc.conf