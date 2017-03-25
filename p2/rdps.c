/*
 * Jason Louie (V00804645)
 * rdps.c - RDP sender/client
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>	
#include <time.h>
#include <errno.h>
#include <netinet/in.h>	//IPPROTO_UDP
#include <sys/time.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define MAX_ARR_LENGTH 50
void print_log(struct tm* time, struct timeval* ms, char type, char* srcIP, char* srcport, char* dstIP, char* dstport, char* packet);

/*
 * A struct used for readability when creating packets.
 */
struct header{
	char* magic;
	char* type;
	int seq;
	int ack;
	int payload;
	int window;
	char* blank;
};

/*
 * A struct where its values are incremented for each transmission
 * and is printed out in the end: see print_summary().
 */
struct data{
	int bytesSent;
	int bytesResent;
	int packetsSent;
	int packetsResent;
	int senSYN;
	int senFIN;
	int senRST;
	int recACK;
	int recRST;
};

/*
 * Returns a newly created packet with the format: magic, type,
 * sequence number acknowledgement number, payload, window, blank.
 * The following numbers correlate to 'type' when it is passed in:
 * 1 = DAT, 2 = ACK, 3 = SYN, 4 = FIN, 5 = RST
 */
char* create_packet(struct header* head, FILE* fp, int type, int seqNo, int* window, int* EOFflag){
	char* headerStr = (char*)malloc(sizeof(char)* MAX_BUFFER);
	(*head).magic = "CSC361";
	switch(type){
		case 1:
			(*head).type = "DAT";
			break;
                case 2:
                        (*head).type = "ACK";
			(*head).payload = 0;
                        break;
                case 3:
                        (*head).type = "SYN";
			(*head).payload = 0;
                        break;
                case 4:
                        (*head).type = "FIN";
			(*head).payload = 0;
                        break;
                case 5:
                        (*head).type = "RST";
                        (*head).payload = 0;
                        break;
	}
	
	(*head).seq = seqNo;
	(*head).ack = 0;	//does not matter when sending
	(*head).window = *window;
	(*head).blank = "\n";
	sprintf(headerStr, "%s|%s|%d|%d|%d|%d|",(*head).magic, (*head).type, (*head).seq, (*head).ack, (*head).payload, (*head).window);

	//if data packet, strcat header with contents of payload length
	if(type == 1){
		char* contents = (char*)malloc(sizeof(char) * MAX_BUFFER);
		int character;	
		char* string;
		int i;

		for(i = 0; i < MAX_BUFFER - 28; i++){//28=approx max size of header without contents
			character = fgetc(fp);
			if(character == EOF){
				*EOFflag = 1;
				break;
			}

			char* string = (char*)&character;//string points to address of character holding the value
			strcat(contents, string);
		}
	(*head).payload = strlen(contents);
	sprintf(headerStr, "%s|%s|%d|%d|%d|%d|%s|",(*head).magic, (*head).type, (*head).seq, (*head).ack, (*head).payload, (*head).window, contents);
	}

	return headerStr;
}

/*
 * Retrieves and returns the integer values in a packet,
 * attNum is the index corresponding with the format of the packet.
 */
int get_attribute(char* original, int attNum){
	char* buffer;
	buffer = strdup(original);
        int attribute, i;
        //parse buffer to get the ack
        char* token;
        token = strtok(buffer, "|");

        for(i = 0; i < attNum; i++){
                token = strtok(NULL, "|\n");
        }

	attribute = atoi(token);
        return attribute;
}

/*
 * Retrieves and returns the string in a packet,
 * attNum is the index corresponding with the format of the packet.
 */
char* get_string_attribute(char* original, int attNum){
	char* buffer, *token;
	int i;
        buffer = strdup(original);
        token = strtok(buffer, "|");

        for(i = 0; i < attNum; i++){
                token = strtok(NULL, "|");
        }

	return token;
}

/*
 * Establishes/Releases connection, attempts three times for
 * error control. Returns 1 upon success and
 * -1 upon failure after three attempts.
 */
int c_management(int sock, struct sockaddr_in sockAddsend, struct timeval timeout, fd_set socks, char* synPacket, int* window, time_t t, struct tm time, struct timeval time2, char* argv[], struct data* summary){
	int bytesSent, ISN, ACK, i, selectVal, resetFlag;
	ssize_t recSize;
	resetFlag = 0;
	char sendBuffer[1024], recBuffer[1024];
	memset(sendBuffer, 0, sizeof sendBuffer);//clears contents before
	strcat(sendBuffer, synPacket);

	for(i = 0; i < 3; i++){
		if(resetFlag == 1){
			resetFlag = 0;
		}
		FD_ZERO(&socks);
		FD_SET(sock, &socks);
		timeout.tv_sec = 4;//4 sec
		timeout.tv_usec = 0;

		//send ISN
		bytesSent = sendto(sock, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr*)&sockAddsend, sizeof sockAddsend);
		if(bytesSent < 0){
			perror("sendto call failed");
			close(sock);
			exit(1);
		}
		
		selectVal = select(sock + 1, &socks, NULL, NULL, &timeout);
		if(selectVal == -1){
			perror("select call failed\n");
			return -1;
		}

		//timer timeout
		if(selectVal == 0){
			continue;
		}
		time = *localtime(&t);
		gettimeofday(&time2, NULL);
		if(i == 0){
		print_log(&time, &time2, 's', argv[1], argv[2], argv[3], argv[4], sendBuffer);
		}else{
			print_log(&time, &time2, 'S', argv[1], argv[2], argv[3], argv[4], sendBuffer);

		}

		if(strcmp(get_string_attribute(sendBuffer, 1), "FIN") == 0){
			(*summary).senFIN++;	
		}else{
			(*summary).senSYN++;
		}
		//receive ACK and confirm ISN + 1
		recSize = recvfrom(sock, (void*)recBuffer, sizeof recBuffer, 0, NULL, NULL);
		if(recSize < 0){
			perror("recvfrom call failed");
			close(sock);
			exit(1);
		}

		time = *localtime(&t);        
		gettimeofday(&time2, NULL);
		print_log(&time, &time2, 'r', argv[1], argv[2], argv[3], argv[4], recBuffer);

		//parse recBuffer to get ACK and ISN
		ACK = get_attribute(recBuffer, 3);
		ISN = get_attribute(synPacket, 2);
		*window = get_attribute(recBuffer, 5);

		//if ack = isn+1: return 1 else continue
		if(ACK == ISN + 1){
			if(strcmp(get_string_attribute(recBuffer, 1), "RST") == 0){
//printf("recv'd reset in conn managemnet\n");
				(*summary).recRST++;
				i = 0;
				resetFlag = 1;
				continue;	
			}else{
				(*summary).recACK++;
			}			
			return 1;
		}else{
			continue;
		}
	}
	return -1;
}

/*
 * Sends a packet and increments attributes for the final summary
 */
int send_packet(int sock, struct sockaddr_in sockAdd, char* packet, int* window, struct data* summary){
	//sendto call and error check
	int bytesSent = sendto(sock, packet, strlen(packet), 0, (struct sockaddr*)&sockAdd, sizeof sockAdd);
	if(bytesSent < 0){
		perror("sendto call failed");
		close(sock);
		exit(1);
	}

	window = window - get_attribute(packet, 5);
	if(window <= 0){
		window = 0;
	}

	return bytesSent;
}

/*
 * Prints a log for each transmission.
 */
void print_log(struct tm* time, struct timeval* ms, char type, char* srcIP, char* srcport, char* dstIP, char* dstport, char* packet){

	printf("%d:%d:%d:%ld ", (*time).tm_hour, (*time).tm_min, (*time).tm_sec, (*ms).tv_usec);
	printf("%c ", type);//event type
	if(type == 's' || type == 'S'){
		printf("%s:%s ", srcIP, srcport);//source IP and port
		printf("%s:%s ", dstIP, dstport);//dest IP and port
		printf("%s %d %d\n", get_string_attribute(packet, 1), get_attribute(packet, 2),  get_attribute(packet, 4));
	}else{//'r' or 'R'
		printf("%s:%s ", dstIP, dstport);//dest IP and port
		printf("%s:%s ", srcIP, srcport);//source IP and port
		printf("%s %d %d\n", get_string_attribute(packet, 1), get_attribute(packet, 3),  get_attribute(packet, 5));
	}
}

/*
 * Prints a final summary log at the end of the program to
 * display statistics from the program.
 */
void print_summary(struct data* summary, double duration){
	printf("total data bytes sent: %d\n", (*summary).bytesSent);
	printf("unique data bytes sent: %d\n", (*summary).bytesSent - (*summary).bytesResent);
	printf("total data packets sent: %d\n", (*summary).packetsSent);
	printf("unique data packets sent: %d\n", (*summary).packetsSent - (*summary).packetsResent);
	printf("SYN packets sent: %d\n", (*summary).senSYN);
	printf("FIN packets sent: %d\n", (*summary).senFIN);
	printf("RST packets sent: %d\n", (*summary).senRST);
	printf("ACK packets received: %d\n", (*summary).recACK);
	printf("RST packets received: %d\n", (*summary).recRST);
	printf("total time duration (second): %f\n", duration*-1); //gives negative number
}

/*
 * Searches in the array of packets for a sequence number match 
 * to find the index value. Returns -1 upon no match
 */
int get_packet(char* packets[], int seqNo){
	int i;
	for(i = 0; i < MAX_ARR_LENGTH; i++){
		if(get_attribute(packets[i], 2) == seqNo){
			return i;
		}
	}
	return -1;
}

/*
 * Initialize the structure's attributes to NULL, this
 * is used in check_repeat().
 */
void initialize_data(struct data* summary){
	(*summary).bytesSent = 0;
	(*summary).bytesResent = 0;
	(*summary).packetsSent = 0;
	(*summary).packetsResent = 0;
	(*summary).senSYN = 0;
	(*summary).senFIN = 0;
	(*summary).senRST = 0;
	(*summary).recACK = 0;
	(*summary).recRST = 0;
}

/*
 * Initialize the values to NULL at each index
 */
void initialize_array(char* array[]){
        int j;
        for(j = 0; j < MAX_ARR_LENGTH; j++){
                array[j] = "NULL";
        }
}


int main(int argc, char* argv[]){	

	struct sockaddr_in sockAddsend, sockAddrec;
	int selectVal, optVal;
	int EOFflag;
	optVal = 1;
	socklen_t fromLen = sizeof(sockAddsend);

	FILE *fp;
	fp = fopen(argv[5], "r");
	if(!fp){
		printf("File does not exist, now exiting.\n");
		exit(1);	
	}

	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0){
		perror("socket call failed");
		fclose(fp);
		exit(1);
	}	
	
	sockAddsend.sin_family = AF_INET;
	sockAddsend.sin_addr.s_addr = inet_addr(argv[3]); //192.16
        sockAddsend.sin_port = htons(atoi(argv[4])); //8080

        fd_set socks;

	struct timeval timeout, time2;
        time_t t;
        t = time(NULL);
        struct tm time = *localtime(&t);        
	clock_t begin, end;	

	int bytesSent, start, SYN, ACK, counter, seqNo, window;
	start = SYN = ACK = counter = 0;
	window = 2048;
	seqNo = rand() % 10000;
	
	struct data summary;
	initialize_data(&summary);

	begin = clock();
	char* packets[MAX_ARR_LENGTH];
	initialize_array(packets);

	int index = 0;
	while(1){
		if(start == 0){
			struct header head;
			char* synPacket = create_packet(&head, fp, 3, seqNo, &window, &EOFflag);
packets[index++] = synPacket;
			int est = c_management(sock, sockAddsend, timeout, socks, synPacket, &window, t, time, time2, argv, &summary);
        		if(est == 1){
				printf("\nConnection management is successful\n\n");
				seqNo++;	
		        }else{
                		printf("\nConnection management is unsuccessful, now exiting.\n");
	        	        fclose(fp);
        		        close(sock);
		                exit(1);
			}
			start = 1;
		}//end of if

//printf("\n=====sending data section here=====\n");
		
		//check if end of file is reached
		if(EOFflag == 1){
			FD_ZERO(&socks);//clear the set
			break;
                }

		//create DAT packet
		struct header packHeader;
                char* packet = create_packet(&packHeader, fp, 1, seqNo, &window, &EOFflag);
		packets[index++] = packet;
                if(window >= 0){//send if window size is not empty
	                bytesSent = send_packet(sock, sockAddsend, packet, &window, &summary);
			summary.bytesSent += bytesSent;
			summary.packetsSent++;
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 's', argv[1], argv[2], argv[3], argv[4], packet);
		}
		FD_ZERO(&socks);
		FD_SET(sock, &socks);
		timeout.tv_sec = 3;//4 sec, 2 sec
		timeout.tv_usec = 0;

		selectVal = select(sock + 1, &socks, NULL, NULL, &timeout);
               	if(selectVal == -1){
                       	perror("select call failed\n");
	                fclose(fp);
                        close(sock);
               	        exit(1);
	        }

	        //timer timeout
                if(selectVal == 0 && window > strlen(packet)){
                       	counter += 1;
	                if(counter == 3 && window >= 0){//send RESET packet
			printf("Three unsuccessful retransmissions, sending reset packet.\n");
				struct header packHeader;
				seqNo = seqNo + get_attribute(packet, 4);
				char* packet = create_packet(&packHeader, fp, 5, seqNo, &window, &EOFflag);
				packets[index++] = packet;
				bytesSent = send_packet(sock, sockAddsend, packet, &window, &summary);
				summary.senRST++;
				time = *localtime(&t);
				gettimeofday(&time2, NULL);
				print_log(&time, &time2, 'S', argv[1], argv[2], argv[3], argv[4], packet);
                		
				start == 0;
			}else{//resend
				//FIND PACKET WITH seqNo as sequence number
				int z = get_packet(packets, seqNo);
//printf("RESENDING IN TIMER: found match at index:%d\n",z);
				if(window >= 0 && z != -1){
					bytesSent = send_packet(sock, sockAddsend, packets[z], &window, &summary);
					if(strcmp(get_string_attribute(packets[z], 1), "DAT") == 0){
						summary.bytesSent += bytesSent;
						summary.bytesResent += bytesSent;
						summary.packetsSent++;
						summary.packetsResent++;
					}
					time = *localtime(&t);
					gettimeofday(&time2, NULL);
					print_log(&time, &time2, 'S', argv[1], argv[2], argv[3], argv[4], packet);

				}
			}
		}
		
		char buffer[1024];
//printf("waiting in recvfrom\n");
		int recSize = recvfrom(sock, (void *)buffer, sizeof buffer, 0, (struct sockaddr*)&sockAddsend, &fromLen);
		if(recSize < 0){
			perror("recvfrom call failed");
			exit(1);
		}

		if(get_string_attribute(buffer, 1) == "RST"){//receive reset
			start = 0;
			summary.recRST++;
			continue;
		}else{
			summary.recACK++;
		}

		time = *localtime(&t);
		gettimeofday(&time2, NULL);
		print_log(&time, &time2, 'r', argv[1], argv[2], argv[3], argv[4], buffer);
		ACK = get_attribute(buffer, 3);

		if(ACK > seqNo){
			seqNo = ACK;
		}

		window = window + get_attribute(buffer, 5); //window + payload

		if(window > 2048){
			window = 2048;
		}
	
		counter = 0;
	}//end of while

	//update seqNo
	if(ACK > seqNo){
		seqNo = ACK;
	}

	struct header head2;
	char* finPacket = create_packet(&head2, fp, 4, seqNo, &window, &EOFflag);
	packets[index++] = finPacket;
	int rel = c_management(sock, sockAddsend, timeout, socks, finPacket, &window, t, time, time2, argv, &summary);	
	if(rel == 1){
		printf("\nConnection released.\n\n");
	}else{
		printf("\nConnection release unsuccessful.\n");
	}
	end = clock();
	double duration = (double)(begin - end) / CLOCKS_PER_SEC;
	print_summary(&summary, duration);
	close(sock);
	fclose(fp);
	return 0;
}


