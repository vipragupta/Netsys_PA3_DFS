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


//This function parses the dfc.conf to find the configurations.
void getDefaultFileName(char *filename, char *dfs1, char *dfs2, char *dfs3, char *dfs4, char *user, char *pwd) {
	FILE *file;
	file = fopen(filename, "r");
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

long int getMd5Sum(char *fileName) {
	FILE *file;
	char fileBuffer[1048576];
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
	
	for(int i = strlen(c) - 1; i < strlen(c); i++) {
		char temp[3];
	    sprintf(temp, "%0x", c[i]);
	    strcat(c_new, temp);
	}
	printf("\nConverted hash: %s\n", c_new);
	long int md5;
	char *somethin1g;
	
	md5 = strtol(c_new, somethin1g, 16);

	fclose(file);
	return md5;
}

void getDecryptedData(char *encryptedData, char *decryptedData, int size) {
	int i = 0;
	int index = 0;

	while (i < size) {
		decryptedData[i] = encryptedData[i] ^ key[index++];
		if (index == strlen(key)) {
			index = 0;
		}
		i++;
	}
	decryptedData[i] = '\0';
	//printf("\ndecryptedData: %s\n", decryptedData);
}

int main (int argc, char **argv)
{
	char dfs1[20];
	char dfs2[20];
	char dfs3[20];
	char dfs4[20];

	char username[20];
	char password[20];

	getDefaultFileName(argv[1], dfs1, dfs2, dfs3, dfs4, username, password);
	printf("dfs1:%s:\n", dfs1);
	printf("dfs2:%s:\n", dfs2);
	printf("dfs3:%s:\n", dfs3);
	printf("dfs4:%s:\n", dfs4);
	printf("username:%s:\n", username);
	printf("password:%s:\n\n", password);

	FILE *file;
	char fileName[50];
	char fileBuffer[1048576];
    bzero(fileName, sizeof(fileName));
    strcpy(fileName, "./Client_1/foo1.txt");

    long int md5 = getMd5Sum(fileName);
	printf("MD5: %lld\n", md5);

    file = fopen(fileName, "rb");
	if(file == NULL)
    {
      printf("Given File Name \"%s\" does not exist.\n", fileName);
      return;
    }

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);
	bzero(fileBuffer, sizeof(fileBuffer));
	//int byte_read = fread(fileBuffer, 1, file_size, file);

	char encryptedData[1048576];
	bzero(encryptedData, sizeof(encryptedData));
	
	int index = 0;
	
	
	char charRead;
	int byteCount = 0;
	int size = 0;

	while(1) {
		charRead = fgetc(file);
		if (charRead == EOF) {
			break;
		}

		char ch = charRead ^ key[index++];
		encryptedData[size++] = ch;

		if (index == strlen(key)) {
			index = 0;
		}
	}
	encryptedData[size] = '\0';
	fclose(file);

	char decryptedData[size];
	getDecryptedData(encryptedData, decryptedData, size);
	printf("decryptedData: %s\n", decryptedData);
}

// COMPILE: gcc dfClient.c -w -o dfc -lcrypto -lssl
// RUN: ./dfc Client_1/dfc.conf