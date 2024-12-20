#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int send_head (int sock, int head);
int send_msg (int sock, char *msg, int msg_len);

// Space is allocated for buf_p. This must be freed
char *recv_msg (int sock);


#endif