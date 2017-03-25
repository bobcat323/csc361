/*
 * Jason Louie (V00804645)
 * rdpr.c - RDP receiver/server
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
#include <arpa/inet.h> //inet_ntoa

#define MAX_BUFFER 1024 
#define MAX_ARR_LENGTH 50
/*
 * A struct used for readability when creating packets
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
 * and is printed out in the end: see print_summary()
 */
struct data{
        int bytesRec;
        int bytesRerec;
        int packetsRec;
        int packetsRerec;
        int recSYN;
        int recFIN;
        int recRST;
        int senACK;
        int senRST;
};

/*
 * Returns a newly created packet with the format: magic, type,
 * sequence number acknowledgement number, payload, window, blank.
 * The following numbers correlate to 'type' when it is passed in:
 * 1 = DAT, 2 = ACK, 3 = SYN, 4 = FIN, 5 = RST
 */
char* create_packet(struct header* head, int type, int ackNo, int window, int payload){
        char* headerStr = (char*)malloc(sizeof(char)* MAX_BUFFER);//1024=max buffer length
        (*head).magic = "CSC361";
        switch(type){
                case 1:
                        (*head).type = "DAT";
                        break;
                case 2:
                        (*head).type = "ACK";
                        break;
                case 3:
                        (*head).type = "SYN";
                        break;
                case 4:
                        (*head).type = "FIN";
                        break;
                case 5:
                        (*head).type = "RST";
                        break;
        }
	(*head).payload = payload;
        (*head).seq = 0;
        (*head).ack = ackNo;
        (*head).window = window;	
        (*head).blank = "\n";

        sprintf(headerStr, "%s|%s|%d|%d|%d|%d|%s",(*head).magic, (*head).type, (*head).seq, (*head).ack, (*head).payload, (*head).window, (*head).blank);
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
        //parse buffer to get the attribute
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
 * Retrieves and returns the contents in a packet,
 * attNum is the index corresponding with the format of the packet.
 */
char* get_message(char* original, int attNum){
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
 * Sends an ACK packet and increments attributes for the final summary
 */
void send_ack(int sock, struct sockaddr_in sockAdd, struct header* head, char* packet, struct data* summary){
	int bytesSent;
	char sendBuffer[MAX_BUFFER];
	memset(sendBuffer, 0, sizeof sendBuffer);
	strcat(sendBuffer, packet);

        bytesSent = sendto(sock, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr*)&sockAdd, sizeof sockAdd);
        if(bytesSent < 0){
                perror("sendto call failed");
                close(sock);
                exit(1);
        }
	(*summary).senACK++;
}

/*
 * Prints a log for each transmission in the following
 * format: hours, minutes, seconds, microseconds, 
 * event type, src IP, src port, dst IP, dst port, 
 * packet type, sequence number, acknowledgement number,
 * payload length, window
 */
void print_log(struct tm* time, struct timeval* ms, char type, char* srcIP, char* srcport, char* dstIP, int dstport, char* packet){
	
        printf("%d:%d:%d:%ld ", (*time).tm_hour, (*time).tm_min, (*time).tm_sec, (*ms).tv_usec);
        printf("%c ", type);//event type
        printf("%s:%s ", srcIP, srcport);//source IP and port
        printf("%s:%d ", "192.168.1.1", 8081);//dest IP and port
        //packet type, seqnum or acknum, payload length or window size
	if(type == 's' || type == 'S'){
		printf("%s %d %d\n", get_string_attribute(packet, 1), get_attribute(packet, 3), get_attribute(packet, 5));
	}else{//'r' or 'R'
		printf("%s %d %d\n", get_string_attribute(packet, 1), get_attribute(packet, 2), get_attribute(packet, 4));

	}
}

/*
 * Prints a final summary log at the end of the program to
 * display statistics from the program.
 */
void print_summary(struct data* summary, double duration){
        printf("total data bytes received: %d\n", (*summary).bytesRec);
        printf("unique data bytes received: %d\n", (*summary).bytesRec - (*summary).bytesRerec);
        printf("total data packets received: %d\n", (*summary).packetsRec);
        printf("unique data packets received: %d\n", (*summary).packetsRec - (*summary).packetsRerec);
        printf("SYN packets received: %d\n", (*summary).recSYN);
        printf("FIN packets received: %d\n", (*summary).recFIN);
        printf("RST packets received: %d\n", (*summary).recRST);
        printf("ACK packets sent: %d\n", (*summary).senACK);
        printf("RST packets sent: %d\n", (*summary).senRST);
        printf("total time duration (second): %f\n", duration*-1); //gives negative number so I multiplied by -1
}

/*
 * Searches in the array of packets for a match to find whether
 * or not a packets has already been received or not.
 * Returns the index number upon duplicates, else -1 for no match.
 */
int check_repeat(char* packets[], char* newPacket){
	int i;
	for(i = 0; i < MAX_ARR_LENGTH; i++){

		if(strcmp(packets[i], "NULL") == 0){
			return -1;
		}
		if(strcmp(packets[i], newPacket) == 0){
			return i;
		}
	}
	return -1;
}

/*
 * Initialization function that sets all the attributes to zero.
 */
void initialize_data(struct data* summary){

        (*summary).bytesRec = 0;
        (*summary).bytesRerec = 0;
        (*summary).packetsRec = 0;
        (*summary).packetsRerec = 0;
        (*summary).recSYN = 0;
        (*summary).recFIN = 0;
        (*summary).recRST = 0;
        (*summary).senACK = 0;
        (*summary).senRST = 0;
}

void initialize_array(char* array[]){
	int j;
	for(j = 0; j < MAX_ARR_LENGTH; j++){
                array[j] = "NULL";
        }
}

int main(int argc, char* argv[]){
	
	FILE* fp;
	//create the file
	fp = fopen(argv[3], "w+");
	if(!fp){
                perror("Error trying to write to file, now exiting.\n");
                exit(1);
        }
	fclose(fp);
	//append permissions
	fp = fopen(argv[3], "a");
	if(!fp){
		perror("Error trying to write to file, now exiting.\n");
		exit(1);
	}

        int optVal = 1;
        ssize_t recSize;
	int selectVal;
        struct sockaddr_in sockAdd;
	socklen_t fromLen = sizeof(sockAdd);

        memset(&sockAdd, 0, sizeof sockAdd);
        sockAdd.sin_family = AF_INET;   //configure to IPv4
	sockAdd.sin_addr.s_addr = inet_addr(argv[1]); //argv[1] = 10.10
        sockAdd.sin_port = htons(atoi(argv[2])); //argv[2] = 8080

        //socket
        int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(sock < 0){
                perror("socket call failed");
                exit(1);
	}

        //setsockopt
        int sockOpt = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof (optVal));
        if(sockOpt < 0){
                perror("sockopt call failed");
                close(sock);
                exit(1);
        }

        //bind
        int bindc = bind(sock, (struct sockaddr *)&sockAdd, sizeof(sockAdd));
        if(bindc < 0){
                perror("bind call failed");
                close(sock);
		exit(1);
        }

	fd_set socks;
	char* dstIP;
	int dstPort;
	//dstIP = inet_ntoa(sockAdd.sin_addr); gives 10.10 IP, need 192
	//dstPort = htons(sockAdd.sin_port);

	struct timeval timeout, time2;
	clock_t begin, end;
	time_t t;
	t = time(NULL);
	struct tm time = *localtime(&t);

	int window = 2048;
	int payload = 0;
	int first = 2;
	int retransCounter = 0;
	struct data summary;
        initialize_data(&summary);
	begin = clock();

	char* packets[MAX_ARR_LENGTH];
	initialize_array(packets);

	int index = 0;
	int check;
        while(1){
		char buffer[1025];
		char* packet;
		struct header head;
		int ACK;
		memset(buffer, 0, sizeof buffer);

		FD_ZERO(&socks);
                FD_SET(sock, &socks);
                timeout.tv_sec = 2;//6 sec, 4 sec
                timeout.tv_usec = 0;
		selectVal = select(sock + 1, &socks, NULL, NULL, &timeout);
                if(selectVal == -1){
                        perror("select call failed\n");
                        return -1;
                }else if(selectVal == 0 && first == 2){
			retransCounter++;
			if(retransCounter == 3){
				printf("Failed connection establishment three times, now exiting.\n");
				fclose(fp);
				close(sock);
				exit(0);
			}
			continue;
		}else if(selectVal == 0 && first == 1){//timeout during data transfer

			ACK = get_attribute(packets[check], 2) + get_attribute(packets[check], 4);
			payload = get_attribute(packets[check], 4);
			window = window - get_attribute(packets[check], 4);
			if(window <= 0){
				window = 0;
			}
			packet = create_packet(&head, 2, ACK, window, payload);
			send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 'S', argv[1], argv[2], dstIP, dstPort, packets[index]);
			//update window according to payload
			window = window + get_attribute(packets[check], 4);
		}//end else if

memset(buffer, 0, sizeof buffer);
		recSize = recvfrom(sock, (void *)buffer, sizeof buffer, 0, (struct sockaddr*)&sockAdd, &fromLen);	
		if(recSize < 0){
			perror("recvfrom call failed");
			close(sock);
			exit(1);
		}
		time = *localtime(&t);
		gettimeofday(&time2, NULL);
		print_log(&time, &time2, 'r', argv[1], argv[2], dstIP, dstPort, buffer);
		if(strcmp(get_string_attribute(buffer, 1), "DAT") == 0){//data packet
		summary.bytesRec += recSize;
		}

		check = check_repeat(packets, buffer);
		//already received this packet case
		if(check > -1){
			ACK = get_attribute(packets[check], 2) + get_attribute(packets[check], 4);
			packet = create_packet(&head, 2, ACK, window, 0);
			send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 'S', argv[1], argv[2], dstIP, dstPort, packet);
			summary.bytesRerec += recSize;
			if(strcmp(get_string_attribute(packets[check], 1), "SYN") == 0){
				summary.recSYN++;
			}else if(strcmp(get_string_attribute(packets[check], 1), "FIN") == 0){
				summary.recFIN++;
			}else{
				summary.packetsRerec++;
				summary.packetsRec++;

			}
			continue;
		}

		char* newBuff = strdup(buffer);
		packets[index++] = newBuff;

		char* TYPE = get_string_attribute(buffer, 1);
		if(strcmp(TYPE, "SYN") == 0){//SYN packet
			summary.recSYN++;
			if(retransCounter > 0){//received SYN for 2nd time, send reset packet
				ACK = get_attribute(buffer, 2) + 1;
				packet = create_packet(&head, 5, ACK, window, 0);
				send_ack(sock, sockAdd, &head, packet, &summary);
				time = *localtime(&t);
				gettimeofday(&time2, NULL);
				print_log(&time, &time2, 'S', argv[1], argv[2], dstIP, dstPort, packet);
				retransCounter = 0;
				summary.senRST++;
			}
			ACK = get_attribute(buffer, 2) + 1;
			packet = create_packet(&head, 2, ACK, window, payload);
			send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 's', argv[1], argv[2], dstIP, dstPort, packet);
			printf("\nConnection established.\n\n");
			first = 1;
		}else if(strcmp(TYPE, "FIN") == 0){//fin packet
                        ACK = get_attribute(buffer, 2) + 1;
                        packet = create_packet(&head, 2, ACK, window, payload);
                        send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 's', argv[1], argv[2], dstIP, dstPort, packet);
			summary.recFIN++;
		        printf("\nConnection released.\n\n");
			break;
		}else if(strcmp(TYPE, "RST") == 0){//reset packet
			ACK = get_attribute(buffer, 2) + 1;
                        packet = create_packet(&head, 2, ACK, window, 0);
			send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
                        print_log(&time, &time2, 's', argv[1], argv[2], dstIP, dstPort, packet);
			summary.recRST++;
		}else if(strcmp(TYPE, "DAT") == 0){//data packet
			summary.packetsRec++;

			//write into received.dat the message
			char* s = get_message(buffer, 6);
			if(first == 1){
				fseek(fp, 0, SEEK_SET);
				first = 0;
			}

			fprintf(fp, "%s", s);
			int check = fseek(fp, 0, SEEK_CUR);
			if(check != 0){
				printf("error on fseek\n");
				exit(1);
			}
	
			ACK = get_attribute(buffer, 2) + get_attribute(buffer, 4);
			payload = get_attribute(buffer, 4);
			window = window - get_attribute(buffer, 4);
			if(window <= 0){
				window = 0;
			}
			packet = create_packet(&head, 2, ACK, window, payload);
			send_ack(sock, sockAdd, &head, packet, &summary);
			time = *localtime(&t);
			gettimeofday(&time2, NULL);
			print_log(&time, &time2, 's', argv[1], argv[2], dstIP, dstPort, packet);
			
			//update window according to payload
			window = window + get_attribute(buffer, 4);
		}
	}//end of while

	end = clock();
	double duration = (double)(begin - end) / CLOCKS_PER_SEC;
	print_summary(&summary, duration);

        close(sock);
	fclose(fp);
        return 0;
}
