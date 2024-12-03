#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dotenv.h"

void load_env(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening .env file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        char *delimiter = strchr(line, '=');
        if (delimiter) {
            *delimiter = '\0';
            const char *key = line;
            const char *value = delimiter + 1;
            setenv(key, value, 1); // Set environment variable
        }
    }
    fclose(file);
}
