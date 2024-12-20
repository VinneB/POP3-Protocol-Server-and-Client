#include "shared.h"

#define USER_BUF_SIZE 32

void console (int server_sock);
void cleanup (int sock, char *buf);
char *req_user_input();

int main (int argc, char **argv) {
    struct addrinfo hints, *servinfo, *p;
    int ret, server_sock = -1;

    if (argc != 3) {
        fprintf(stderr, "client <host> <port>\n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(argv[1], argv[2], &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if((server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            errno = 0;
            continue;
        }
        if (connect(server_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_sock);
            perror("bind");
            errno = 0;
            continue;
        }
        break;
    }

    freeaddrinfo(p); // Free the linked list of addrinfo structs

    if (p = NULL) {
        fprintf(stderr, "Unable to connect to server\n");
        cleanup(server_sock, NULL);
        exit(1);
    }

    console(server_sock);
    
}

void console (int server_sock) {
    int ret, msg_len, con_flg = 0, email_fd, email_id; // Con_flg: 1 - terminate ;
    char *buf = NULL, temp_str[15];

    // Wait for server initital response
    if ((buf = recv_msg(server_sock)) == NULL) {
        if (errno == -1) {
            fprintf(stderr, "Server disconnected\n");
            cleanup(server_sock, buf);
            exit(1);
        } else {
            fprintf(stderr, "recv_msg: %s\n", strerror(errno));
            cleanup(server_sock, buf);
            exit(1);
        }
    }

    printf("S: %s\n", buf);
    free(buf);
    buf = NULL;

    ret = 0;
    while (1) {
        buf = req_user_input(); // Allocates memory for buf inside function. Needs to be freed.
        snprintf(temp_str, 5, "%s", buf);
        printf("%s.\n", temp_str);
        if (!strcmp(buf, "QUIT")) {
            con_flg = 1;
        } else if (!strcmp(temp_str, "RETR")) {
            con_flg = 2;
            snprintf(temp_str, 2, "%s", buf+5);
            email_id = atoi(temp_str);
        }

        msg_len = strlen(buf)+1;
        if ((ret = send_head(server_sock, msg_len)) != 0) {
            fprintf(stderr, "send (head): %s\n", strerror(ret));
            continue;
        }
        if ((ret = send_msg(server_sock, buf, msg_len)) != 0) {
            fprintf(stderr, "send (msg): %s\n", strerror(ret));
            continue;
        }
        free(buf);
        buf = NULL;

        if ((buf = recv_msg(server_sock)) == NULL) {
            if (errno == -1) {
                fprintf(stderr, "Server disconnected\n");
            } else {
                fprintf(stderr, "recv_msg: %s\n", strerror(errno));
            }
            cleanup(server_sock, buf);
            exit(1);
        }

        printf("S: %s\n", buf);

        if (con_flg == 2) {
            sprintf(temp_str, "email-%d", email_id);
            email_fd = open(temp_str, O_CREAT | O_WRONLY, 0666);
            printf("%d\n", strlen(buf));
            write(email_fd, buf, strlen(buf));
            close(email_fd);
            email_fd = 0;
            con_flg = 0;
        }
        
        // Cleanup
        free(buf);
        buf = NULL;
        msg_len = 0;
        ret = 0;

        if (con_flg == 1) {
            cleanup(server_sock, buf);
            exit(0);
        }
    }
    
}

void cleanup (int sock, char *buf) {
    if (sock != -1) {
        close(sock);
    }
    if (buf != NULL) {
        free(buf);
    }
}

char *req_user_input() {
    char *buf = NULL;
    size_t len = 0;
    printf("C: ");
    getline(&buf, &len, stdin);
    buf[strlen(buf)-1] = '\0';
    return buf;
}
