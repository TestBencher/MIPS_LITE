#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 1024
uint32_t memory[MEMORY_SIZE];  // Memory array

void file_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("The file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char line[20];
    int index = 0;

    while (fgets(line, sizeof(line), file) != NULL && index < MEMORY_SIZE) {
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);
        memory[index++] = word;
    }

    fclose(file);
    printf("Memory image loaded successfully. %d words read.\n", index);
}

int main() {
    char filename[100];

    printf("Enter the memory image filename: ");
    if (fgets(filename, sizeof(filename), stdin) != NULL) {
        // Remove trailing newline character if present
        filename[strcspn(filename, "\n")] = '\0';

        file_read(filename);
    } else {
        printf("Error reading filename.\n");
    }

    return 0;
}
