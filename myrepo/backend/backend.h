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


struct idd_request {
	int data_direction;
	unsigned long nbytes;
	unsigned long offset;
	unsigned long seq_no;
	struct work_struct work;
	void *priv_data;
};

struct idd_response {
	int                     res;
	int op;
	unsigned long seq_no;
	void *priv_data;
};

DEFINE_RING_TYPES(idd, struct idd_request, struct idd_response);

  
typedef struct backend_info {
	struct idd_back_ring main_ring;
	struct idd_back_ring data_ring;
//        syscall_handler_data_t  *list;
//        syscall_handler_data_t  *next_data;
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
} backend_info_t;

typedef struct idd_connect{
	uint32_t domid;
	uint32_t main_ring_gref;
	uint32_t data_ring_gref;
	uint32_t ring_port;
	uint32_t data_port;
}idd_connect_t;

#endif
