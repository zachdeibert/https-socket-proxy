#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define TIMEOUT_SECS 90
#define BUFFER_SIZE 65536

int main(int argc, const char **argv) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        puts("Status: 500 Internal Server Error\n");
        perror("socket");
        return errno;
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr = {
            .s_addr = 0x0100007F
        },
        .sin_port = htons(22),
        .sin_zero = 0
    };
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        puts("Status: 500 Internal Server Error\n");
        perror("connect");
        close(sockfd);
        return errno;
    }
    char buffer[BUFFER_SIZE];
    puts("\n");
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
}
