#!/bin/sh

x86_64-w64-mingw32-gcc -o backdoor.exe backdoor.c -lwininet -lwsock32
gcc -o server server.c
strip backdoor.exe
