#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define TIMEOUT_SECS 30
#define BUFFER_SIZE 65536

int main(int argc, const char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <host> <port> [authorization token]\n", *argv);
        return EINVAL;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return errno;
    }
    struct hostent *host = gethostbyname(argv[1]);
    if (!host) {
        herror("gethostbyname");
        return h_errno;
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(atoi(argv[2])),
        .sin_zero = 0
    };
    bcopy(host->h_addr, &addr.sin_addr.s_addr, host->h_length);
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return errno;
    }
    const char *header0 = "CONNECT localhost:22 HTTP/1.1\r\n"
                          "Host: zachdeibert.github.io\r\n";
    if (write(sockfd, header0, strlen(header0)) < 0) {
        perror("write");
        close(sockfd);
        return errno;
    }
    const char *header2 = "\r\n";
    if (argc == 4) {
        const char *header1 = "Authorization: Basic ";
        if (write(sockfd, header1, strlen(header1)) < 0) {
            perror("write");
            close(sockfd);
            return errno;
        }
        if (write(sockfd, argv[3], strlen(argv[3])) < 0) {
            perror("write");
            close(sockfd);
            return errno;
        }
        if (write(sockfd, header2, strlen(header2)) < 0) {
            perror("write");
            close(sockfd);
            return errno;
        }
    }
    if (write(sockfd, header2, strlen(header2)) < 0) {
        perror("write");
        close(sockfd);
        return errno;
    }
    char buffer[BUFFER_SIZE];
    while (1) {
        int len = read(sockfd, buffer, BUFFER_SIZE);
        int seq = 0;
        for (int i = 0; i < len; ++i) {
            switch (seq) {
                case 0:
                    if (buffer[i] == '\r') {
                        seq = 1;
                    }
                    break;
                case 1:
                    if (buffer[i] == '\n') {
                        seq = 2;
                    } else {
                        seq = 0;
                    }
                    break;
                case 2:
                    if (buffer[i] == '\r') {
                        seq = 3;
                    } else {
                        seq = 0;
                    }
                    break;
                case 3:
                    if (buffer[i] == '\n') {
                        write(STDOUT_FILENO, buffer + i + 1, len - i - 1);
                        goto pipe;
                    } else {
                        seq = 0;
                    }
                    break;
            }
        }
    }
    pipe:
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(sockfd, &rfds);
        struct timeval timeout = {
            .tv_sec = TIMEOUT_SECS,
            .tv_usec = 0
        };
        int ret = select(sockfd + 1, &rfds, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("select");
            close(sockfd);
            return errno;
        } else if (ret > 0) {
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                int len = read(STDIN_FILENO, buffer, BUFFER_SIZE);
                if (len < 0) {
                    perror("read");
                    close(sockfd);
                    return errno;
                }
                if (write(sockfd, buffer, len) < 0) {
                    perror("write");
                    close(sockfd);
                    return errno;
                }
            }
            if (FD_ISSET(sockfd, &rfds)) {
                int len = read(sockfd, buffer, BUFFER_SIZE);
                if (len < 0) {
                    perror("read");
                    close(sockfd);
                    return errno;
                }
                if (write(STDOUT_FILENO, buffer, len) < 0) {
                    perror("write");
                    close(sockfd);
                    return errno;
                }
            }
        } else {
            errno = ETIMEDOUT;
            perror("select");
            close(sockfd);
            return 0;
        }
    }
    return 0;
}
