#include <stdio.h>
#include <unistd.h>
#include "util.h"
#include <stdbool.h>

long time_out = -1;
long count = -1;
char* file_path = PROC_PATH FILE_NAME;

int main(int argc, char *argv[]) {
    
    set_settings(argc, argv);
    read_from_file(0);

    if (time_out > 0) {
        if (count > 0) {
            for (int i = 0; i < count - 1; ++i) {
                sleep(time_out);
                read_from_file(1);
            }
        } else {
            while (true) {
                sleep(time_out);
                read_from_file(1);
            }
        }
    }
    
    return 0;
}

void read_from_file(int output) {
    FILE *vmstat_file = fopen(file_path, "r");
    if (!vmstat_file) {
        printf("File doesn't exist\n");
        return;
    }

    // Reading data from file
    int r, b, free_mem, buff, cache, sys, usr;
    fscanf(vmstat_file, "r = %d\nb = %d\nfree = %d\nbuff = %d\ncache = %d\nsys = %d\nusr = %d",
           &r, &b, &free_mem, &buff, &cache, &sys, &usr);

    fclose(vmstat_file);

    // Printing in the adjusted format
    printf("--procs-- ------------memory---------- --cpu--\n");
    printf("  r   b         free   buff   cache    sys  usr\n");
    printf("%3d %3d   %10d %6d %7d   %4d %4d\n", r, b, free_mem, buff, cache, sys, usr);

    // Optionally, you can add logic to handle the 'output' parameter as needed.
}


void set_settings(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        char *settings = argv[i];
        char *endptr;
        long num = strtol(settings, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }
        if (time_out < 0) {
            time_out = num;
        } else {
            count = num;
        }
    }
}
