# PA3_DFS
Distributed File System

The code containes 2 files dfClient.c and dfServer.c. The are the 2 main files containing the client ad server code.

## Functionalities

1. **put [FileName] [subfolder]**  -- send 4 chunks of file from client to each server
2. **get [FileName] [subfolder]**  -- receive chunks of file from server to client
3. **list [subfolder]**            -- get the list of files that server stores
4. **mkdir  [subfolder]**		   -- make the directory in server.

If user enters any other command except the above 4, Client doesn't send anything to the server.

For packet transfer, I have used a struct which has the following structure:
```
struct packet
{
	char username[20];		
	char password[20];
	char command[10];				//will contain the command that user entered
	char message[100];				//If command failed, then this will contain the message from server.
	int code;						//200:Pass,  500:Fail
	char subfolder[100];

	char firstFileName[50];			//First chunk
	char firstFile[FILEPACKETSIZE];
	int firstFileSize;

	char secondFileName[50];		//Second Chunk
	char secondFile[FILEPACKETSIZE];	
	int secondFileSize;
};
```
where FILEPACKETSIZE is  5*1024  i.e. 5 KB.

## How to run:

# Server:
gcc dfServer.c -o server
./server DFS1/ 8886

# client:
gcc dfClient.c -w -o dfc -lcrypto -lssl
./dfc Client_2/dfc.conf