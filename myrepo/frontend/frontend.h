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


#define IDD_MAX_SEGMENTS_PER_REQUEST 11
struct idd_request {
	int data_direction;
//	unsigned long nbytes;
//	unsigned long offset;
	uint8_t nr_segments;
	uint64_t sector_number;
	struct idd_request_segment {
		grant_ref_t gref;	/* reference to I/O buffer frame */
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

#define IDD_RING_SIZE __CONST_RING_SIZE(idd, PAGE_SIZE)
#define IDD_BOUNCE_ANY          (-1ULL)
 
struct idd_shadow {
	struct idd_request req;
	struct request *request;
	unsigned long frame[IDD_MAX_SEGMENTS_PER_REQUEST];
	struct grant *grants_used[IDD_MAX_SEGMENTS_PER_REQUEST];
}; 
 
typedef struct idd_irq_info {
	struct idd_front_ring main_ring;
	struct idd_front_ring data_ring;
	struct vm_struct *main_ring_area;
	struct vm_struct *data_ring_area;
 	grant_handle_t main_ring_handle;
	grant_handle_t data_ring_handle;
//	int main_irq;
	int ring_irq;
	uint32_t domid;
	struct semaphore rw_sem;
	struct semaphore notify_sem;
//	void * io_data_page;
	struct scatterlist sg[IDD_MAX_SEGMENTS_PER_REQUEST];
	unsigned long id;
	struct request_queue *rq;
	struct llist_head persistent_gnts;
	unsigned int persistent_gnts_c;
	struct idd_shadow shadow[IDD_RING_SIZE];
	unsigned long shadow_free;
	struct gnttab_free_callback callback;
	struct work_struct work;
	spinlock_t io_lock;
} idd_irq_info_t;


typedef struct idd_connect{
	uint32_t domid;
	uint32_t main_ring_gref;
	uint32_t data_ring_gref;
	uint32_t ring_port;
	uint32_t data_port;
}idd_connect_t;

#endif
