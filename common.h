#ifndef _COMMON_H
#define _COMMON_H

#ifndef _MYIO
#define _MYIO

#define IN  // param is solely input
#define OUT // modifies param as output

#endif

char *SERVER_IP = "192.168.1.180";
unsigned long SERVER_PORT = 11521;

#define BUF_SZ 1024 // size of one buffer to send/recv over socket

/* 
	* flags to signal end of a procedure
	* 
	* STATUS_DONE for general end of execution of commands
	* STATUS_END for end of transferring file
	* STATUS_ERR for general error
*/
static const char STATUS_DONE[] = "\033[0;32m[+] Done\033[0m\n";
static const char STATUS_END[] = "\033[0;33m[*] End\033[0m\n";
static const char STATUS_ERR[] = "\033[0;31m[-] Err\033[0m\n";

#define recvbuf(s, buf) recv(s, buf, BUF_SZ, 0)
#define sendbuf(s, buf) send(s, buf, BUF_SZ, 0)

#endif