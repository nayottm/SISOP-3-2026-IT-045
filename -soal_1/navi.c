#include "protocol.h"
#include <pthread.h>
#include <signal.h>

int sock = 0;
char name[50];
int is_admin = 0;

void *receive_handler(void *arg) {
    char buffer[1024];
    while (1) {
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) exit(0);
        buffer[valread] = '\0';
        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

void handle_exit(int sig) {
    printf("\n[System] Disconnecting from The Wired...\n");
    close(sock);
    exit(0);
}

int main() {
    signal(SIGINT, handle_exit);
    char ip[20]; int port;
    if (!read_config("protocol", ip, &port)) exit(1);

    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;

while (1) {
        printf("Enter your name: ");
        scanf(" %[^\n]", name); 
        if (strcmp(name, ADMIN_USERNAME) == 0) {
            char pass[50];
            printf("Enter Password: ");
            scanf("%s", pass);
            
            if (strcmp(pass, ADMIN_PASSWORD) == 0) {
                send(sock, "AUTH_ADMIN:protocol7", 20, 0);
                is_admin = 1;
                printf("[System] Authentication Successful. Granted Admin privileges.\n");
                break;
            } else {
                printf("[System] Wrong Password. Access Denied.\n");
                continue; 
            }
        } else {
            // Logika untuk user biasa (Alice, Lain, dll)
            send(sock, name, strlen(name), 0);
            char res[10];
            int v = read(sock, res, 10);
            res[v] = '\0';
            
            if (strcmp(res, "OK") == 0) {
                printf("--- Welcome to The Wired, %s\n", name);
                break;
            } else {
                printf("[System] The identity '%s' is already synchronized.\n", name);
            }
        }
    }

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    char input[BUFFER_SIZE];
    while (1) {
        if (is_admin) {
            printf("=== THE KNIGHTS CONSOLE ===\n1. Check Active Entities\n2. Check Server Uptime\n3. Emergency Shutdown\n4. Exit\nCommand >> ");
            int c; scanf("%d", &c);
            if (c == 1) send(sock, "RPC_GET_USERS", 13, 0);
            else if (c == 2) send(sock, "RPC_GET_UPTIME", 14, 0);
            else if (c == 3) send(sock, "RPC_SHUTDOWN", 12, 0);
            else if (c == 4) handle_exit(0);
        } else {
            printf("> ");
            scanf(" %[^\n]", input);
            if (strcmp(input, "/exit") == 0) handle_exit(0);
            send(sock, input, strlen(input), 0);
        }
    }
    return 0;
}