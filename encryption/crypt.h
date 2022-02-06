#ifndef _CRYPT_H
#define _CRYPT_H

#include <stdint.h>

#ifndef _MYIO
#define _MYIO

#define IN  // param is solely input
#define OUT // modifies param as output

#endif

#define KEY "9adbe0b3033881f88ebd825bcf763b43" //md5 of mykey

uint8_t *encrypt(OUT char[]);
char *decrypt(OUT uint8_t[]);

#endif