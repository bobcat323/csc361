Readme.txt
Jason Louie (V00804645)
CSC 361, Lab Section B04
Package contains three other files: sws.c, Makefile, Testfile.sh.

Overview:
sws.c is a program to simulate a simple web server that receives requests from clients (in our case, terminal when you echo a request with use of nc). It involves creating a socket, binding it to a port, then listens for a request message in its proper format. Upon successful messages received, the server responds with a message log to itself as well as the client. Additionally, the client will receive the content contained in the request. Otherwise, both the server and client log will print the log indicating failure (error 400 or 404). 

Procedure:
To start the simple web server, enter the command "make"; make will compile and run the server with preset commands set to run on port 8082 and directory sws/www. If the user wants to run the server with different parameters, use the command "make v2 PORT=<port number> DIR=<directory>" where <port number> and <directory> is the port number and directory respectively. At any point in time, the user may enter 'q' (case sensitive) to gracefully close the server. On the client side, the user may run the bash file Testfile.sh to send a list of requests to the DEFAULT port number. If running on a different port, the client must individually send requests on the specified port number.

Resources accredited to this program: 
http://developerweb.net/viewtopic.php?id=2933
http://stackoverflow.com/questions/16510831/how-to-use-select-to-read-input-from-stdin
http;//pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
CSC 361 Lab 2: udp_server.c	by Seyed Dawood Sajjadi Torshizi
CSC 361 Lab 2: udp_client.c	by Seyed Dawood Sajjadi Torshizi
CSC 361 Lab 5: slides	by Seyed Dawood Sajjadi Torshizi
