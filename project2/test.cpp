#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
using namespace std;

int length(void*);
int main(int argc, char *argv[]) {
    // test memset()
    int p[] = {1,2,3,4};
    int x = 10;
    // char* q = (char *)p;
    char *q = (char*)"abcde";
    for (int i = 0; i < 10; i++) {
        printf("%c\n", *(q + i));
    }
}

int length(void* p) {
    int i = 0;
    char *q = (char *)p;
    while (*q != '\0') {
        q += 1;
        i++;
    }
    return i;
}
