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
#define INIT_NO 5096

void doTransmission(int);

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
        if (recvlen > 0) {
            buf[recvlen] = '\0';
            FILE *fp = fopen((char*)buf, "rb");
            if (fp == NULL) {
                char *response = (char*) "File not exists";
                sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&cli_addr, addrlen);
            } else {
                int size_of_packets = WINDOW_SIZE / MPL;
                struct packet packets[size_of_packets]; /* used to buffer the sent packets*/
                int acked[size_of_packets]; /* if acked, then 1; else 0*/

                while (1) { /* maybe this outer loop is unneccessary*/
                    // first put all prepared packets into array packets
                    int i; /* to deal with the situation we only need fewer packets to send all data rather than size_of_packets */
                    int first_sequence_no = INIT_NO;
                    int next_no = INIT_NO;


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
                        sendto(sockfd, (char *)&packets[j], sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                    }

                    // wait for the ACK sent by the receiver
                    struct packet rec_pac;
                    for (;;) {
                        recvlen = recvfrom(sockfd, (char *)&rec_pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, &addrlen);
                        if (recvlen > 0) {
                            // check the ACK
                            int ack_no = rec_pac.number;
                            if (ack_no == first_sequence_no) {
                                
                            }
                            printf("receive ack no. %d\n", ack_no);
                        }
                    }


                   // need to send the FIN packet when all the data is successfully ACKed

                // fin
                // pac.fin = 1;
                // sendto(sockfd, (char *)&pac, sizeof(struct packet), 0, (struct sockaddr *)&cli_addr, addrlen);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

void doTransmission(int sock) {

}



