#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "packet.h"

int main(int argc, char *argv[]) {
    struct packet p;
    p.type = 1;
    printf("%d\n", p.type);
}

