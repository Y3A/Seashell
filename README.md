# simple RAT in c

+ data is encrypted with AES-256-CBC
+ encryption implementation is taken from https://github.com/kokke/tiny-AES-c

+ change IP and port inside implant.c and server.c
+ compile executable with ./build.sh (need to install mingw and gcc beforehand)
+ run server to listen on attacking machine for reverse shell

implant.exe will attempt to connect to attacker 5 times, with a 10 second pause in between.

Core Functionalities:<br/>
	-- default:                      !exec <command><br/>
	-- run shell commands on target: !exec <command><br/>
	-- download file:                !get <target filename><br/>
	-- upload file:                  !put <local filename> (still under development)<br/>
	-- exit shell:                   !exit<br/>
	-- run shell commands locally:   !local <command><br/>
	-- view help:                    !help
