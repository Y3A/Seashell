# simple RAT in c

+ data is encrypted with AES-256-CBC
+ encryption implementation is taken from https://github.com/kokke/tiny-AES-c

+ change IP and port inside implant.c and server.c
+ compile executable with ./build.sh (need to install mingw and gcc beforehand)
+ run server to listen on attacking machine for reverse shell

implant.exe will attempt to connect to attacker 5 times, with a 10 second pause in between.

Core Functionalities:
	-- default:                      !exec <command>
	-- run shell commands on target: !exec <command>
	-- download file:                !get <target filename>
	-- upload file:                  !put <local filename> (still under development)
	-- exit shell:                   !exit
	-- run shell commands locally:   !local <command>
	-- view help:                    !help