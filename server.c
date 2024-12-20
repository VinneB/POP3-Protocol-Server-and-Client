#include "shared.h"
#include <signal.h>
#include <dirent.h>

#define BACKLOG 10

#ifndef EMAIL_FILE_EXT
#define EMAIL_FILE_EXT "txt"
#endif


// Results of hash with len = 3
#define STAT_H 193466723
#define LIST_H 193452819
#define RETR_H 193463942
#define DELE_H 193444488
#define TOP_H 193462350
#define QUIT_H 193460232

#define LIST_LN_SZ 10
#define OK_MSG_SZ 4

// Node like data structure used for storing email data
typedef struct Email Email;

struct Email {
    int index;
    int size;
    int head_size;
    char *data;
    char *filename;
    Email *next;
};

// Globals
int server_sock;
int client_sock;
struct addrinfo *res;
Email *head;
char *msg_buf;

Email *load_emails ();
void free_emails (Email *head);
void free_email (Email *email);

int elist_size (Email *head);
Email *get_email (Email *head, int e_num);
Email *rem_email (Email *head, int e_num);


void cleanup(int fd1, int fd2, struct addrinfo *addrinfo_res, Email *head, char *buf) {
    if (fd1 != -1) {
        close(fd1);
    }
    if (fd2 != -1) {
        close(fd2);
    }
    if (addrinfo_res != NULL) {
        freeaddrinfo(addrinfo_res);
    }
    if (head != NULL) {
        free_emails(head);
    }
    if (buf != NULL) {
        free(buf);
    }
}

void term_handler (int signum) {
    cleanup(server_sock, client_sock, res, head, msg_buf);
    exit(0);
}

int hash(const char* s, size_t len)
{
    unsigned int h = 5381;
    while (len--) {
        /* h = 33 * h ^ s[i]; */
        h += (h << 5);  
        h ^= *s++;
    }
    return h;
}

int get_tok (char *msg, char *token, char delim) {
    int i = 0;
    for (i; (msg[i] != delim) && i < strlen(msg); i++) {
        token[i] = msg[i];
    }
    token[i++] = '\0'; // Skip over space and set null terminator
    return i;
}

char *get_file_ext (char *name, char *ext) {
    char *p;
    for (p = name; (*p != '.') && (*p != '\0'); p++);
    if (*p != '\0') {
        strcpy(ext, ++p);
    } else {
        ext[0] = '\0';
    }
    return ext;
}




int main (int argc, char **argv) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    struct addrinfo hints, *p = NULL;
    res = NULL;
    server_sock = -1;
    client_sock = -1; 
    int port, err_code, msg_len, msg_index, arg_int, res_beh_flag = 0;
    char pop3_comm[8], pop3_arg1[8], pop3_arg2[8], temp[32];
    Email *email = NULL;
    head = NULL;

    // res_beh_flag: 1 - terminate ; 2 - ill-format error


    // TEMP (TESTING)
    /*
    if ((head = load_emails()) == NULL) {
        exit(1);
    }
    Email *node = head;
    char header[512];
    char body[512];
    while (node != NULL) {
        snprintf(header, node->head_size, node->data);
        snprintf(body, ((node->size + 1) - node->head_size), (node->data+node->head_size));
        printf("--- HEAD ---\n%s\n---BODY---\n%s\n", header, body);
        node = node->next;
    }
    free_emails(head);

    while (node != NULL) {
        printf("Size: %d\nHead Size: %d\n\n%s\n\n",node->size, node->head_size, node->data);
        node = node->next;
    }
    

    exit(0);
    */


    

    // Response strings
    char *conn_msg = "+OK POP3 server ready";
    char *term_msg = "+OK POP3 server signing off";
    char *illform_msg = "-ERR ill formatted command";

    if (argc != 2) {
        printf("server <port>\n");
        exit(1);
    }
    port = atoi(argv[1]);

    // Setup term signal handling
    struct sigaction term_action;
    term_action.sa_handler = term_handler;
    sigemptyset(&term_action.sa_mask);
    term_action.sa_flags = 0;
    sigaction(SIGINT, &term_action, NULL);


    // Setup for our getaddrinfo call
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get info for server socket
    if ((err_code = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err_code));
        exit(1);
    }

    err_code = -1;
    // Attempt to setup a server socket using our local address info
    for (p = res; p != NULL; p = p->ai_next) {
        if((server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            errno = 0;
            continue;
        }
        if (bind(server_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_sock);
            perror("bind");
            errno = 0;
            continue;
        }
        if (listen(server_sock, BACKLOG) == -1) {
            close(server_sock);
            perror("listen");
            errno = 0;
            continue;
        }
        err_code = 0;
        break;
    }

    // End program if unable to create a functional server socket
    if (err_code == -1) { // The error code is -1 if we were unable to 
        cleanup(server_sock, client_sock, res, head, msg_buf);
        exit(1);
    }

    freeaddrinfo(res); // Free the linked list of addrinfo structs
    res = NULL;

    client_addr_size = sizeof(client_addr);

    head = load_emails();
    while (1) {
		printf("Waiting for connection...\n");
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size); // Wait for incoming client connections

        printf("Connected to client\n");

        // Send response
        err_code = 0;
        msg_len = strlen(conn_msg) + 1;
        err_code = send_head(client_sock, msg_len);
        if (err_code != 0) {
            fprintf(stderr, "send (head):", strerror(err_code));
        } else {
            err_code = send_msg(client_sock, conn_msg, msg_len);
            if (err_code != 0) {
                fprintf(stderr, "send (msg): %s\n", strerror(err_code));
            }
        }

        if (err_code != 0) {
            err_code = 0;
            msg_len = 0;
            close(client_sock);
            continue;
        }

        // Handle client POP3 Commands

        while (1) {

            // Wait for client command
            if ((msg_buf = recv_msg(client_sock)) == NULL) {
                if (errno == -1) {
                    fprintf(stderr, "Client disconnected\n");
                } else {
                    fprintf(stderr, "recv_msg: %s\n", strerror(errno));
                }
                free(msg_buf);
                close(client_sock);
                errno = 0;
                break;
            }

            // Parse POP3 Command
            msg_index = 0;
            msg_index += get_tok(msg_buf+msg_index, pop3_comm, ' ');

            printf("Recieved %s. Executing %s\n", msg_buf, pop3_comm);

            switch (hash(pop3_comm, 3)) {
                case STAT_H: { // Send back # of stored msgs ; 0 args
                    free(msg_buf);
                    msg_buf = malloc(LIST_LN_SZ+OK_MSG_SZ+1);
                    sprintf(msg_buf, "+OK %d", elist_size(head));
                } break;
                case LIST_H: { // List all stored msgs (inc # and sz) ; 0 args
                    free(msg_buf);
                    msg_buf = malloc((elist_size(head)*LIST_LN_SZ)+OK_MSG_SZ+1); // Buf size: (line_len * num_of_line) + len("+OK") + len('\0')
                    msg_index = 0;
                    strcpy(msg_buf, "+OK\n");
                    msg_index = 4;
                    for (email = head; email != NULL; email = email->next) {
                        sprintf(temp, "%d %d\n", email->index, email->size);
                        strcpy(msg_buf+msg_index, temp);
                        msg_index += strlen(temp);
                    }
                    msg_buf[strlen(msg_buf)-1] = '\0'; // Remove trailing newline
                    
                } break;
                case RETR_H: { // Send email with # = arg1 ; 1 args
                    msg_index += get_tok(msg_buf+msg_index, pop3_arg1, ' ');
                    arg_int = atoi(pop3_arg1);
                    free(msg_buf);
                    email = get_email(head, arg_int);
                    msg_buf = malloc((email->size)+OK_MSG_SZ+1);
                    sprintf(msg_buf, "+OK\n%s", email->data);

                } break;
                case DELE_H: { // Delete email with # = arg1 ; 1 args
                    msg_index += get_tok(msg_buf+msg_index, pop3_arg1, ' ');
                    arg_int = atoi(pop3_arg1);
                    rem_email(head, arg_int);
                    free(msg_buf);
                    msg_buf = malloc(OK_MSG_SZ+1);
                    sprintf(msg_buf, "+OK");

                } break;
                case TOP_H: { // Send header and first arg2 lines of email with # = arg1 ; 2 args
                    msg_index += get_tok(msg_buf+msg_index, pop3_arg1, ' ');
                    arg_int = atoi(pop3_arg1);
                    msg_index += get_tok(msg_buf+msg_index, pop3_arg2, ' ');
                    email = get_email(head, arg_int);
                    free(msg_buf);
                    int count = 0;
                    int j = 0;
                    for (j = 0; count < atoi(pop3_arg2); j++) {
                        if (email->data[(email->head_size)+j] == '\n') { count++; }
                    }
                    msg_buf = malloc(OK_MSG_SZ+(email->head_size)+j+1);
                    snprintf(msg_buf, OK_MSG_SZ+(email->head_size)+j+1, "+OK\n%s", email->data);

                } break;
                case QUIT_H: { // Terminate conv with client ; 0 args
                    free(msg_buf);
                    msg_buf = malloc(strlen(term_msg)+1);
                    strcpy(msg_buf, term_msg);
                    res_beh_flag = 1;
                } break;
                default: {
                    res_beh_flag = 2;
                    free(msg_buf);
                }
            }

            if (res_beh_flag == 2) {
                msg_buf = malloc(strlen(illform_msg)+1);
                strcpy(msg_buf, illform_msg);
            }

            // Send response to client (relies on msg_buf)
            msg_len = strlen(msg_buf)+1;

            err_code = send_head(client_sock, msg_len);
            if (err_code != 0) {
                fprintf(stderr, "send (head):", strerror(err_code));
            }
            err_code = send_msg(client_sock, msg_buf, msg_len);
            if (err_code != 0) {
                fprintf(stderr, "send (msg): %s\n", strerror(err_code));
            }


            // Clean up
            msg_len = 0;
            msg_index = 0;
            free(msg_buf);
            msg_buf = NULL;
            email = NULL;
            strcpy(pop3_comm, "");
            strcpy(pop3_arg1, "");
            strcpy(pop3_arg2, "");
            arg_int = 0;

            // Terminate connection with client if res_beh_flag is set
            if (res_beh_flag) {
                close(client_sock);
                client_sock = -1;
                res_beh_flag = 0;
                break;
            }

        }

    }
}

// NOTE: the body of the email (end of the file) does not terminate with a '\n'

Email *load_emails () {
    DIR *d = NULL;
    char ext[6], temp;
    struct dirent *dir = NULL;
    int fd, flg = 0, i = 1, read_flg = 0;
    Email *node = NULL, *head = NULL;

    if ((d = opendir(".")) == NULL) {
        perror("opendir (load_emails)");
        errno = 0;
        return NULL;
    }
    while ((dir = readdir(d)) != NULL) {
        if (!strcmp(get_file_ext(dir->d_name, ext), EMAIL_FILE_EXT)) { // Only make nodes for files corresponding to an email
            // Allocate heap space for node
            if (head == NULL) { // First node
                node = malloc(sizeof(Email));
                head = node;
            } else {
                node->next = malloc(sizeof(Email));
                node = node->next;
            }

            node->index = i++;
            node->next = NULL;

            node->filename = malloc(strlen(dir->d_name)+1);
            strcpy(node->filename, dir->d_name);

            if ((fd = open(dir->d_name, O_RDONLY)) == -1) {
                perror("open (load_emails)");
                errno = 0;
                free_emails(head);
                return NULL;
            } 

            // Get the size of the file
            node->size = lseek(fd, 0L, SEEK_END);
            lseek(fd, 0L, SEEK_SET);

            // Find the end of the header i.e. size of header
            while ((read_flg = read(fd, &temp, 1)) > 0) {
                if (temp == '\n' && flg == 1) {
                    node->head_size  = lseek(fd, 0L, SEEK_CUR); // Set the head size to the position where the first empty line is
                    break;
                } if (temp == '\n') {
                    flg = 1;
                } else {
                    flg = 0;
                }
            }
            if (read_flg == -1) {
                perror("read (load_emails)");
                errno = 0;
                free_emails(head);
                return NULL;
            }
            
            // Get data
            lseek(fd, 0L, SEEK_SET);
            node->data = (char*) malloc((node->size)+1);
            read_flg = 0;
            while (read_flg < node->size) {
                if ((read_flg += read(fd, node->data+read_flg, node->size-read_flg)) == -1) {
                    perror("read (load_emails)");
                    errno = 0;
                    free_emails(head);
                    return NULL;
                }
            }
            node->data[node->size] = '\0';


            // Clean up
            close(fd);
            fd = -1;
            flg = 0;
            temp = '\0';
        }
    }
        return head;
}

void free_emails (Email *head) {
    Email *temp = NULL;
    do {
        temp = head;
        head = head->next;
        free_email(temp);
        temp = NULL;
    } while (head != NULL);
}

void free_email (Email *email) {
    free(email->data);
    free(email->filename);
    free(email);
    email = NULL;
}

// Email linked list helper functions
int elist_size (Email *head) {
    int i;
    for (i = 0; head != NULL; i++) {
        head = head->next;
    };
    return i;
}

Email *get_email (Email *head, int e_num) {
    for (head; (head != NULL) && (head->index != e_num); head = head->next);
    return head;
}

// Remove email with email->index = e_num. Returns head. Return NULL if email not found or if remove the only email in the list.
Email *rem_email (Email *head, int e_num) {
    Email *node, *temp;
    if (head->index == e_num) { // Case: removing head
        remove(head->filename);
        node = head->next;
        free_email(head);
        return node;
    }

    for (node = head; (node->next != NULL) && (node->next->index != e_num); head = head->next); // Make node the email before the one we want to remove
    if (node->next != NULL) {
        temp = node->next;
        node->next = node->next->next;
        remove(temp->filename);
        free_email(temp);
    } else {
        return NULL;
    }
    return head;
}
