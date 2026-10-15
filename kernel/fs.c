#include "fs.h"
#include "kernel/fs.h"
#include "kernel/panic.h"

/* File System */
struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];  // 2560 bytes


int octal2integer(char *octal, int len) {
    /* Convert Octal to Integer
     * Example (for "101")
     *  Initial state: decimal = 0
     *   - i = 0 (character '1'): decimal = (0 * 8) + 1 = 1
     *   - i = 1 (character '0'): decimal = (1 * 8) + 0 = 8
     *   - i = 2 (character '1'): decimal = (8 * 8) + 1 = 65
     */
    int decimal = 0;

    for (int i = 0; i < len; i++) {
        // valid octal digit b/w 0 and 7
        if (octal[i] < '0' || octal[i] > '7')
            break;

        decimal = decimal * 8 + (octal[i] - '0');
    }

    return decimal;

}

// Read the whole disk into memory
void fs_init(void) {
    // Load the disk image into temporary buffers
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        // &disk[sector * SECTOR_SIZE] = disk + (sector * SECTOR_SIZE)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);

    // Copy the content from temporary buffer into files
    unsigned offset = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        struct tar_header *header = (struct tar_header *) &disk[offset];

        if (header->name[0] == '\0')
            break;

        if (strcmp(header->magic, "ustar") != 0)
            PANIC("FILE SYSTEM: invalid tar header magic:\"%s\"", header->magic);

        // Get the size of the file in decimal (tar's is in octal format)
        int filesz = octal2integer(header->size, sizeof(header->size));

        struct file *file = &files[i];
        file->in_use = true;
        strcpy(file->name, header->name);
        memcpy(file->data, header->data, filesz);
        file->size = filesz;

        // printf("FILE SYSTEM: file=%s, size=%d\n", file->name, file->size);

        // Update the offset
        offset += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
    }
}

// Writing to the disk
void fs_flush(void) {

    memset(disk, 0, sizeof(disk));

    unsigned offset = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        struct file *file = &files[i];

        if (!file->in_use)
            continue;

        struct tar_header *header = (struct tar_header *) &disk[offset];
        memset(header, 0, sizeof(* header));
        strcpy(header->name, file->name);
        strcpy(header->mode, "000644");
        strcpy(header->magic, "ustar");
        strcpy(header->version, "00");
        header->type = '0';

        // Convert back the size from decimal to octal
        int filesz = file->size;
        for (int j = sizeof(header->size); j > 0; j--) {
            header->size[j-1] = (filesz % 8) + '0';
            filesz /= 8;
        }

        // Calculate the checksum
        int checksum = ' ' * sizeof(header->checksum);
        for (unsigned j = 0; j < sizeof(struct tar_header); j++)
            checksum += (unsigned char) disk[offset + j];

        for (int j = 5; j >= 0; j--) {
            header->checksum[j]  = (checksum % 8) + '0';
            checksum /= 8;
        }

        // Copy the file data
        memcpy(header->data, file->data, file->size);

        // Update the offset
        offset += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
    }

    // Write the `disk` buffer into the virtio-blk.
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);

    // printf("FILE SYSTEM: Wrote %d bytes to disk\n", (int) sizeof(disk));
}

// File System - syscall
struct file *fs_lookup(const char *filename) {
    for (int i = 0; i < FILES_MAX; i++) {
        struct file *file = &files[i];
        if (!strcmp(file->name, filename))
            return file;
    }

    return NULL;
}

// Returns the index-th file that is in use, or NULL
struct file *fs_readdir(int index) {
    int n = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        if (!files[i].in_use)
            continue;

        if (n == index)
            return &files[i];

        n++;
    }

    return NULL;
}

// Returns the index-th file that is in use, or NULL
struct file *fs_create(const char *filename) {
    int len = strlen(filename);

    if (len > (int) sizeof(files[0].name))
        return NULL;

    for (int i = 0; i < FILES_MAX; i++) {

        // find the unused file block
        if (!files[i].in_use) {
            struct file *file = &files[i];

            // Clear out the block
            memset(file, 0, sizeof(*file));
            file->in_use = true;
            strcpy(file->name, filename);
            file->size = 0;
            return file;
        }
    }

    return NULL;
}
