#define MPL 1024
#define WINDOW_SIZE 5120
#define MAX_SEQ_NO 30720
#define TIME_OUT 500
#define PAYLOAD_SIZE (MPL - sizeof(int) * 3 - sizeof(unsigned int))

struct packet {
    int type; /* 0 means ACK, 1 means SEQ*/
    int syn, fin;
    unsigned int number;
    char data[PAYLOAD_SIZE];
};
