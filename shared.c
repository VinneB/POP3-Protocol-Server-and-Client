#include "shared.h"

int send_head (int sock, int head) {
    int ret = 0;
    head = htonl(head);
    if (send(sock, &head, sizeof(int), 0) == -1) {
        ret = errno;
        errno = 0;
    }
    return ret;
}

int send_msg (int sock, char *msg, int msg_len) {
    int bytes_sent = 0;
    int ret = 0;
    while (bytes_sent < msg_len) {
        if ((bytes_sent += send(sock+bytes_sent, (void *)msg, msg_len-bytes_sent, 0)) == -1) {
            ret = errno;
            errno = 0;
            break;
        }
    }
    return ret;
}

// Space is allocated for buf_p. This must be freed
char *recv_msg (int sock) {
    char *buf_p = NULL;
    int msg_len;
    int bytes_recv = 0;
    if ((bytes_recv = recv(sock, &msg_len, sizeof(int), 0)) == -1) {
    } else if (bytes_recv = 0) {
        errno = -1;
    } else {
        bytes_recv = 0;
        msg_len = ntohl(msg_len);
        buf_p = malloc(msg_len);
        while (bytes_recv < msg_len) {
            if ((bytes_recv += recv(sock, buf_p+bytes_recv, msg_len-bytes_recv, 0)) == -1) {
                free(buf_p);
                buf_p = NULL;
                break;
            } else if (bytes_recv == 0) {
                errno = -1;
                free(buf_p);
                buf_p = NULL;
                break;
            }
        }
        if (bytes_recv == msg_len) { bytes_recv = 0; }
    }
    
    return buf_p;
 
}