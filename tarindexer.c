// written by OpenAI o1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BLOCK_SIZE 512

// Function to check if a block is all zeros (used to detect end-of-archive)
int is_zero_block(unsigned char *block) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (block[i] != 0) {
            return 0;
        }
    }
    return 1;
}

// Function to read a block safely
int safe_read(FILE *file, unsigned char *buffer) {
    size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, file);
    if (bytes_read == BLOCK_SIZE) {
        return 1; // Success
    } else if (bytes_read == 0 && feof(file)) {
        return 0; // EOF reached
    } else {
        fprintf(stderr, "Error reading block: %s\n", strerror(errno));
        return -1; // Error
    }
}

int main(int argc, char *argv[]) {
    FILE *tar_file = stdin; // Default to stdin

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [tar_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            // Print usage instructions
            fprintf(stdout, "Usage: %s [tar_file]\n", argv[0]);
            fprintf(stdout, "Reads a tar file from a specified file or stdin and outputs file names, byte offsets, and sizes.\n");
            exit(EXIT_SUCCESS);
        } else if (argv[1][0] == '-') {
            // Unknown option
            fprintf(stderr, "Unknown option: %s\n", argv[1]);
            fprintf(stderr, "Usage: %s [tar_file]\n", argv[0]);
            exit(EXIT_FAILURE);
        } else {
            tar_file = fopen(argv[1], "rb");
            if (!tar_file) {
                fprintf(stderr, "Error opening file '%s': %s\n", argv[1], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    unsigned char header[BLOCK_SIZE];
    char long_name[1024] = {0}; // Buffer for GNU long names
    long offset = 0;
    int consecutive_zero_blocks = 0;

    while (1) {
        int result = safe_read(tar_file, header);
        if (result == 0) {
            // EOF reached
            break;
        } else if (result == -1) {
            // Error reading
            exit(EXIT_FAILURE);
        }

        // Check for two consecutive zero blocks indicating end-of-archive
        if (is_zero_block(header)) {
            consecutive_zero_blocks++;
            if (consecutive_zero_blocks == 2) {
                break; // End of archive
            } else {
                offset += BLOCK_SIZE;
                continue;
            }
        } else {
            consecutive_zero_blocks = 0;
        }

        // Extract the typeflag
        char typeflag = header[156];

        // Handle GNU long name
        if (typeflag == 'L') {
            // This is a long name entry
            // Read the size of the long name
            char size_field[13]; // 12 bytes for size + null terminator
            memcpy(size_field, header + 124, 12);
            size_field[12] = '\0'; // Ensure null-terminated string

            // Convert size from octal string to integer
            char *endptr;
            long long_name_size = strtol(size_field, &endptr, 8);
            if (endptr == size_field || long_name_size <= 0 || long_name_size >= sizeof(long_name)) {
                fprintf(stderr, "Invalid long name size at offset %ld\n", offset);
                exit(EXIT_FAILURE);
            }

            // Read the long name data blocks
            long num_blocks = (long_name_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
            size_t bytes_to_read = num_blocks * BLOCK_SIZE;
            if (fread(long_name, 1, bytes_to_read, tar_file) != bytes_to_read) {
                fprintf(stderr, "Error reading long name data at offset %ld\n", offset);
                exit(EXIT_FAILURE);
            }
            long_name[long_name_size] = '\0'; // Ensure null-terminated string

            // Update offset
            offset += BLOCK_SIZE + bytes_to_read;

            // After the 'L' header and long name data, the next header is the actual file header
            result = safe_read(tar_file, header);
            if (result != 1) {
                fprintf(stderr, "Error reading header after long name at offset %ld\n", offset);
                exit(EXIT_FAILURE);
            }
            offset += BLOCK_SIZE;
        } else {
            // No long name, clear the long_name buffer
            long_name[0] = '\0';
            offset += BLOCK_SIZE;
        }

        // Extract the file name
        char name[101] = {0};
        memcpy(name, header, 100);

        // Extract the prefix (for ustar format)
        char prefix[156] = {0}; // 155 bytes + null terminator
        memcpy(prefix, header + 345, 155);

        // Combine prefix and name if necessary
        char full_name[512] = {0};
        if (prefix[0]) {
            snprintf(full_name, sizeof(full_name), "%s/%s", prefix, name);
        } else {
            strncpy(full_name, name, sizeof(full_name) - 1);
        }

        // If we have a long name from GNU tar extension, use it
        if (long_name[0]) {
            strncpy(full_name, long_name, sizeof(full_name) - 1);
        }

        // Ensure null termination
        full_name[sizeof(full_name) - 1] = '\0';

        // Extract file size (stored as an octal string)
        char size_field[13]; // 12 bytes for size + null terminator
        memcpy(size_field, header + 124, 12);
        size_field[12] = '\0'; // Ensure null-terminated string

        // Convert size from octal string to integer
        char *endptr;
        long file_size = strtol(size_field, &endptr, 8);
        if (endptr == size_field) {
            fprintf(stderr, "Invalid size for file '%s' at offset %ld\n", full_name, offset);
            exit(EXIT_FAILURE);
        }

        // Record the position just after the header block (start of file data)
        long file_data_offset = offset;

        // Output the file name, data offset, and size
        printf("%s %ld %ld\n", full_name, file_data_offset, file_size);

        // Calculate the number of blocks the file occupies
        long num_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        // Skip over the file data blocks
        if (num_blocks > 0) {
            if (fseek(tar_file, num_blocks * BLOCK_SIZE, SEEK_CUR) != 0) {
                fprintf(stderr, "Error seeking past file data for '%s': %s\n", full_name, strerror(errno));
                exit(EXIT_FAILURE);
            }
            offset += num_blocks * BLOCK_SIZE;
        }
    }

    if (tar_file != stdin) {
        fclose(tar_file);
    }

    return 0;
}
