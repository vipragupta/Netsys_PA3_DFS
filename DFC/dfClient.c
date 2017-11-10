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

int writeFile(char *fileName, char *data, int size, int enc) {
	FILE *file;
	char fileNameW[50];
	bzero(fileNameW, sizeof(fileNameW));

	strcpy(fileNameW, fileName);
	if (enc == 1) {
		strcat(fileNameW, "_e");
	} else {
		strcat(fileNameW, "_d");
	}

	file = fopen(fileNameW,"wb");
	//printf("File Opened:%s \n", fileNameW);	 
	
	int fileSize = fwrite(data , sizeof(unsigned char), size, file);

	if(fileSize < 0)
    {
    	printf("Error writting file\n");
        exit(1);
    }
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

	getDefaultFileName(argv[1], dfs1, dfs2, dfs3, dfs4, username, password);
	printf("dfs1:%s:\n", dfs1);
	printf("dfs2:%s:\n", dfs2);
	printf("dfs3:%s:\n", dfs3);
	printf("dfs4:%s:\n", dfs4);
	printf("username:%s:\n", username);
	printf("password:%s:\n\n", password);

	FILE *file;
	char fileName[50];
	char fileBuffer[FILEBUFFERSIZE];
    bzero(fileName, sizeof(fileName));
    strcpy(fileName, "./Client_1/foo1.txt");

    long md5 = getMd5Sum(fileName);
	printf("MD5: %ld\n\n", md5);

    file = fopen(fileName, "rb");
	if(file == NULL)
    {
      printf("Given File Name \"%s\" does not exist.\n", fileName);
      return;
    }

    size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
	fseek(file, 0, SEEK_SET);
	bzero(fileBuffer, sizeof(fileBuffer));

	char encryptedData[FILEBUFFERSIZE];
	bzero(encryptedData, sizeof(encryptedData));
	
	int index = 0;
	
	char charRead;
	int byteCount = 0;

	int byte_read = fread(fileBuffer, 1, file_size, file);
	char decryptedData[byte_read];

	fclose(file);

	getEncryptedData(fileBuffer, encryptedData, byte_read);
	writeFile(fileName, encryptedData, byte_read, 1);
	printf("size:%d\n\nEncryptData: %s\n", byte_read, encryptedData);
	
	getEncryptedData(encryptedData, decryptedData, byte_read);
	writeFile(fileName, decryptedData, byte_read, 0);
	printf("\nDecryptData: %s\n", decryptedData);
}

// COMPILE: gcc dfClient.c -w -o dfc -lcrypto -lssl
// RUN: ./dfc Client_1/dfc.conf