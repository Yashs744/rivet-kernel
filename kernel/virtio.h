#pragma once

#include "lib/common.h"

/*
 * Disk I/O - VIRTIO
 */

// 1. Basic sizes and the device's address
#define SECTOR_SIZE 512              // Disks are read and written in fixed blocks called sectors (traditionally 512).
#define VIRTQ_ENTRY_NUM 16           // the Virtqueue will have 16 descriptors, and each ring 16 slots.
#define VIRTIO_DEVICE_BLK 2          // every Virtio device reports a type number. 2 means "block device" (disk).
#define VIRTIO_BLK_PADDR 0x10001000  // Physical address where the device's registers appear.

// 2. Register Maps: Each VIRTIO_REG_* macro is an offset from 0x10001000.
// Together they describe the device's "control panel"

// Identity
#define VIRTIO_REG_MAGIC          0x00    // always 0x74726976 ("virt")
#define VIRTIO_REG_VERSION        0x04    // 1 = legacy interface; 2 = modern interface
#define VIRTIO_REG_DEVICE_ID      0x08    // 2 = block device

// Features
#define VIRTIO_REG_DEVICE_FEATURES      0x10    // Flags representing features the device supports
#define VIRTIO_REG_DEVICE_FEATURES_SEL  0x14    // 32-bit half DEVICE_FEATURES shows: 0 = bits 0-31, 1 = bits 32-63
#define VIRTIO_REG_DRIVER_FEATURES      0x20    // Flags representing device feats understood & activated by the driver
#define VIRTIO_REG_DRIVER_FEATURES_SEL  0x24    // 32-bit half DRIVER_FEATURES writes

// Setup
#define VIRTIO_REG_PAGE_SIZE         0x28    // we tell it: our pages are 4 KB
#define VIRTIO_REG_QUEUE_SEL         0x30    // we pick which queue to configure
#define VIRTIO_REG_QUEUE_NUM_MAX     0x34    // it tells us: max queue size
#define VIRTIO_REG_QUEUE_NUM         0x38    // we tell it: our queue size (16)
#define VIRTIO_REG_QUEUE_PFN         0x40    // we tell it: where the queue is
#define VIRTIO_REG_QUEUE_READY       0x44    // modern interface
// MODERN VERSION SETUP
#define VIRTIO_REG_QUEUE_DESC_LOW    0x80    // physical address for descriptor table
#define VIRTIO_REG_QUEUE_DESC_HIGH   0x84
#define VIRTIO_REG_DRIVER_DESC_LOW   0x90    // physical address for available ring
#define VIRTIO_REG_DRIVER_DESC_HIGH  0x94
#define VIRTIO_REG_DEVICE_DESC_LOW   0xa0    // physical address for used ring
#define VIRTIO_REG_DEVICE_DESC_HIGH  0xa4

// Operation
#define VIRTIO_REG_QUEUE_NOTIFY   0x50    // new request!
#define VIRTIO_REG_DEVICE_STATUS  0x70    // handshake progress

// Device-specific
#define VIRTIO_REG_DEVICE_CONFIG  0x100   // for a disk: its capacity

// 3. Status bits: a handshake checklist
#define VIRTIO_STATUS_ACK         1   // "I see you"
#define VIRTIO_STATUS_DRIVER      2   // "I know how to drive you"
#define VIRTIO_STATUS_DRIVER_OK   4   // "setup done, ready to go"
#define VIRTIO_STATUS_FEATURES_OK 8

// 4. Descriptor flags and request types
#define VIRTQ_DESC_F_NEXT           1   // Chain continues at `next`
#define VIRTQ_DESC_F_WRITE          2   // Device may write to this buffer
#define VIRTQ_AVAIL_F_NO_INTERRUPT  1   // "don't interrupt me when done"
#define VIRTIO_BLK_T_IN             0   // Read from disk
#define VIRTIO_BLK_T_OUT            1   // Write to disk
#define VIRTIO_F_VERSION_1          32

// The Virtqueue Descriptor Table
struct virtq_desc {
    uint64_t addr;    // Address (guest-physical); Offset 0; 8 bytes
    uint32_t len;     // Length; Offset 8;  4 bytes
    uint16_t flags;   // Flag like VIRTQ_DESC_F_NEXT; Offset 12, 2 bytes
    uint16_t next;    // Next field if flags & NEXT; Offset 14, 2 bytes
} __attribute__((__packed__));  // total: 16 bytes

// The Virtqueue Available Ring
struct virtq_avail {
    uint16_t flags;  // Flag like VIRTQ_AVAIL_F_NO_INTERRUPT
    uint16_t index;  // Where the driver would put the next descriptor entry in the ring
    uint16_t ring[VIRTQ_ENTRY_NUM]; // head descriptor of each request; 2 * 16 = 32 bytes
} __attribute__((__packed__));  // total = 2 + 2 + 32 = 36 bytes


// The Virtqueue Used Ring
struct virtq_used_elem {
    uint32_t id;  // index of start of used descriptor chain
    uint32_t len; // how many bytes the device wrote

} __attribute__((__packed__));  // total = 8 bytes

struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((__packed__));  // total = 2 + 2 + 128 = 132 bytes

/*
 *  The VirtQueue
 *  In the legacy interface, the driver gives the device only one number: the page where the queue starts.
 *  The device then CALCULATES where each part is, using fixed rules from the spec:
 *   - The descriptor table starts at the beginning.
 *   - The available ring comes right after it.
 *   - The used ring starts at the next 4 KB boundary.
 */
struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];  // the actual descriptors (16 bytes each)
    struct virtq_avail avail;  // a ring of available descriptor heads with free-running index.
    struct virtq_used used;    // __attribute__((aligned(PAGE_SIZE))); (LEGACY)

    // driver's private bookkeeping
    int queue_index;                // which queue this is (0 for the disk’s only request queue)
    volatile uint16_t *used_index;  // a pointer to used.index
    uint16_t last_used_index;       // how many completions the driver expects to have seen
} __attribute__((__packed__));

/* The Virtqueue BLK Request: the request header + response status
 * 0x80300000  ┌──────────────────────┐ <-- desc[0]: header, 16 bytes
 *             │ type, reserved,      │
 *             │ sector               │
 * 0x80300010  ├──────────────────────┤ <-- desc[1]: data, 512 bytes
 *             │ data[512]            │
 * 0x80300210  ├──────────────────────┤ <-- desc[2]: status, 1 byte
 *             │ status               │
 *             └──────────────────────┘
 */
struct virtio_blk_req {
    uint32_t type;      // IN (read) or OUT (write)
    uint32_t reserved;  // unused, must exist
    uint64_t sector;    // which sector
    uint8_t data[512];  // the sector's contents
    uint8_t status;     // 0 = OK, set by device
} __attribute__((__packed__));  // total: 529 bytes

void virtio_blk_init(void);
void read_write_disk(void *buf, unsigned sector, int is_write);
