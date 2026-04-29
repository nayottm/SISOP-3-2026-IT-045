#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

#define ADMIN_USERNAME "The Knights"
#define ADMIN_PASSWORD "kudalumping"

int read_config(const char *filename, char *ip, int *port);

#endif