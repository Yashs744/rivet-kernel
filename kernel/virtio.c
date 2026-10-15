#include "kernel/virtio.h"
#include "kernel/memory.h"
#include "kernel/panic.h"

/*
 * Virtio
 * Each helper turns “base address + offset” into a pointer and reads or writes through it.
 * volatile prevents the compiler from optimizing the operations.
 */
struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
uint64_t blk_capacity;

uint32_t virtio_reg_read32(unsigned offset) {
    return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}

uint64_t virtio_reg_read64(unsigned offset) {
    return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value) {
    *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value) {
    // sets bits without clearing the others
    // read the current value, OR in the new bit, write it back.
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

struct virtio_virtq *virtq_init(unsigned index) {
    // allocating the queue
    paddr_t paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq *vq = (struct virtio_virtq *) paddr;

    // remembers which queue this is,
    vq->queue_index = index;
    // a shortcut pointer to the device-written counter used.index
    vq->used_index = (volatile uint16_t *) &vq->used.index;

    // Virtqueue Configuration as per the specs
    // 1. Write the virtqueue index to queue_select.
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);

    // 2. Read the virtqueue size from queue_size.  This controls how big the virtqueue is.
    // If this field is 0, the virtqueue does not exist.
    uint32_t max_queue_size = virtio_reg_read64(VIRTIO_REG_QUEUE_NUM_MAX);
    if (max_queue_size == 0)
        PANIC("VIRTIO: Queue %d does not exist", index);
    if (VIRTQ_ENTRY_NUM > max_queue_size)
        PANIC("VIRTIO: Queue too large: %d > %d", VIRTQ_ENTRY_NUM, max_queue_size);

    // 3. Select a smaller virtqueue size and write it to queue_size.
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);

    // 4. Allocate and zero Descriptor Table, Available and Used rings for the
    // virtqueue in contiguous physical memory, on a 4096 byte alignment
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, paddr / PAGE_SIZE);

    return vq;
}

void virtio_blk_init(void) {
    // Sanity Checks
    if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
        PANIC("VIRTIO: invalid magic number!");
    if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
        PANIC("VIRTIO: invalid version!");
    if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)  // Device ID is 2: it’s a disk
        PANIC("VIRTIO: invalid device id!");

    // The initialization process as described in the specs
    // 1. RESET the device
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
    // 2. Set the ACKNOWLEDGE status bit: "I see you"
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
    // 3. Set the DRIVER status bit: "I know how to drive you"
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
    // 4. i. Tell the device our PAGE_SIZE
    virtio_reg_write32(VIRTIO_REG_PAGE_SIZE, PAGE_SIZE);
    // 4. ii. Initialize the Queue
    blk_request_vq = virtq_init(0);
    // 5. Agree on features	Pick optional extras -- Skipped
    // 6. Set the DRIVER_OK status bit: “Setup done, start working”
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

    // Reading the disk’s capacity. Starting at offset 0x100, each device type has its own config area.
    // For a virtio disk, the first field is the capacity, measured in 512-byte sectors.
    // Multiplying by SECTOR_SIZE converts it to bytes.
    blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
    // printf("VIRTIO: Block size is %d\n", (int) blk_capacity);

    // Allocate the request buffer i.e region to store requests on the device
    blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
    blk_req = (struct virtio_blk_req *) blk_req_paddr;


}

/* Sending I/0 Requests */

void virtq_kick(struct virtio_virtq *vq, int descriptor_index) {
    // 1. Post the chain’s head on the available ring. avail.index % 16
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = descriptor_index;

    // 2. Bump up the counter, tell the device "one more request is available"
    vq->avail.index++;

    // 3. Memory Barrier: every memory write before it must be completed and
    // visible before any memory access after it.
    __sync_synchronize();

    // 4. Ring the doorbell.
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);

    // 5. Record what to wait for
    vq->last_used_index++;
}

bool virtq_is_busy(struct virtio_virtq *vq) {
    /*
     * The driver compares what it expects (last_used_index) with what the device
     * has reported (used.index, read through the volatile pointer)
     *
     * Example:
     *  moment                          last_used_index   used.index   busy?
     *  ─────────────────────────────────────────────────────────────────────
     *  before the first request             0               0          no
     *  just after virtq_kick                1               0          YES
     *  device finishes, bumps used.index    1               1          no  → done
     *  after the second kick                2               1          YES
     *  device finishes                      2               2          no  → done
     */
    return vq->last_used_index != *vq->used_index;
}

void read_write_disk(void *buf, unsigned sector, int is_write) {
    /*
     * The whole round trip
     *  kernel                     RAM (shared)               device (QEMU)
     *  │ fill blk_req           ┌────────────┐
     *  │───────────────────────►│ blk_req    │
     *  │ fill descs[0..2]       │ descs      │
     *  │───────────────────────►│            │
     *  │ avail.ring[0] = 0      │ avail      │
     *  │ avail.index = 1        │            │
     *  │───────────────────────►│            │
     *  │ fence                  │            │
     *  │ doorbell (QUEUE_NOTIFY)│            │
     *  │──────────────────────────────────────────────────────────► wakes up
     *  │                        │            │◄──── reads avail, descs, header
     *  │ spin on used.index...  │            │◄──── writes data + status
     *  │                        │ used       │◄──── used.index = 1
     *  │◄── used.index changed  └────────────┘
     *  │ check status, copy data out
     */

    // 1. Check if the sector exists or not
    if (sector >= blk_capacity / SECTOR_SIZE) {
        printf("VIRTIO: tried to read/write sector=%d, but capacity is %d\n",
            sector, (int) blk_capacity / SECTOR_SIZE);
        return;
    }

    // 2. Fill in the blq_req (type, sector, and data id writing)
    blk_req->sector = sector;
    blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    if (is_write)
        memcpy(blk_req->data, buf, SECTOR_SIZE);

    // 3. Build a 3-descriptor chain pointing into blk_req
    struct virtio_virtq *vq = blk_request_vq;

    // Device READS (what to do)
    vq->descs[0].addr = blk_req_paddr;
    vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
    vq->descs[0].next = 1;

    // Device WRITES (the disk data)
    vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[1].len = SECTOR_SIZE;
    vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    vq->descs[1].next = 2;

    // Device WRITES (success / error)
    vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[2].len = sizeof(uint8_t);
    vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

    // 4. Put the chain on the available ring + ring the doorbell that is
    // Notify the device that there is a new request.
    virtq_kick(vq, 0);

    // 5. Wait until the device says it is done
    while (virtq_is_busy(vq))
        ;    // busy-waiting (or polling)

    // 6. Check the status byte
    if (blk_req->status != 0) {
        printf("VIRTIO: failed to read/write sector=%d, status=%d\n", sector, blk_req->status);
        return;
    }

    // 7. if reading, copy the data out to the caller's buffer
    if (!is_write)
        memcpy(buf, blk_req->data, SECTOR_SIZE);
}
