#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int server_fd = -1;
volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        keep_running = 0;
        // Trigger a shutdown on server_fd to break accept()
        if (server_fd != -1) {
            shutdown(server_fd, SHUT_RDWR);
        }
    }
}

void setup_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    setup_signals();

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        freeaddrinfo(res);
        return -1;
    }

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        if (pid > 0) {
            exit(0);
        }
        if (setsid() == -1) {
            perror("setsid");
            return -1;
        }
        if (chdir("/") == -1) {
            perror("chdir");
            return -1;
        }
        int dev_null = open("/dev/null", O_RDWR);
        if (dev_null != -1) {
            dup2(dev_null, STDIN_FILENO);
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
            close(dev_null);
        }
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    while (keep_running) {
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) {
            if (errno == EINTR || !keep_running) {
                break;
            }
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char *buffer = malloc(BUFFER_SIZE);
        if (!buffer) {
            syslog(LOG_ERR, "malloc failed");
            close(client_fd);
            continue;
        }

        ssize_t bytes_received;
        size_t total_received = 0;
        size_t buffer_capacity = BUFFER_SIZE;

        int data_file_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (data_file_fd == -1) {
            syslog(LOG_ERR, "open data file failed: %s", strerror(errno));
            free(buffer);
            close(client_fd);
            continue;
        }

        while (keep_running && (bytes_received = recv(client_fd, buffer + total_received, buffer_capacity - total_received - 1, 0)) > 0) {
            total_received += bytes_received;
            buffer[total_received] = '\0';

            char *newline_ptr;
            while ((newline_ptr = strchr(buffer, '\n')) != NULL) {
                size_t packet_len = (newline_ptr - buffer) + 1;
                
                if (write(data_file_fd, buffer, packet_len) == -1) {
                    syslog(LOG_ERR, "write to file failed: %s", strerror(errno));
                }

                // Send back full content
                lseek(data_file_fd, 0, SEEK_SET);
                char send_buffer[BUFFER_SIZE];
                ssize_t bytes_read;
                while ((bytes_read = read(data_file_fd, send_buffer, sizeof(send_buffer))) > 0) {
                    send(client_fd, send_buffer, bytes_read, 0);
                }
                lseek(data_file_fd, 0, SEEK_END);

                // Move remaining data to the front
                size_t remaining = total_received - packet_len;
                memmove(buffer, newline_ptr + 1, remaining);
                total_received = remaining;
                buffer[total_received] = '\0';
            }

            if (total_received >= buffer_capacity - 1) {
                buffer_capacity += BUFFER_SIZE;
                char *new_buffer = realloc(buffer, buffer_capacity);
                if (!new_buffer) {
                    syslog(LOG_ERR, "realloc failed");
                    break;
                }
                buffer = new_buffer;
            }
        }

        if (bytes_received == -1 && errno != EINTR) {
            perror("recv");
        }

        close(data_file_fd);
        free(buffer);
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    if (server_fd != -1) {
        close(server_fd);
    }
    unlink(DATA_FILE);
    closelog();

    return 0;
}
