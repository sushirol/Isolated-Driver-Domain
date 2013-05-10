#ifndef _IDD_BACKEND_H
#define _IDD_BACKEND_H      1


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include<linux/module.h>
#include <linux/interrupt.h>

#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <xen/grant_table.h>
#include <xen/interface/io/ring.h>

#include <asm/uaccess.h>   // Needed by segment descriptors
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/mutex.h>

static int blk_init(void);
static void blk_cleanup(void);

#define DOMZERO	0

#define IDD_SERVICE_CONNECT                 0
#define IDD_SERVICE_READY                   1
#define IDD_SERVICE_DISCONNECT              2
#define IDD_SERVICE_REGISTER                3
#define IDD_SERVICE_UNREGISTER              4


#define IDD_SYSID_ONE                               0

#define IDD_SYSID_CURRENT   IDD_SYSID_ONE

#define IDD_MAX_SEGMENTS_PER_REQUEST 11
struct idd_request {
        int data_direction;
//      unsigned long nbytes;
//      unsigned long offset;
        uint8_t nr_segments;
        uint64_t sector_number;
        struct idd_request_segment {
                grant_ref_t gref;       /* reference to I/O buffer frame */
                /* @first_sect: first sector in frame to transfer (inclusive).   */
                /* @last_sect: last sector in frame to transfer (inclusive).     */
                uint8_t     first_sect, last_sect;
        }seg[IDD_MAX_SEGMENTS_PER_REQUEST];
        uint64_t seq_no;
        void *priv_data;
}__attribute__((__packed__));

struct idd_response {
        int res;
        int op;
        unsigned long seq_no;
        void *priv_data;
};


DEFINE_RING_TYPES(idd, struct idd_request, struct idd_response);

struct bd_req {
	unsigned short dev;
	uint64_t nr_sects;
	struct block_device *bdev;
	uint64_t sector_number;
};

struct seg_buf {
//Sushrut : offset and no of sect
	unsigned long buf;
	unsigned int nsec;
};

struct idd_bd {
	struct block_device     *bdev;
	sector_t                size;
};

struct pending_req {
	struct backend_info *priv_d;
	u64 id;
	int nr_pages;
	atomic_t pendcnt;
	unsigned short operation;
	int status;
	struct list_head free_list;
	DECLARE_BITMAP(unmap_seg, IDD_MAX_SEGMENTS_PER_REQUEST);
};

struct persistent_gnt {
	struct page *page;
	grant_ref_t gnt;
	grant_handle_t handle;
	uint64_t dev_bus_addr;
	struct rb_node node;
};
  
typedef struct backend_info {
	struct idd_back_ring main_ring;
	struct idd_back_ring data_ring;
	struct task_struct *main_thread;
	struct task_struct *request_thread;
	uint32_t main_ring_gref;
	uint32_t data_ring_gref;
        int main_irq;
        int ring_irq;
        int main_avail;
        int request_avail;
        struct semaphore main_sem;
        struct semaphore rsp_ring_sem;
        struct semaphore req_ring_sem;
        wait_queue_head_t main_queue;
        wait_queue_head_t request_queue;
        struct spinlock ring_lock;
        struct spinlock list_lock;
	void * io_data_page;
        struct work_struct write_task;
        struct work_struct read_task;
        struct idd_request *rw_req;
	unsigned long id;
	
	wait_queue_head_t wq;
	unsigned int waiting_reqs;
	struct idd_bd bd;
	struct pending_req *pending_reqs;
	/* List of all 'pending_req' available */
	struct list_head pending_free;
	spinlock_t pending_free_lock;
	wait_queue_head_t pending_free_wq;
	struct page **pending_pages;
	grant_handle_t *pending_grant_handles;
	atomic_t refcnt;
	spinlock_t blk_ring_lock;
	unsigned int persistent_gnt_c;
	struct rb_root persistent_gnts;

} backend_info_t;

typedef struct idd_connect{
	uint32_t domid;
	uint32_t main_ring_gref;
	uint32_t data_ring_gref;
	uint32_t ring_port;
	uint32_t data_port;
}idd_connect_t;

#define xen_idd_get(_b) (atomic_inc(&(_b)->refcnt))
#define xen_idd_put(_b) (atomic_dec_and_test(&(_b)->refcnt))
#define pending_page(req, seg) pending_pages[vaddr_pagenr(req, seg)]
#define pending_handle(_req, _seg) \
	(backend.pending_grant_handles[vaddr_pagenr(_req, _seg)])

#define KERNEL_SECTOR_SHIFT 9
#define IDD_INVALID_HANDLE (~0)

static int xen_idd_reqs = 64;

#endif
