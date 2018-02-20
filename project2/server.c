#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024
void doTransmission(int);

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen = sizeof(cli_addr);
    int recvlen; /* the length of the received data */
    unsigned char buf[1024]; /* used to receive the message from the client */

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
        printf("waiting on port %d\n", portno);
        recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&cli_addr, &addrlen);
        printf("received %d bytes\n", recvlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("received message: \"%s\"\n", buf);
        }
    }
    close(sockfd);
    return 0;
}

void doTransmission(int sock) {

}



