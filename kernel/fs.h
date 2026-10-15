#pragma once

#include "lib/common.h"
#include "kernel/virtio.h"

/* File System via tar
 * TAR file structure
 *  +----------------+
 *  |   tar header   |
 *  +----------------+
 *  |   file data    |
 *  +----------------+
 *  |   tar header   |
 *  +----------------+
 *  |   file data    |
 *  +----------------+
 *  |      ...       |
 * Header followed by the data of file one after another.
 *
 */

#define FILES_MAX     6
#define DISK_MAX_SIZE align_up(sizeof(struct file) * FILES_MAX, SECTOR_SIZE)  // align_up(2258, 512) = 2560

// UStar tar header: https://en.wikipedia.org/wiki/Tar_(computing)#UStar_format
struct tar_header {
    char name[100];      // File name (or partial name if split with prefix)
    char mode[8];       // File permissions (octal ASCII)
    char uuid[8];        // User ID (octal ASCII)
    char gid[8];         // Group ID (octal ASCII)
    char size[12];       // File size in bytes (octal ASCII)
    char mtime[12];      // Modification time (octal ASCII, seconds since Unix epoch)
    char checksum[8];    // Header checksum (sum of all 512 bytes treated as unsigned chars)
    char type;           // Type of file ('0' or null for regular, '1' hard link, '2' symlink, '5' directory)
    char linkname[100];  // Target name for linked files (hard or symbolic)
    char magic[6];       // Magic value: "ustar" followed by a null byte ("ustar\0")
    char version[2];     // Version: two ASCII zeros ("00")
    char uname[32];      // Owner user name (null-terminated ASCII)
    char gname[32];      // Owner group name (null-terminated ASCII)
    char devmajor[8];    // Major device number (for char/block devices)
    char devminor[8];    // Minor device number (for char/block devices)
    char prefix[155];    // Pathname prefix (directory path) used to extend file names >100 characters
    char padding[12];    // Unused padding bytes (zero-filled)

    char data[];         // Array pointing to the data area following the header
} __attribute__((__packed__));  // total = 512 bytes

struct file {
    bool in_use;      // Is the file in use or not
    char name[100];   // Name of the file
    char data[1024];  // Contents of the file
    size_t size;      // Size of the file
};                    // total = 1129 bytes
void fs_init(void);
void fs_flush(void);
struct file *fs_lookup(const char *filename);
struct file *fs_readdir(int index);
struct file *fs_create(const char *filename);
