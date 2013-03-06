#ifndef _SYSCALL_BACKEND_H
#define _SYSCALL_BACKEND_H      1


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include<linux/module.h>

#include <linux/init.h>
#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <xen/grant_table.h>
#include <xen/interface/io/ring.h>

#include <linux/wait.h>

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
	int res;
	int op;
	unsigned long seq_no;
	void *priv_data;
};

DEFINE_RING_TYPES(idd, struct idd_request, struct idd_response);

  
typedef struct idd_irq_info {
	struct idd_front_ring main_ring;
	struct idd_front_ring data_ring;
//        idd_shared_data_t *main;
//        struct vm_struct *main_area;
	struct vm_struct *main_ring_area;
	struct vm_struct *data_ring_area;
//	grant_handle_t main_handle;
 	grant_handle_t main_ring_handle;
	grant_handle_t data_ring_handle;
//	int main_irq;
	int ring_irq;
	uint32_t domid;
	atomic_t counter;
	atomic_t notify_counter;
	int request_avail;
	uint32_t state;
	uint32_t gref;
	uint32_t expand_tgid;
	uint32_t expand_gref;
	wait_queue_head_t queue;
	wait_queue_head_t request_queue;
	struct semaphore state_sem;
	struct semaphore rw_sem;
	struct semaphore notify_sem;
	struct spinlock notify_lock;
	struct task_struct *request_thread;
	void * io_data_page;
} idd_irq_info_t;


typedef struct idd_connect{
	uint32_t domid;
	uint32_t main_ring_gref;
	uint32_t data_ring_gref;
	uint32_t ring_port;
	uint32_t data_port;
}idd_connect_t;

int is_buffer_empty();

#endif
