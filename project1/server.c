#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <time.h>

void dostuff(int);
// used to deal with the situation when name contains space
char *replace(const char *s, const char *old, const char *new);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));   // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        error("ERROR on accept");
    }
    dostuff(newsockfd);
    close(sockfd);
    close(newsockfd);
    return 0;
}

void dostuff(int sock) {
    int n;
    int isExist = 0;
    int buffer_size = 1024;    
    char buffer[buffer_size];
    long length = 0;
    char *content = 0;
    bzero(buffer, buffer_size);
    n = read(sock, buffer, buffer_size - 1);
    if (n < 0) error("ERROR reading from socket");
    printf("%s\n", buffer);

    // get the file name
    char *p = strtok(buffer, " ");
    p = strtok(NULL, " ");
    char filename[50] = ".";
    strcat(filename, p);
    char *tmp = replace(filename, "%20", " ");

    char *file_prefix = strtok(tmp, ".");
    char *type = strtok(NULL, ".");

    // file_name has changed in such case
    // printf("%s\n", file_prefix);
    // printf("%s\n", type);

    // transform the type into lower case
    for (char *cc = type; *cc; ++cc) {
        if (*cc > 0x40 && *cc < 0x5b) {
            *cc = *cc + 32;
        }
    }
    memset(filename, '\0', sizeof(filename));
    strcat(filename, file_prefix);
    strcat(filename, ".");
    strcat(filename, type);

    char file_name[50] = ".";
    strcat(file_name, filename);
    printf("%s\n", file_name);
    // test if this file is in the directory
    FILE *fp = fopen(file_name, "rb");
    char response[400];
    bzero(response, 400);
    char *header = 0;
    
    if (strcmp(file_name, "./") == 0) {
        fclose(fp);    
        fp = NULL;
    }
    if (fp == NULL) {
        isExist = 0;
        header = "HTTP/1.1 404 Not Found\n";
    } else {
        isExist = 1;
        header = "HTTP/1.1 200 OK\n";
    }

    // get the current time
    struct tm *newtime;
    char date[128];
    time_t lt;
    lt= time(NULL);
    newtime = localtime(&lt);
    strftime(date, 128, "%a, %d %b %Y %T GMT", newtime);

    // set the header
    strcat(response, header);
    strcat(response, "Connection: close\n");
    strcat(response, "Server: chengma's server\nDate: ");
    strcat(response, date);

    // get the type of the file
    if (isExist) {
        char *content_type;
        if (strcmp(type, "jpg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(type, "jpeg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(type, "gif") == 0) {
            content_type = "image/gif";
        } else if (strcmp(type, "html") == 0) {
            content_type = "text/html";
        } else if (strcmp(type, "htm") == 0) {
            content_type = "text/htm";
        }
        strcat(response, "\nContent-Type: ");
        strcat(response, content_type);
    }
    strcat(response, "\n\n");
    if (isExist) {
        // get the content of the file
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        content = malloc(length);
        fread(content, 1, length - 1, fp); 
    } else {
        content = "<h1>The file is not in the directory</h1>";
        length = strlen(content);
    }
    write(sock, response, strlen(response));
    write(sock, content, length - 1);
    if (fp != NULL) {
        fclose(fp);
    }
}

char *replace(const char *s, const char *old, const char *new) {
    char * result;
    int i, cnt = 0;
    int newLen = strlen(new);
    int oldLen = strlen(old);

    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], old) == &s[i]) {
            cnt++;
            i += oldLen - 1;
        }
    }
    result = (char*) malloc(i + cnt * (newLen - oldLen) + 1);
    i = 0;
    while (*s) {
        if (strstr(s, old) == s) {
            strcpy(&result[i], new);
            i += newLen;
            s += oldLen;
        } else {
            result[i++] = *s++;
        }
    }
    result[i] = '\0';
    return result;
}




