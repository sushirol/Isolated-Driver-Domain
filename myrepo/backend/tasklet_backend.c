#include "backend.h"

#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/init_task.h>
#include <linux/fdtable.h>
#include <linux/spinlock.h>
#include <linux/cred.h>
#include <linux/security.h>
#include <xen/grant_table.h>
#include <xen/page.h>
#include <xen/events.h>
#include <asm/xen/hypercall.h>
#include <linux/delay.h>
#include <xen/xenbus.h>
#include <xen/events.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple block device");

static backend_info_t backend;

struct file *file;
int f;
static int count=0;
void process_write_req(unsigned long int data){
	struct idd_request* req = (struct idd_request*)data;

	struct idd_response *rsp;
	int err=-1, notify;
#if 1
        mm_segment_t old_fs;
        loff_t pos = 0;

	printk("direction %d \n", req->data_direction);
        printk("fvalue %p\n",file);
	pos = req->offset;
	smp_mb();
	printk("direction %d pos %lld\n", req->data_direction,pos);
	if(req->data_direction == 1){
	        old_fs = get_fs();
		smp_mb();
	        set_fs(KERNEL_DS);
	        err = do_sync_write(file, (const char*)backend.io_data_page, req->nbytes, &pos);
		printk("write %s\n", (const char *)backend.io_data_page);
		set_fs(old_fs);
	}
#endif
	down(&backend.rsp_ring_sem);
	rsp = RING_GET_RESPONSE(&backend.main_ring, backend.main_ring.rsp_prod_pvt);
	rsp->res = err;
	rsp->seq_no = req->seq_no;
	rsp->op = req->data_direction;
	rsp->priv_data = req->priv_data;
	backend.main_ring.rsp_prod_pvt++;
	smp_mb();
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&backend.main_ring, notify);
	notify_remote_via_irq(backend.ring_irq);
	printk("interrupt handled at backend %lu. sending interrupt to frontend %lu!\n",req->seq_no, rsp->seq_no);
	up(&backend.rsp_ring_sem);
}

void process_read_req(unsigned long int data){
	struct idd_request* req = (struct idd_request*)data;

	struct idd_response *rsp;
	int err=-1, notify;
#if 1
        mm_segment_t old_fs;
        loff_t pos = 0;

	pos = req->offset;
	smp_mb();
	printk("direction %d pos %lld\n", req->data_direction,pos);

	if(req->data_direction == 0){
	        old_fs = get_fs();
		smp_mb();
        	set_fs(KERNEL_DS);
		err = do_sync_read(file, backend.io_data_page, req->nbytes, &pos);
		set_fs(old_fs);
//		printk(KERN_INFO "buf:%s\n",backend.io_data_page);
		count--;
		smp_mb();
	}
#endif
	down(&backend.rsp_ring_sem);
	rsp = RING_GET_RESPONSE(&backend.main_ring, backend.main_ring.rsp_prod_pvt);
	rsp->res = err;
	rsp->seq_no = req->seq_no;
	rsp->op = req->data_direction;
	rsp->priv_data = req->priv_data;
	backend.main_ring.rsp_prod_pvt++;
	smp_mb();
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&backend.main_ring, notify);
	notify_remote_via_irq(backend.ring_irq);
	printk("interrupt handled at backend %lu. sending interrupt to frontend %lu!\n",req->seq_no, rsp->seq_no);
	up(&backend.rsp_ring_sem);
}


static irqreturn_t irq_ring_interrupt(int irq, void *dev_id)
{
	struct idd_request *req;
	struct idd_response *rsp;
	int notify;
	RING_IDX rc;
	struct tasklet_struct *task;

	down(&backend.req_ring_sem);
	rc = backend.main_ring.req_cons;
	smp_mb();
	req = RING_GET_REQUEST(&backend.main_ring,rc);
	if(rc == -1){
		printk("get request failed");
		return IRQ_HANDLED;
	} 
	backend.main_ring.req_cons = ++rc;
	backend.rw_req = req;
	smp_mb();
	up(&backend.req_ring_sem);
	printk("got from frontend %lu!\n", req->seq_no);

#if 1
	if(req->data_direction == 1){
		task = (struct tasklet_struct *)kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);
//		tasklet_init(&req->tsklt, process_write_req, (unsigned long int)req);
		tasklet_init(task, process_write_req, (unsigned long int)req);
		printk("inserted in queue seq_no %lu!\n",req->seq_no);
	}else{
		task = (struct tasklet_struct *)kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);
//		tasklet_init(&req->tsklt, process_read_req, (unsigned long int)req);
		tasklet_init(task, process_read_req, (unsigned long int)req);
		printk("inserted in queue seq_no %lu!\n",req->seq_no);
	}
#else
	rsp = RING_GET_RESPONSE(&backend.main_ring, backend.main_ring.rsp_prod_pvt);
	rsp->res = 1;
	rsp->seq_no = req->seq_no;
	rsp->op = req->data_direction;
	rsp->priv_data = req->priv_data;
	backend.main_ring.rsp_prod_pvt++;
	smp_mb();
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&backend.main_ring, notify);
	notify_remote_via_irq(backend.ring_irq);
	printk("interrupt handled at backend %lu. sending interrupt to frontend %lu!\n",req->seq_no, rsp->seq_no);
#endif
	tasklet_schedule(task);
        return IRQ_HANDLED;
}

static void *idd_alloc_shared(uint32_t *gref)
{
	struct page *page = alloc_page(GFP_KERNEL | __GFP_ZERO);
//	struct page *page = alloc_pages(GFP_KERNEL | __GFP_ZERO, 3);
	if (!page)
		return NULL;
	*gref = gnttab_grant_foreign_access(DOMZERO, pfn_to_mfn(page_to_pfn(page)), 0);
	if ((int32_t) *gref < 0) {
		__free_page(page);
		return NULL;
	}
	return (void *) pfn_to_kaddr(page_to_pfn(page));
}

static void idd_free_shared(uint32_t gref, void *addr)
{
	/* It will execute free_page() as well */
	printk(KERN_WARNING "Foreign Access: %x\n", gnttab_query_foreign_access(gref));
	gnttab_end_foreign_access(gref, 0, (unsigned long) addr);
}


static int blk_init(void)
{
	struct evtchn_alloc_unbound ring_alloc;
	struct evtchn_close close;
	idd_connect_t data;
	int err=0;
	struct idd_sring *sring;

        mm_segment_t old_fs;
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        f = do_sys_open(AT_FDCWD, (const char*)"/root/sdb", O_RDWR, 0);
        printk("fvalue %d\n",f);
//        f = do_sys_open(AT_FDCWD, (const char*)"/dev/sdb", O_RDWR, 0);
        f = do_sys_open(AT_FDCWD, (const char*)"/dev/ramd", O_RDWR, 0);
        printk("fvalue %d\n",f);
        file = fget(f);
        printk("file %p\n",file);
	set_fs(old_fs);

	sema_init(&backend.rsp_ring_sem,1);
	sema_init(&backend.req_ring_sem,1);

/********************** EVERYTHING BELOW IS A RING BUFFER *******************/

	printk("Inseting module\n");

	sring = idd_alloc_shared(&backend.main_ring_gref);
	if (unlikely(sring == NULL)) {
		err = -ENOMEM;
		goto end;
	}
	SHARED_RING_INIT(sring);
//	BACK_RING_INIT(&backend.main_ring, sring, 8 * PAGE_SIZE);
	BACK_RING_INIT(&backend.main_ring, sring, PAGE_SIZE);

	data.domid = 0;
	data.main_ring_gref = backend.main_ring_gref;
        printk("DEBUG : main_ring_gref %u %u\n",data.main_ring_gref,backend.main_ring_gref);

	//updating connection data info

/********************* SHARED IO DATA PAGE *****************************/

	backend.io_data_page = (void *)idd_alloc_shared(&backend.data_ring_gref);	
	data.data_ring_gref = backend.data_ring_gref;
	smp_mb();
        printk("DEBUG : data_ring_gref %u %u\n",data.data_ring_gref,backend.data_ring_gref);
		
/********************** EVERYTHING BELOW IS A EVENT CHANNEL *******************/
	backend.ring_irq = -1;
	ring_alloc.dom = DOMID_SELF;
	ring_alloc.remote_dom = DOMZERO;
	smp_mb();

	err = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &ring_alloc);
	if (unlikely(err != 0))
		goto end2; //TODO

	err = bind_evtchn_to_irqhandler(ring_alloc.port, irq_ring_interrupt,
		0, "syscall_backend_irq_ring", &backend);

	if (unlikely(err < 0))
		goto end3; //TODO

	backend.ring_irq = err;
	data.ring_port = ring_alloc.port;
	smp_mb();
	printk("port %u domid %x\n", ring_alloc.port, DOMID_SELF);

/********************************************************************/

	err = HYPERVISOR_idd_service_op(IDD_SERVICE_REGISTER,
		0, &data);
	if (unlikely(err != 0))
		goto end4; //TODO

	init_waitqueue_head(&backend.request_queue);

/*	backend.request_thread = kthread_create(process_request, &,
                "thread_name");
*/

	return 0;

end4:
	unbind_from_irqhandler(backend.ring_irq, &backend);
end3:
	close.port = ring_alloc.port;
	smp_mb();
	if (HYPERVISOR_event_channel_op(EVTCHNOP_close, &close) != 0)
		BUG();
end2:
	idd_free_shared(backend.main_ring_gref, backend.main_ring.sring);
end:
	return err;
}

static void blk_cleanup(void)
{
	printk("Removing module\n");
        fput(file);
        sys_close(f);
	unbind_from_irqhandler(backend.ring_irq, &backend);
}

module_init(blk_init);
module_exit(blk_cleanup);
