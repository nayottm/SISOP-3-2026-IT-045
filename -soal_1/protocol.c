#include "protocol.h"

int read_config(const char *filename, char *ip, int *port) {
    FILE *proto_file = fopen(filename, "r");
    if (!proto_file) {
        perror("Gagal membaca file konfigurasi");
        return 0;
    }
    if (fscanf(proto_file, "%s %d", ip, port) != 2) {
        fclose(proto_file);
        return 0;
    }
    fclose(proto_file);
    return 1;
}