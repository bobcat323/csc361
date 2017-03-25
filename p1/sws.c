/*
 * Jason Louie (V00804645)
 * sws.c - A Simple Web Server
 * A server-end program that creates a UDP connection for receiving requests and 
 * sending messages in response from a client.
 * Press 'q' at any moment to terminate the server gracefully.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>		//IPPROTO_UDP
#include <sys/select.h>		
#include <sys/time.h>
#include <unistd.h>		//STDIN_FILENO
#include <sys/stat.h>		//S_ISDIR
#include <time.h>
#include <arpa/inet.h>		//inet_nota

/*
 * A global variable to keep a record of values
 * mainInfo[i]: 0 = directory, 1 = URi, 2 = contents
 */
char *mainInfo[3];

/*
 * Check if the directory exists or not. Upon success, the directory is stored in the
 * first index of mainInfo.
 */
void checkArgs(int argc, char* argv[], char *mainInfo[]){
	if(argc != 3){
            perror("Incorrect arguments, requires <port number> <directory>, now exiting\n");
            exit(0);
        }
	if(argv[2] == "../../"){
		printf("Denied access to root directory, now exiting.\n");
		exit(0);
	}

	if(!isDirectory(argv[2])){
		printf("File/Directory does not exist, now exiting.\n");
		exit(0);	
	}
	mainInfo[0] = argv[2];
}

/*
 * Supplementary function for checkArgs(); return 1 upon success(a directory) 
 * and 0 if a file.
 */
int isDirectory(char *arg){
	int var;
        struct stat statbuf;

        stat(arg, &statbuf);
        if(S_ISDIR(statbuf.st_mode)){	//directory
            var = 1;	
        }else{	//file
            var = 0;
        }

	return var;
}

/*
 * Supplementary function for checkArgs() that takes a pointer to a string and 
 * converts it to uppercase letters.
 */
char *toUpper(char *token){
	int val, i;
	i = 0;
	while(token[i]){
		token[i] = toupper(token[i]);
		i++;
	}

	return token;
}

/*
 * Opens the file and stores the contents of the file. Returns 1 upon success and 
 * 0 for failure
 */
int getFile(char *mainInfo[]){
	size_t bytesRead;
	int check;
	char *buffer;
	long int fileSize;
	char *filePath = strdup(mainInfo[0]);
	strcat(filePath, mainInfo[1]);
	FILE *input = fopen(filePath, "r");
	
	if(input == NULL){	//does not exist
		check = 2;
		mainInfo[0] = " ";
		mainInfo[1] = " ";
		mainInfo[2] = " ";
	}else{	
		fseek(input, 0L, SEEK_END);
		fileSize = ftell(input);
		buffer = realloc(NULL, fileSize);
		fseek(input, 0L, SEEK_SET);
		bytesRead = fread(buffer, sizeof(char), fileSize, input);
		fclose(input);
		mainInfo[2] = strdup(buffer);
		check = 1;
		free(buffer);
	}
	return check;
}

/*
 * Returns a value corresponding to an index to a char array used in printLog()
 * -1 = (move on), 0 = OK, 1 = Bad Request, 2 = Not Found
*/
int checkBuffer(char buff[], char *mainInfo[], char *argv[]){
	int check1, check2, check3;
	char *token, *buffer, *dummy;
	char *words[3];
	dummy = strdup(buff);
 	buffer = buff;	

	//tokenize each word in buff into an index in words[]
	words[0] = strtok(buffer, " ");
	words[1] = strtok(NULL, " \r");
	words[2] = strtok(NULL, " \r");

	if(words[0] == NULL){//prevents seg fault upon no input
		return -1;
	}

	//check for blank line at the end of the request message (buff)
	if(dummy[strlen(dummy) - 4] != '\r' && dummy[strlen(dummy) - 3] != '\n' && dummy[strlen(dummy) - 2] != '\r' && dummy[strlen(dummy) - 1] != '\n' || words[1][0] != '/'){
		mainInfo[2] = " ";
		return 1;
	}

	if(strcmp(toUpper(words[0]), "GET") == 0){ //match first word
		check1 = 1;
		if(strcmp(words[1], "/") == 0){	//match second word given no path
			mainInfo[1] = "/index.html";
			if(strcmp(toUpper(words[2]), "HTTP/1.0") == 0){	//match third word given no path			
				check2 = check3 = 1;
			}
		}else if(strcmp(words[1], "/../") == 0){ //match attemp to reach root directory
			return 2;
		}else{
			check2 = 1;
			mainInfo[1] = strdup(words[1]);
			if(strcmp(toUpper(words[2]), "HTTP/1.0") == 0){ //match third word
				check3 = 1;
			}			
		}
	}

	int check = getFile(mainInfo);
	if(check == 1 && check1 == 1 && check2 == 1 && check3 == 1){
		return 0;
	}else if(check == 2 && check1 == 1 && check2 == 1 && check3 == 1 && strcmp(mainInfo[1], "/") != 0){
		return 2;
	}
	return 1;
}

/*
 * Send the status and content of the file to the client in a proper format.
 */
void sendResponse(int sock, struct sockaddr_in sockAdd, int valid, char *mainInfo[]){
	char status[100];
	int bytesSent, bufferSize;
	bufferSize = 1024;
	if(valid == 0){
		strcpy(status, "HTTP/1.0 200 OK\n\n");
	}else if(valid == 1){
		strcpy(status, "HTTP/1.0 400 Bad Request\n\n");
	}else{	//valid = 2
		strcpy(status, "HTTP/1.0 404 Not Found\n\n");
	}

	//send status code with reason phrase
	bytesSent = sendto(sock, status, strlen(status), 0, (struct sockaddr*)&sockAdd, sizeof sockAdd);
	if(bytesSent < 0){
                perror("sendto call failed");
                close(sock);
                exit(1);
        }
	if(valid == 0){	//status code is 200, send contents of file
		if(strlen(mainInfo[2]) <= bufferSize){
			bytesSent = sendto(sock, mainInfo[2], strlen(mainInfo[2]), 0, (struct sockaddr*)&sockAdd, sizeof sockAdd);
			if(bytesSent < 0){
	                perror("sendto call failed");
        	        close(sock);
                	exit(1);
			}
		}else{
			int index = 0;
			while(index < strlen(mainInfo[2])){
				sendto(sock, &mainInfo[2][index], strlen(mainInfo[2]), 0, (struct sockaddr*)&sockAdd, sizeof sockAdd);
				if(bytesSent < 0){
			                perror("sendto call failed");
			                close(sock);
                			exit(1);
		        	}
				index += bufferSize;
			}
		}
	}
}

/*
 * Prints the log to the server indicating success or failure in a proper format.
 */
void printLog(char *argv[], int valid, char buff[], struct sockaddr_in sockAdd, char *mainInfo[]){
	time_t t;
	t = time(NULL);
	struct tm time = *localtime(&t);
	char *buffer = buff;
	strtok(buffer, "\r");
	char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char *response[3] = {"HTTP/1.0 200 OK", "HTTP/1.0 400 Bad Request", "HTTP/1.0 404 Not Found"};
	int port = ntohs(sockAdd.sin_port);
	char* clientIP = inet_ntoa(sockAdd.sin_addr);

	printf("%s %d %d:%d:%d %s:%d %s;%s;", months[time.tm_mon], time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, clientIP, port, buffer, response[valid]);
	if(valid == 0){
		printf("%s%s",mainInfo[0], mainInfo[1]);
	}	
	printf("\n\n");
}

int main(int argc, char* argv[]){
	checkArgs(argc, argv, mainInfo);

	ssize_t recSize;	//recvfrom error check
	socklen_t fromLen;	//length of sender's address
	struct sockaddr_in sockAdd;	//sender's address
	sockAdd.sin_family = AF_INET;	//configure to IPv4
	sockAdd.sin_addr.s_addr = htonl(INADDR_ANY);	//converts unsigned int of all available interfaces from host byte order to network byte order		
	sockAdd.sin_port = htons(atoi(argv[1]));		//converts to network byte order
    	int optVal = 1;
	int selectVal, selectVal2;		//select error check
	fd_set socks, socks2;		//two sets for select
	struct timeval timeout, timeout2;
	
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0){
		perror("socket call failed");
		exit(1);
	}
        
	int sockOpt = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof (optVal));
	if(sockOpt < 0){
            perror("sockopt call failed");
            close(sock);
            exit(1);
	}

	int bindc = bind(sock, (struct sockaddr *)&sockAdd, sizeof(sockAdd));
	if(bindc < 0){
		perror("bind call failed");
		close(sock);
		exit(1);
	}

	printf("sws is running on UDP port %s and serving %s\npress 'q' to quit ...\n\n", argv[1], argv[2]);
	
	while(1){
		char buffer[1024];	
		buffer[0] = 0;		//avoid seg fault
		FD_ZERO(&socks);
		FD_SET(STDIN_FILENO, &socks);
		FD_ZERO(&socks2);
		FD_SET(sock, &socks2);

		timeout.tv_sec = 0;     //move on after idle for X seconds
		timeout.tv_usec = 0;
		timeout2.tv_sec = 0;     //move on after idle for X seconds
		timeout2.tv_usec = 0;

		selectVal = select(STDIN_FILENO + 1, &socks, NULL, NULL, &timeout);
		if(selectVal < 0){
			perror("select call failed");
			close(sock);
			exit(1);
		}else if(FD_ISSET(STDIN_FILENO, &socks)){
			if(getchar() == 'q'){
				printf("Entered 'q', now exiting\n");
				close(sock);
				exit(0);				
			}
		}

		selectVal2 = select(sock + 1, &socks2, NULL, NULL, &timeout2);
		if(selectVal2 < 0){
			perror("select call failed");
			close(sock);
			exit(1);
                }else if(FD_ISSET(sock, &socks2)){
			recSize = recvfrom(sock, (void *)buffer, sizeof buffer, 0, (struct sockaddr*)&sockAdd, &fromLen);	
			if(recSize < 0){
			perror("recvfrom call failed");
				close(sock);
				exit(1);
			}
		}
		char tempBuffer[1024];
		strcpy(tempBuffer, buffer);
		int valid = checkBuffer(buffer, mainInfo, argv);
		if(valid >= 0){	//valid: 0=OK,1=BADREQ,2=NOTFOUND
			sendResponse(sock, sockAdd, valid, mainInfo);
			printLog(argv, valid, tempBuffer, sockAdd, mainInfo);
			memset(buffer, 0, 1024*sizeof(char));	//clear the buffer after each request
		}
	}
	return 0;
}
