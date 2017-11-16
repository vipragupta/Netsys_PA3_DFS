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

#define FILEPACKETSIZE 5*1024	
#define FILEBUFFERSIZE 1048576
#define MAXLINE 4096
char *key = "vipra";

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
void getDefaultFileName(char *filename, char *dfs1, char *dfs2, char *dfs3, char *dfs4, char *user, char *pwd) {
	FILE *file;
	file = fopen(filename, "r");
	char data[FILEBUFFERSIZE];
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

    // printf("**dfs1:%s:\n", dfs1);
	// printf("**dfs2:%s:\n", dfs2);
	// printf("**dfs3:%s:\n", dfs3);
	// printf("**dfs4:%s:\n", dfs4);
	// printf("**username:%s:\n", user);
	// printf("**password:%s:\n", pwd);
    fclose(file); 
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
		//printf("decimal: %d\t\tchar: %c\t\td: %d\t\tmultiplier: %d\t\tproduct:%d\t\t\n", decimal, hex[i], d, multiplier, product);
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

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);

	unsigned char c[64];
	unsigned char c_new[32];

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
			 strcat(c_new, temp);
	}

	char *somethin1g;
	printf("MD5 Hash: %s\n", c_new);
	
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

	file = fopen(fileName, "rb");
	if(file == NULL)
    {
      printf("Given File Name \"%s\" does not exist.\n", fileName);
      return;
    }

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);
	

	size_t chunkSize;
	

	int byte_read;
	int i = 1;

	while (1) {
		if (i < 4) {
			chunkSize = file_size / 4;
		} else {
			chunkSize = file_size - (file_size/ 4) * 3;
		}
		bzero(fileBuffer, sizeof(fileBuffer));
		byte_read = fread(fileBuffer, 1, chunkSize, file);
		//printf("Chunk:%d     fileBuffer:%s:\n", chunkNum, fileBuffer);
		if (i == chunkNum) {
			break;
		}
		i ++;
	}

	fclose(file);
	
	printf("Chunk:%d     FileSize:%d\t\tchunkSize:%d\nfileBuffer:%s:\n\n\n", chunkNum, file_size, chunkSize, fileBuffer);

	getEncryptedData(fileBuffer, encryptedData, byte_read);
	//printf("byteRead:%d\t\tEncryptData: %s\n\n", byte_read, encryptedData);
	
	char fileChunkName[50];
	bzero(fileChunkName, sizeof(fileChunkName));
	strcpy(fileChunkName, fileName);
	strcat(fileChunkName, "_");
	fileChunkName[sizeof(fileChunkName)] = (char) chunkNum;
	//strcat(fileChunkName, (char)chunkNum);

	//writeFile(fileChunkName, "", encryptedData, byte_read, 1);
	
	return chunkSize;
}

void DecryptDataAndWrite(char *encryptedData, char *fileName, char *additionalFileName, int size, int append) {

	char decryptedData[FILEBUFFERSIZE];
	bzero(decryptedData, sizeof(decryptedData));
	
	getEncryptedData(encryptedData, decryptedData, size);
	writeFile(fileName, additionalFileName, decryptedData, size, append);
}

int main (int argc, char **argv)
{
	
	char dfs1[20];
	char dfs2[20];
	char dfs3[20];
	char dfs4[20];

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

	getDefaultFileName(argv[1], dfs1, dfs2, dfs3, dfs4, username, password);
	// printf("dfs1:%s:\n", dfs1);
	// printf("dfs2:%s:\n", dfs2);
	// printf("dfs3:%s:\n", dfs3);
	// printf("dfs4:%s:\n", dfs4);
	printf("username:%s:\n", username);
	printf("password:%s:\n\n", password);

	/*
	char fileName[50];
    bzero(fileName, sizeof(fileName));
    strcpy(fileName, "./Client_1/foo1");

    long md5 = getMd5Sum(fileName);
	printf("MD5: %ld\n\n", md5);

	char encryptedDataOne[FILEBUFFERSIZE];
	bzero(encryptedDataOne, sizeof(encryptedDataOne));
	char encryptedDataTwo[FILEBUFFERSIZE];
	bzero(encryptedDataTwo, sizeof(encryptedDataTwo));
	char encryptedDataThree[FILEBUFFERSIZE];
	bzero(encryptedDataThree, sizeof(encryptedDataThree));
	char encryptedDataFour[FILEBUFFERSIZE];
	bzero(encryptedDataFour, sizeof(encryptedDataFour));

	size_t chunkSizeOne = getFileChunk(encryptedDataOne, fileName, 1);
	size_t chunkSizeTwo = getFileChunk(encryptedDataTwo, fileName, 2);
	size_t chunkSizeThree = getFileChunk(encryptedDataThree, fileName, 3);
	size_t chunkSizeFour = getFileChunk(encryptedDataFour, fileName, 4);
	


	DecryptDataAndWrite(encryptedDataOne, fileName, "_Final", chunkSizeOne, 0);
	DecryptDataAndWrite(encryptedDataTwo, fileName, "_Final", chunkSizeTwo, 1);
	DecryptDataAndWrite(encryptedDataThree, fileName, "_Final", chunkSizeThree, 1);
	DecryptDataAndWrite(encryptedDataFour, fileName, "_Final", chunkSizeFour, 1);
*/
	int sock1, sock2, sock3, sock4;
	struct sockaddr_in servaddr1;
	struct sockaddr_in servaddr2;
	struct sockaddr_in servaddr3;
	struct sockaddr_in servaddr4;
	char sendline[MAXLINE];
	char recvline1[MAXLINE];
	char recvline2[MAXLINE];
	char recvline3[MAXLINE];
	char recvline4[MAXLINE];

	//Create a socket for the client
	//If sockfd<0 there was an error in the creation of the socket
	if ((sock1 = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Problem in creating the socket for Server 1.");
		exit(2);
	}
	if ((sock2 = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Problem in creating the socket for Server 2.");
		exit(2);
	}
	if ((sock3 = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Problem in creating the socket for Server 3.");
		exit(2);
	}
	if ((sock4 = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Problem in creating the socket for Server 4.");
		exit(2);
	}


	//Creation of the socket
	memset(&servaddr1, 0, sizeof(servaddr1));
	memset(&servaddr2, 0, sizeof(servaddr2));
	memset(&servaddr3, 0, sizeof(servaddr3));
	memset(&servaddr4, 0, sizeof(servaddr4));

	getIpandPort(dfs1, dfs1Ip, dfs1Port);
	getIpandPort(dfs2, dfs2Ip, dfs2Port);
	getIpandPort(dfs3, dfs3Ip, dfs3Port);
	getIpandPort(dfs4, dfs4Ip, dfs4Port);
	
	printf("1. IP:%s\t\tPort:%s\n", dfs1Ip, dfs1Port);
	printf("2. IP:%s\t\tPort:%s\n", dfs2Ip, dfs2Port);
	printf("3. IP:%s\t\tPort:%s\n", dfs3Ip, dfs3Port);
	printf("4. IP:%s\t\tPort:%s\n", dfs4Ip, dfs4Port);

	servaddr1.sin_family = AF_INET;
	servaddr1.sin_addr.s_addr= inet_addr(dfs1Ip);
	servaddr1.sin_port =  htons(atoi(dfs1Port));  //convert to big-endian order

	servaddr2.sin_family = AF_INET;
	servaddr2.sin_addr.s_addr= inet_addr(dfs2Ip);
	servaddr2.sin_port =  htons(atoi(dfs2Port));  //convert to big-endian order

	servaddr3.sin_family = AF_INET;
	servaddr3.sin_addr.s_addr= inet_addr(dfs3Ip);
	servaddr3.sin_port =  htons(atoi(dfs3Port));  //convert to big-endian order

	servaddr4.sin_family = AF_INET;
	servaddr4.sin_addr.s_addr= inet_addr(dfs4Ip);
	servaddr4.sin_port =  htons(atoi(dfs4Port));  //convert to big-endian order

	//Connection of the client to the socket
	if (connect(sock1, (struct sockaddr *) &servaddr1, sizeof(servaddr1)) < 0) {
		perror("Problem in connecting to the server 1");
		exit(3);
	} else {
		printf("Connection Successful for Server 1.\n");
	}
	// if (connect(sock2, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0) {
	// 	perror("Problem in connecting to the server 2");
	// 	exit(3);
	// } else {
	// 	printf("Connection Successful for Server 2.\n");
	// }
	// if (connect(sock3, (struct sockaddr *) &servaddr3, sizeof(servaddr3)) < 0) {
	// 	perror("Problem in connecting to the server 3");
	// 	exit(3);
	// } else {
	// 	printf("Connection Successful for Server 3.\n");
	// }
	// if (connect(sock4, (struct sockaddr *) &servaddr4, sizeof(servaddr4)) < 0) {
	// 	perror("Problem in connecting to the server 4");
	// 	exit(3);
	// } else {
	// 	printf("Connection Successful for Server 4.\n");
	// }
	while (fgets(sendline, MAXLINE, stdin) != NULL) {


		struct packet pack;
		int nbytes;
		unsigned int server_length = sizeof(servaddr1);

		strcpy(pack.username, username);
		strcpy(pack.password, password);
		nbytes = sendto(sock1, &pack, sizeof(struct packet), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));

		if (nbytes < 0){
			printf("Error in sendto\n");
		}

		printf("Waiting for server ack..\n");
		struct packet receivedPacket;
		nbytes = recvfrom(sock1, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&servaddr1, &server_length);  
		
		if (nbytes > 0) {
			printf("%s", "Server Sent:");
			fputs(receivedPacket.message, stdout);
		} else {
			printf("Negative bytes received.\n");
		}

/*

		send(sock1, sendline, strlen(sendline), 0);
		send(sock2, sendline, strlen(sendline), 0);
		send(sock3, sendline, strlen(sendline), 0);
		send(sock4, sendline, strlen(sendline), 0);

		if (recv(sock1, recvline1, MAXLINE,0) == 0){
			//error: server terminated prematurely
			perror("The server 1 terminated prematurely");
			exit(4);
		}
		printf("%s", "String received from the server 1: ");
		fputs(recvline1, stdout);

		if (recv(sock2, recvline2, MAXLINE,0) == 0){
			//error: server terminated prematurely
			perror("The server 2 terminated prematurely");
			exit(4);
		}
		printf("%s", "String received from the server 2: ");
		fputs(recvline2, stdout);

		if (recv(sock3, recvline3, MAXLINE,0) == 0){
			//error: server terminated prematurely
			perror("The server 3 terminated prematurely");
			exit(4);
		}
		printf("%s", "String received from the server 3: ");
		fputs(recvline3, stdout);

		if (recv(sock4, recvline4, MAXLINE,0) == 0){
			//error: server terminated prematurely
			perror("The server 4 terminated prematurely");
			exit(4);
		}

		printf("%s", "String received from the server 4: ");
		fputs(recvline4, stdout);

		*/
	}

	exit(0);
}



// COMPILE: gcc dfClient.c -w -o dfc -lcrypto -lssl
// RUN: ./dfc Client_1/dfc.conf