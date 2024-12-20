# POP3-Protocol-Server-and-Client
An implementation of the POP3 email protocol implemented as a server client pair. This was done for CS4461: Computer Networking.

How to compile: Once the repo has been cloned, verify it contains a file named 'makefile'. Assuming it's there (which it should be), just type 'make'. This will produce two executables as well as some object (*.o) files.

How to Test: Open two terminals on your machine. In each terminal, navigate to the directory containing the designated executables. In the first terminal, type './server <port>' where '<port> is some port. In the second terminal, type './client localhost <port>' where '<port>' is the same port number you used for the server.
You should now be able to type the POP3 commands specified in the project specifications into the client terminal.
A few examples:
	- 'STAT'
	- 'LIST'
	- 'RETR 1'
	- 'TOP 1 1'
	- 'DELE 1'
