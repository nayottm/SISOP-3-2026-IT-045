#include "protocol.h"
#include <sys/select.h>

typedef struct {
    int socket;
    char name[50];
    int is_admin;
    int authenticated;
} Client;

Client clients[MAX_CLIENTS];
time_t start_time;

void write_log(const char *category, const char *msg) {
    FILE *log_file = fopen("history.log", "a");
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_file, "[%s] %s %s\n", timestamp, category, msg);
    fclose(log_file);
}

void broadcast(char *message, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket > 0 && clients[i].socket != sender_fd && !clients[i].is_admin && clients[i].authenticated) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
}

int main() {
    start_time = time(NULL);
    char ip[20];
    int port;

    if (!read_config("protocol", ip, &port)) exit(EXIT_FAILURE);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    
    write_log("[System]", "[SERVER ONLINE]");
    printf("The Wired is online at %s:%d\n", ip, port);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket > 0) {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_sd) max_sd = clients[i].socket;
            }
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == 0) {
                    clients[i].socket = new_socket;
                    clients[i].authenticated = 0;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket;
            if (FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE);

                if (valread <= 0) {
                    if (clients[i].authenticated) {
                        char log_msg[100];
                        sprintf(log_msg, "[User '%s' disconnected]", clients[i].name);
                        write_log("[System]", log_msg);
                    }
                    close(sd);
                    clients[i].socket = 0;
                } else {
                    if (!clients[i].authenticated) {
                        if (strncmp(buffer, "AUTH_ADMIN:", 11) == 0) {
                            clients[i].is_admin = 1;
                            clients[i].authenticated = 1;
                            strcpy(clients[i].name, ADMIN_USERNAME);
                            write_log("[System]", "[User 'The Knights' connected]");
                        } else {
                            int exists = 0;
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].socket > 0 && strcmp(clients[j].name, buffer) == 0) {
                                    exists = 1; break;
                                }
                            }
                            if (exists) {
                                send(sd, "EXISTS", 6, 0);
                            } else {
                                strcpy(clients[i].name, buffer);
                                clients[i].authenticated = 1;
                                send(sd, "OK", 2, 0);
                                char log_msg[100];
                                sprintf(log_msg, "[User '%s' connected]", clients[i].name);
                                write_log("[System]", log_msg);
                            }
                        }
                    } else if (clients[i].is_admin) {
                        if (strcmp(buffer, "RPC_GET_USERS") == 0) {
                            int count = 0;
                            for(int j=0; j<MAX_CLIENTS; j++) if(clients[j].socket > 0 && !clients[j].is_admin) count++;
                            char res[50]; sprintf(res, "Active Entities: %d", count);
                            send(sd, res, strlen(res), 0);
                            write_log("[Admin]", "[RPC_GET_USERS]");
                        } else if (strcmp(buffer, "RPC_GET_UPTIME") == 0) {
                            char res[50]; sprintf(res, "Uptime: %lds", time(NULL) - start_time);
                            send(sd, res, strlen(res), 0);
                            write_log("[Admin]", "[RPC_GET_UPTIME]");
                        } else if (strcmp(buffer, "RPC_SHUTDOWN") == 0) {
                            write_log("[Admin]", "[RPC_SHUTDOWN]");
                            write_log("[System]", "[EMERGENCY SHUTDOWN INITIATED]");
                            exit(0);
                        }
                    } else {
                        char broadcast_msg[BUFFER_SIZE + 100];
                        sprintf(broadcast_msg, "[%s]: %s", clients[i].name, buffer);
                        broadcast(broadcast_msg, sd);
                        char log_msg[BUFFER_SIZE + 100];
                        sprintf(log_msg, "[[%s]: %s]", clients[i].name, buffer);
                        write_log("[User]", log_msg);
                    }
                }
            }
        }
    }
    return 0;
}