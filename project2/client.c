#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024
void udp_msg_sender(int, struct sockaddr*, char*);

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

    for (;;) {
        recvlen = recvfrom(client_fd, buf, BUFSIZE, 0, (struct sockaddr *)&ser_addr, &addrlen);
        printf("%s\n", buf);
    }
    close(client_fd);
    return 0;
}

void udp_msg_sender(int fd, struct sockaddr* dst, char* filename) {
    printf("client:%s\n", filename);
    sendto(fd, filename, strlen(filename), 0, dst, sizeof(*dst));
    printf("%s\n", filename);
}


