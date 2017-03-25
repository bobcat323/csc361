Readme.txt
Jason Louie (V00804645)
CSC 361, Lab Section B04
Package contains the following files: rdps.c, rdpr.c, Makefile, readme.txt

Overview:
Two C programs labelled as a client(rdps) and server(rdpr) simulate a reliable datagram protocol. The program is broken down into three stages: data management, flow control and error control. The process begins by rdps creating a socket, bind it to a port, then send a SYN packet to establish the connection when rdpr is in a listening state. Once the connection has been established, the sender may begin sending data packets containing the contents of a file. Each sent packet will print a log indicating statistics like when it was sent and its sequence number and payload. Once the sender has reached the end of the file it sends a FIN packet to confirm connection release.

When the connection is finished, each program will print a final log to summarize how many bytes are sent/received, number of data packets sent/received etc. This information is stored in another struct and is incremented at each transmission.

Procedure:
To start the RDP, open two terminals and run the command "make" to compile all the files. Then on each terminal run "make rec" and "make send" to run the server and client respectively. This will run the program on port 8081 by default. 

INITIAL Design Document:
1. How do you design and implement your RDP header and header fields? Do you use any additional header fields?

I use a struct that holds the attributes of the RDP header. Each attribute’s type is either a char pointer or integer.  The attributes are then concatenated altogether to a char string using sprintf() with a forward slash as a delimiter for clarity and is finally sent from the sender to receiver.  There are no additional header fields for the case of simplicity.

2. How do you design and implement the connection management using SYN, FIN and RST packets? How to choose the initial sequence number?

Connection management is implemented by creating a socket and binding the address and port number to it.  Afterwards, the sender will send a SYN packet containing the initial sequence number(ISN) and will wait to receive an ACK (ISN + 1) from the receiver.  The ISN is randomly chosen by using the rand() function call. Upon sending, the sender will begin a timer of 5 seconds; the sender will retransmit if it does not receive an acknowledgement before the timer expires.  After three retransmissions and no acknowledgements, the connection is assumed to be unsuccessful and the program will terminate.  The connection release will behave similarly, this time the sender will send a FIN and wait for ACK from the receiver.  The same retransmission process undergoes.  Finally the sender will terminate once it receives an ACK.

3. How do you design and implement the flow control using window size? How to choose the initial window size and adjust the size? How to read and write the file and how to manage the buffer at the sender and receiver side, respectively?

During the connection establishment, the receiver will send a header (the acknowledgement) along with a window size (initially 2048 since it is empty) to follow David Clark’s algorithm.  After each subsequent data transmission, the sender will send data in chunks according the the window size.  The acknowledgement from the receiver will then send the acknowledgement along with its new window size.  This method will then follow the sliding window technique.

Reading and writing to files will be done by fopen(), fread() fwrite() and closing the file with fclose().  


4. How do you design and implement the error detection, notification and recovery? How to use timer? How many timers do you use? How to respond to the events at the sender and receiver side, respectively? How to ensure reliable data transfer?

Error detection will be designed by using timers.  To implement timers, I will use the clock() function to get the current timestamp and call it again when comparing the duration (clock() - previous clock()) to see if it is larger than the set timer.  I plan to use a timer for each data transmission to keep track if that data packet needs to be retransmitted.  If the sender receives an acknowledgement before the timer expiration, then the timer will stop and start once another packet is sent.  On the receiver side, I will use another timer to implement a cumulative acknowledgement.  To ensure reliability in data transfer, I will use the acknowledgement number to compare with the received sequence number.

5. Any additional design and implementation considerations you want to get feedback from your lab instructor?

Is the sequence number important when sending an acknowledgement from the receiver to sender? If I recall, it isn’t during the connection management stage but I’m unsure during data transfer.

Also in the p2a specifications under 3.1 number 4, it states the max packet size can be 1024 Bytes. However in the header table below it says the windows size is 10240 Bytes. Is this a typo? Otherwise is there an explanation why the window size is larger than our restricted max packet size.

FINAL Design Document:
1. 1. How do you design and implement your RDP header and header fields? Do you use any additional header fields?

Each packet has a structure of the following format: magic, type, sequence number, acknowledgement number, payload, window, blank line (and contents of the file if it is a data packet). I use a struct for readability purposes; but for sending, I construct a string and send a pointer to the string. In the packet, I use "|" as delimiters when parsing information from the packet. There are no additional attributes for the case of simplicity. 

2. How do you design and implement the connection management using SYN, FIN and RST packets? How to choose the initial sequence number?

Connection management remains the same implementation from the initial design document. After three unsuccessful attempts of establishment, the program will gracefully exit since the client wasn't able to connect with its server. 

3. How do you design and implement the flow control using window size? How to choose the initial window size and adjust the size? How to read and write the file and how to manage the buffer at the sender and receiver side, respectively?

Flow control is the same initial design document. One implementation I added is opening the file (received.dat) with "w+" permissions just to overwrite a previously used file. Then called another fopen function with append permissions when appending contents of data packets. 

4. How do you design and implement the error detection, notification and recovery? How to use timer? How many timers do you use? How to respond to the events at the sender and receiver side, respectively? How to ensure reliable data transfer?

Each packet will attempt to retransmit three times if the first transmission was unsuccessful(timeout); Upon the second SYN packet, the receiver will send a reset packet to re-establish the connection. Likewise for the sender attempting three data packets. While receiving retransmitted packets, the receiver will check its previously received packets to check if it has already received the packet or not.

5. Any additional design and implementation considerations you want to get feedback from your lab instructor?

Under dropped packet circumstances, the program will sometimes enters a state of deadlock where the sender retransmits a data packet but the receiver is not listening at the moment (the same under dropped acknowledgement packets). 

Resources accredit to this program:
https://www.thecodingforums.com/threads/help-how-do-i-send-an-ack-packet-in-udp.360271/ 
http://stackoverflow.com/questions/11450899/casting-pointer-its-address-to-integer-pointer 
https://en.wikipedia.org/wiki/Struct_(C_programming_language)
http://stackoverflow.com/questions/19823797/how-to-get-the-clients-ip-after-receiving-a-udp-packet-with-recvfrom-in-c
http://www.cplusplus.com/reference/ctime/clock/
http://stackoverflow.com/questions/11450899/casting-pointer-its-address-to-integer-pointer
