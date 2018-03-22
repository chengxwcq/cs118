#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "packet.h"
#include "time.h"

#define BUFSIZE 1024

void updatePackets(struct packet[], int, int);
void updateAcks(int [], int, int);
void updateTimers(time_t [], int, int);

int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen = sizeof(cli_addr);
    int recvlen; /* the length of the received data */
    unsigned char buf[BUFSIZE]; /* used to receive the message from the client */
    char sendbuf[PAYLOAD_SIZE];

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }
    memset((char *)&serv_addr, '\0', sizeof(serv_addr));

    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    }

    for (;;) {
        recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&cli_addr, &addrlen);
        printf("%s\n", buf);
        if (recvlen > 0) {
            buf[recvlen] = '\0';
            FILE *fp = fopen((char*)buf, "rb");
            if (fp == NULL) {
                char *response = (char*) "File not exists";
                struct packet pac;
                pac.type = -1;
                strcpy(pac.data, response);
                sendto(sockfd, (char *)&pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);

                memset(buf, '\0', BUFSIZE);
            } else {
                int size_of_packets = WINDOW_SIZE / MPL;
                struct packet packets[size_of_packets]; /* used to buffer the sent packets*/
                int acked[size_of_packets]; /* if acked, then 1; else 0*/
                time_t timer[size_of_packets]; /* used to caculate the time */
                // first put all prepared packets into array packets
                int i; /* to deal with the situation we only need fewer packets to send all data rather than size_of_packets */
                int next_no = INIT_NO;
                int valid_num_of_packets; /* used to record the valid number of packets in the window */

                for (i = 0; i < size_of_packets && !feof(fp); i++) {
                    int numread = fread(sendbuf, sizeof(char), PAYLOAD_SIZE - 1, fp);
                    struct packet pac;

                    sendbuf[numread] = '\0';
                    strcpy(pac.data, sendbuf);
                    pac.type = 1;
                    pac.number = next_no;
                    next_no = next_no + MPL;

                    if (next_no > MAX_SEQ_NO)
                        next_no = next_no % MAX_SEQ_NO;
                    packets[i] = pac;
                }

                // send all the packets
                for (int j = 0; j < i; j++) {
                    printf("sending packet %d %d\n", packets[j].number, WINDOW_SIZE);
                    sendto(sockfd, (char *)&packets[j], sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                    timer[j] = clock();
                }
                valid_num_of_packets = i;

                // wait for the ACK sent by the receiver
                struct packet rec_pac;

    // change to the unblock mode
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec= 10;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);


                // this condition has some problems
                for (; valid_num_of_packets != 0;) { /* loop until the number of valid packets in the window is 0 */
                    // check if the timer has timer out
                    for (int j = 0; j < i; j++) {
                        time_t current_time = clock();
                        double d = (double)(current_time - timer[j]) / CLOCKS_PER_SEC * 1000;
               //printf("time out is %d\n", TIME_OUT);
                        if (d > TIME_OUT && acked[j] != 1) {
                            sendto(sockfd, (char *)&packets[j], sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                            timer[j] = clock();
                            printf("sending packet %d %d Retransmission\n", packets[j].number, WINDOW_SIZE);
                        }
                    }

                    if (recvfrom(sockfd, (char *)&rec_pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, &addrlen) != -1) {
                        // check the ACK
                        int ack_no = rec_pac.number;
                        printf("receiving packet %d\n", ack_no);

                        if (ack_no == packets[0].number) { /* meaning the window can move forward*/
                            // first check whether the next packet has been acked
                            int p = 1;
                            for (; p < i; p++) {
                                if (acked[p] != 1) 
                                    break;
                            }
                            // update the current window
                            updatePackets(packets, p, size_of_packets);
                            updateAcks(acked, p, size_of_packets);
                            updateTimers(timer, p, size_of_packets);

                            // put other packets into the window and send them
                            for (i = i - p; i < size_of_packets && !feof(fp); i++) {
                                int numread = fread(sendbuf, sizeof(char), PAYLOAD_SIZE - 1, fp);
                                struct packet pac;
                                sendbuf[numread] = '\0';
                                strcpy(pac.data, sendbuf);
                                pac.type = 1;
                                pac.number = next_no;
                                next_no = next_no + MPL;
                                packets[i] = pac;

                                if (next_no > MAX_SEQ_NO)
                                next_no = next_no % MAX_SEQ_NO;

                                // send the packet
                                printf("sending packet %d %d\n", packets[i].number, WINDOW_SIZE);
                                sendto(sockfd, (char *)&packets[i], sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                            }
                            valid_num_of_packets = i;
                        } else {
                            // find the index of the received ack
                            int index_ack = 0;
                            for (; index_ack < size_of_packets; index_ack++) {
                                if (packets[index_ack].number == ack_no)
                                    break;
                            }
                            acked[index_ack] = 1;
                        }
                    }
                }
                printf("%s\n", "transmission succeed");

                int received_ack = 0;
                // need to send the FIN packet when all the data is successfully ACKed
                for (;;) {
                    struct packet pac;
                    strcpy(pac.data, "");
                    pac.number = -1;
                    pac.fin = 1;
                    sendto(sockfd, (char *)&pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                    printf("%s\n", "send fin");

                    time_t fin_start = clock();

                    for (;;) {
                        if (recvfrom(sockfd, (char *)&rec_pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, &addrlen) != -1) {
                            if (rec_pac.fin == 1 && rec_pac.number == -1) {
                                printf("%s\n", "Receiving ACK of FIN");
                                received_ack = 1;
                                break;
                            }
                        }
                        time_t current_time = clock();
                        double d = (double)(current_time - fin_start)/ CLOCKS_PER_SEC * 1000;
                        if (d > TIME_OUT) 
                            break;
                    }
                    if (received_ack == 1)
                        break;
                }
                break;
            }
        }
    }
    close(sockfd);
    return 0;
}


void updatePackets(struct packet packets[], int index, int length) {
    int i = 0;
    for (; index < length; i++, index++) {
        packets[i] = packets[index];
    }
    for (; i < length; i++) {
        struct packet p;
        packets[i] = p;
    }
}

void updateAcks(int acks[], int index, int length) {
    int i = 0;
    for (; index < length; i++, index++) {
        acks[i] = acks[index];
    }
    for (; i < length; i++) {
        acks[i] = 0;
    }
}

void updateTimers(time_t timers[], int index, int length) {
    int i = 0;
    for (; index < length; i++, index++) {
        timers[i] = timers[index];
    }
    for (; i < length; i++) {
        timers[i] = clock();
    }
}
