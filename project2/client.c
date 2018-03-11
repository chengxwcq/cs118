// what to do if the a.txt file is not received ? 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "packet.h"

#define BUFSIZE 1024
void udp_msg_sender(int, struct sockaddr*, char*);
void updatePackets(struct packet[], int, int, FILE*); /* put the data into the file */
void updateAcks(int acks[], int, int);
int get_the_index(int init_no, int seq_no, int max_packet_length, int max_no);

int main(int argc, char* argv[]) {
    int client_fd, portno;
    struct sockaddr_in ser_addr;
    int recvlen;
    socklen_t addrlen = sizeof(ser_addr);
    unsigned char buf[BUFSIZE];

    if (argc < 4) {
        fprintf(stderr, "insufficient parameters, should be client <server_hostname> <server_portnumber> <filename>\n");
        exit(1);
    }

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    portno = atoi(argv[2]);
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);
    ser_addr.sin_port = htons(portno);

    udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr, argv[3]);

    FILE* fp;
    struct packet rec_pac;
    struct packet send_pac;

    fp = fopen("b.txt", "wba");

    int first_packet_number = INIT_NO; /* used to certify the no. of the packet */
    int size_of_packets = WINDOW_SIZE / MPL;
    struct packet packets[size_of_packets];
    int acks[size_of_packets]; /* used to indicate whether a certain packet has received or not */
    for (int i = 0; i < size_of_packets; i++)
        acks[i] = 0;

    for (;;) {
        recvlen = recvfrom(client_fd, &rec_pac, sizeof(struct packet), 0, (struct sockaddr *)&ser_addr, &addrlen);
//        printf("%s\n", rec_pac.data);
//        fprintf(fp, "%s", rec_pac.data);

        int seq = rec_pac.number;
    printf("Receiving packet %d\n", seq);
//    printf("first number is %d\n", first_packet_number);
        if (seq == first_packet_number) { /* meaning that it can be writen into the file */
            packets[0] = rec_pac;
            // check if other packets can be written
            int k = 1;
            for (; k < size_of_packets; k++) {
                if (acks[k] == 0) {
                    break;
                }
            }
            updateAcks(acks, k, size_of_packets);
            updatePackets(packets, k, size_of_packets, fp);
 //   printf("un- updated first_packet_no is %d, MPL is %d, k is %d\n", first_packet_number, MPL, k);
            first_packet_number = first_packet_number + MPL * k;

            if (first_packet_number > MAX_SEQ_NO)
                first_packet_number = first_packet_number - first_packet_number / MAX_SEQ_NO * MAX_SEQ_NO;

  //  printf("updated first_packet_no is %d\n", first_packet_number);

        } else { /* store the packet */
            // if the packet has been acked but the ack not received by the server, ignore the packet
            // if the packet is in the window
            if (seq >= first_packet_number || first_packet_number - seq > size_of_packets * MPL) {
                int index = get_the_index(first_packet_number, seq, MPL, MAX_SEQ_NO);
                packets[index] = rec_pac;
                acks[index] = 1;
            }
        }

        if (rec_pac.fin == 1) {
            for (int i = 0; i < size_of_packets; i++) {
                if (acks[i] == 1) {
                    fprintf(fp, "%s", packets[i].data);
                } else {
                    break;
                }
            }
            printf("%s\n", "fin");
            break;
        }

        if (rec_pac.type == -1) /* meaning that the file doesn't exist */
            break;

        send_pac.type = 1;
        send_pac.number = rec_pac.number;
        sendto(client_fd, (char *)&send_pac, sizeof(struct packet), 0, (struct sockaddr *)&ser_addr, addrlen);
        printf("Sending packet %d\n", rec_pac.number);

        memset((char *)&rec_pac, 0, sizeof(struct packet));
        memset((char *)&send_pac, 0, sizeof(struct packet));
    }
    fclose(fp);
    close(client_fd);
    return 0;
}

void udp_msg_sender(int fd, struct sockaddr* dst, char* filename) {
    printf("client:%s\n", filename);
    sendto(fd, filename, strlen(filename), 0, dst, sizeof(*dst));
    printf("%s\n", filename);
}

int get_the_index(int init_no, int seq_no, int max_packet_length, int max_no) {
    if (seq_no < init_no) {
        int p = (max_no - init_no) / MPL;
        if (seq_no % MPL == 0) {
            return p + seq_no / MPL;
        } else {
            return p + seq_no / MPL + 1;
        }
    }
    int diff = seq_no - init_no;
    if (diff % max_packet_length == 0) {
        return diff / max_packet_length;
    } else {
        return diff / max_packet_length + 1;
    }
}

void updatePackets(struct packet packets[], int index, int length, FILE* fp) {
    for (int j = 0; j < index; j++) {
        fprintf(fp, "%s", packets[j].data);
    }
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
