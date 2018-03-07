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

#define BUFSIZE 1024

void updataPackets(struct packet[], int, int);
void updataAcks(int [], int, int);

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen = sizeof(cli_addr);
    int recvlen; /* the length of the received data */
    unsigned char buf[BUFSIZE]; /* used to receive the message from the client */
    char sendbuf[PAYLOAD_SIZE];

    int numread; /* used to indicated the number of bytes read from the file*/

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
                // first put all prepared packets into array packets
                int i; /* to deal with the situation we only need fewer packets to send all data rather than size_of_packets */
                int first_sequence_no = INIT_NO;
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
                }
                valid_num_of_packets = i;

                // wait for the ACK sent by the receiver
                struct packet rec_pac;

                // this condition has some problems
                for (; valid_num_of_packets != 0;) { /* loop until the number of valid packets in the window is 0 */
                    recvlen = recvfrom(sockfd, (char *)&rec_pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, &addrlen);
                    if (recvlen > 0) {
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
                            updataPackets(packets, p, size_of_packets);
                            updataAcks(acked, p, size_of_packets);

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
                        }
                    }

                }
                // need to send the FIN packet when all the data is successfully ACKed
                struct packet pac;
                pac.fin = 1;
                sendto(sockfd, (char *)&pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
            }
        }
    }
    close(sockfd);
    return 0;
}


void updataPackets(struct packet packets[], int index, int length) {
    int i = 0;
    for (; index < length; i++, index++) {
        packets[i] = packets[index];
    }
    for (; i < length; i++) {
        struct packet p;
        packets[i] = p;
    }
}

void updataAcks(int acks[], int index, int length) {
    int i = 0;
    for (; index < length; i++, index++) {
        acks[i] = acks[index];
    }
    for (; i < length; i++) {
        acks[i] = 0;
    }
}

