#!/bin/sh

x86_64-w64-mingw32-gcc -o implant.exe implant.c encryption/aes.c encryption/crypt.c -lwininet -lwsock32 -I.
gcc -o server server.c encryption/aes.c encryption/crypt.c -lreadline -I.
strip implant.exe