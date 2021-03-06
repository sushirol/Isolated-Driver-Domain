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
#include <linux/freezer.h>
#include <linux/blkdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple block device");

static backend_info_t backend;

int sleep_cond=0;

static inline int vaddr_pagenr(struct pending_req *req, int seg){
	int ret;
	ret = (req - backend.pending_reqs) * IDD_MAX_SEGMENTS_PER_REQUEST + seg;
	return ret;
}

static inline unsigned long vaddr(struct pending_req *req, int seg)
{
	unsigned long pfn = page_to_pfn(backend.pending_page(req, seg));
	return (unsigned long)pfn_to_kaddr(pfn);
}

static void free_req(struct pending_req *req){
        unsigned long flags;
        int was_empty;

        spin_lock_irqsave(&backend.pending_free_lock, flags);
	was_empty = list_empty(&backend.pending_free);
	list_add(&req->free_list, &backend.pending_free);
	spin_unlock_irqrestore(&backend.pending_free_lock, flags);
	if (was_empty)
		wake_up(&backend.pending_free_wq);
}

static void make_response(backend_info_t *be, u64 id, unsigned short op, int st){
	struct idd_response resp;
	unsigned long flags;
	int notify;
  int status;

	resp.op = op;
	resp.seq_no = id;
	resp.priv_data = NULL;
	resp.res = st;

	spin_lock_irqsave(&be->blk_ring_lock, flags);

	memcpy(RING_GET_RESPONSE(&be->main_ring, backend.main_ring.rsp_prod_pvt),&resp, sizeof(resp));
	be->main_ring.rsp_prod_pvt++;

	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&be->main_ring, notify);
//  printk("remote thread status %d \n",atomic_read(&(be->main_ring.sring->rsp_status)));
  status = atomic_read(&(be->main_ring.sring->rsp_status));
  smp_mb();
  if(status == SLEEPING){
//    printk("Frontend response handling thread was sleeping %d. Sending interrupt\n",status);
    notify_remote_via_irq(be->ring_irq);
  }
	spin_unlock_irqrestore(&be->blk_ring_lock, flags);

}

static void unmap_pages(struct pending_req *req){
	struct page *pages[IDD_MAX_SEGMENTS_PER_REQUEST];
	struct gnttab_unmap_grant_ref unmap[IDD_MAX_SEGMENTS_PER_REQUEST];
	unsigned int i, invcount = 0;
	grant_handle_t handle;
	int ret;

	for (i = 0; i < req->nr_pages; i++) {
		handle = pending_handle(req, i);
		if(handle == IDD_INVALID_HANDLE)
			continue;
		gnttab_set_unmap_op(&unmap[invcount], vaddr(req, i),
				GNTMAP_host_map, handle);
		pending_handle(req, i) = IDD_INVALID_HANDLE;
		pages[invcount] = virt_to_page(vaddr(req, i));
		invcount++;
	}
	
	ret = gnttab_unmap_refs(unmap, pages, invcount, 0);
	BUG_ON(ret);
}

static void __end_block_io_op(struct pending_req *pending_req, int error){
        if (error) {
                printk("Buffer not up-to-date at end of operation, error=%d\n", error);
                pending_req->status = -1;
        }
        if (atomic_dec_and_test(&pending_req->pendcnt)) {
		unmap_pages(pending_req);
                make_response(pending_req->priv_d, pending_req->id,
                                pending_req->operation, pending_req->status);
                xen_idd_put(pending_req->priv_d);
        	free_req(pending_req);
        }
}

static void end_block_io_op(struct bio *bio, int error)
{
        __end_block_io_op(bio->bi_private, error);
        bio_put(bio);
}


static struct pending_req *alloc_req(void){
	struct pending_req *req = NULL;
	unsigned long flags;

	spin_lock_irqsave(&backend.pending_free_lock, flags);
	if (!list_empty(&backend.pending_free)) {
		req = list_entry(backend.pending_free.next, struct pending_req,
			free_list);
		list_del(&req->free_list);
	}
	spin_unlock_irqrestore(&backend.pending_free_lock, flags);
	return req;
}

static int map_pages_to_req(struct idd_request *req, 
				struct pending_req *pending_req, 
				struct seg_buf seg[]){

	struct gnttab_map_grant_ref map[IDD_MAX_SEGMENTS_PER_REQUEST];
	int i;
	int nseg = req->nr_segments;
	int ret = 0;

	for (i = 0; i < nseg; i++) {
		uint32_t flags;

		flags = GNTMAP_host_map ;
		if (pending_req->operation != 0)
			flags |= GNTMAP_readonly;

		gnttab_set_map_op(&map[i], vaddr(pending_req, i),
			flags, req->seg[i].gref, DOMZERO);
	}

	ret = gnttab_map_refs(map, NULL, &backend.pending_page(pending_req, 0), nseg);
	BUG_ON(ret);

	for (i = 0; i < nseg; i++) {

		if (unlikely(map[i].status != 0)) {
			printk("invalid buffer -- could not remap it\n");
			map[i].handle = IDD_INVALID_HANDLE;
			ret |= 1;
		}
		pending_handle(pending_req, i) = map[i].handle;
		if(ret)
			continue;


		seg[i].buf = map[i].dev_bus_addr | 
			(req->seg[i].first_sect << KERNEL_SECTOR_SHIFT);
	}
	return ret;
}

static int dispatch_rw_block_io(backend_info_t *be,
				struct idd_request *req,
				struct pending_req *pending_req){
	struct bd_req breq;
	struct seg_buf seg[IDD_MAX_SEGMENTS_PER_REQUEST];
	struct bio *biolist[IDD_MAX_SEGMENTS_PER_REQUEST];
	unsigned int nseg;
	struct bio *bio = NULL;
	int i, nbio = 0;
	int op;
	struct blk_plug plug;

	if(req->data_direction == 1)
		op = WRITE_ODIRECT;
	else if(req->data_direction == 0)
		op = READ;
	else{
		op = 0;
		goto fail_response;
	}

	nseg = req->nr_segments;

	if (unlikely(nseg == 0) || 
		unlikely(nseg > IDD_MAX_SEGMENTS_PER_REQUEST)) {
		printk("Bad number of segments in request (%d)\n", nseg);
		goto fail_response;
	}

	breq.sector_number = req->sector_number;
	breq.nr_sects = 0;
	pending_req->priv_d = be;
	pending_req->id = req->seq_no;
	pending_req->operation = req->data_direction;
	pending_req->status = 0;
	pending_req->nr_pages = nseg;

//Sushrut : fill seg struct
	for (i = 0; i < nseg; i++) {
		seg[i].nsec = req->seg[i].last_sect - req->seg[i].first_sect + 1;
		if ((req->seg[i].last_sect >= (PAGE_SIZE >> KERNEL_SECTOR_SHIFT)) ||
			(req->seg[i].last_sect < req->seg[i].first_sect))
			goto fail_response;
		breq.nr_sects += seg[i].nsec;
	}

/*
 *  breq.bdev = blkdev_get_by_path("/dev/ramd", 
 *	breq.bdev = blkdev_get_by_path("/dev/loop0", 
 *      FMODE_READ | FMODE_WRITE | FMODE_LSEEK | 
 *      FMODE_PREAD | FMODE_PWRITE, NULL);
 *  breq.dev = MKDEV(MAJOR(breq.bdev->bd_inode->i_rdev), 
 *    MINOR(breq.bdev->bd_inode->i_rdev));
 */

  breq.bdev = be->bdev;
  breq.dev = be->dev;

/*
 *  if (IS_ERR(breq.bdev)) {
 *    printk("xen_vbd_create: device %08x could \
 *      not be opened.\n",breq.dev);
 *    return -ENOENT;
 *  }
 *
 *  if (breq.bdev->bd_disk == NULL) {
 *    printk("xen_vbd_create: device %08x doesn't \
 *    exist.\n",breq.dev);
 *    return -ENOENT;
 *  }
 */

	for (i = 0; i < nseg; i++) {
		if (((int)breq.sector_number|(int)seg[i].nsec) &
		((bdev_logical_block_size(breq.bdev) >> 9) - 1)) {
			printk("Misaligned I/O request from domain 0\n");
			goto fail_response;
		}
	}	

//Sushrut : insert pages into bio

	if (map_pages_to_req(req, pending_req, seg))
		goto fail_flush;

	xen_idd_get(be);

	for (i = 0; i < nseg; i++) {
		while ((bio == NULL) ||	bio_add_page(bio, 
					be->pending_page(pending_req, i),
					seg[i].nsec << 9, 
					seg[i].buf & ~PAGE_MASK) == 0) {
	
			bio = bio_alloc(GFP_KERNEL, nseg - i );
			if (unlikely(bio == NULL))
				goto fail_put_bio;

			biolist[nbio++] = bio;	
			bio->bi_bdev = breq.bdev;
			bio->bi_private = pending_req;
			bio->bi_end_io = end_block_io_op;
			bio->bi_sector  = breq.sector_number;
		}
		breq.sector_number += seg[i].nsec;
	}
	atomic_set(&pending_req->pendcnt, nbio);

/*
	if(req->data_direction == 1){
		for(i=0; i < nseg; i++){
			print_hex_dump(KERN_DEBUG, "",DUMP_PREFIX_OFFSET, 16, 1,
                		(void *)vaddr(pending_req, i), PAGE_SIZE, 1);
		}
	}
*/
		
	blk_start_plug(&plug);
	for (i = 0; i < nbio; i++){
		submit_bio(op, biolist[i]);
	}

	blk_finish_plug(&plug);

	return 0;	

fail_flush:
	unmap_pages(pending_req);

fail_response:	
	make_response(be, req->seq_no, req->data_direction, -1);
	free_req(pending_req);
//	msleep(1);
	return -EIO;
fail_put_bio:
	for (i = 0; i < nbio; i++)
		bio_put(biolist[i]);
	__end_block_io_op(pending_req, -EINVAL);
//	msleep(1);
	return -EIO;
}

static int __do_block_io_op(backend_info_t *be){
	struct idd_request req;
	struct pending_req *pending_req;
	RING_IDX rc, rp;
	int more_to_do = 0;

	rc = be->main_ring.req_cons;
	rp = be->main_ring.sring->req_prod;
	rmb();
	
	while (rc != rp) {
		if (RING_REQUEST_CONS_OVERFLOW(&be->main_ring, rc))
			break;
		
		if (kthread_should_stop()) {
			more_to_do = 1;
			break;
		}
		
		pending_req = alloc_req();
		if (NULL == pending_req) {
			more_to_do = 1;
			break;
		}

		memcpy(&req, RING_GET_REQUEST(&be->main_ring, rc), sizeof(req));

		be->main_ring.req_cons = ++rc;
		barrier();

		if(dispatch_rw_block_io(be, &req, pending_req)){
			break;
		}
// Sushrut : yield or cond_resched ?
		cond_resched();
	}
	return more_to_do;
}

static int do_block_io_op(backend_info_t *be){
	int more_to_do = 0;

	do {
		more_to_do = __do_block_io_op(be);
		if (more_to_do)
			break;
		RING_FINAL_CHECK_FOR_REQUESTS(&be->main_ring, more_to_do);
	} while (more_to_do);
	return more_to_do;
}

int idd_request_schedule(void *arg){

	backend_info_t *be = (backend_info_t *)arg;
	RING_IDX rc, rp;
  int status;
  int more_to_do;

  printk("waiting req value %d\n",be->waiting_reqs);
	while(!kthread_should_stop()){
		if (try_to_freeze())
			continue;
    wait_event_interruptible(
      be->wq,
      be->waiting_reqs || kthread_should_stop());

again:
		wait_event_interruptible(
			be->pending_free_wq,
			!list_empty(&be->pending_free) ||
			kthread_should_stop());

    /*
     *if (do_block_io_op(be))
		 *   be->waiting_reqs = 1;
     */

//    do_block_io_op(be);

  	rc = be->main_ring.req_cons;
	  rp = be->main_ring.sring->req_prod;
	  rmb();
	
	  if  (rc != rp && !RING_REQUEST_CONS_OVERFLOW(&be->main_ring, rc)){
      sleep_cond=0;
  		do_block_io_op(be);
    }
    sleep_cond++;
    /*IF THRESHHOLD IS REACHED THEN SLEEP*/
    status = atomic_read(&(backend.main_ring.sring->req_status));
    smp_mb();
    if(sleep_cond > 50000 && status==RUNNING){
//      printk("Sleeping thread %d\n",sleep_cond);
      atomic_set(&(backend.main_ring.sring->req_status), SLEEPING);
      be->waiting_reqs = 0;
      sleep_cond=0;
      RING_FINAL_CHECK_FOR_REQUESTS(&be->main_ring, more_to_do);
      if (more_to_do)
        goto again;
      smp_mb();
    }
	}
	xen_idd_put(be);

	return 0;
}

/*
 *static void idd_notify_work(backend_info_t *be)
 *{
 *  be->waiting_reqs = 1;
 *  wake_up(&be->wq);
 *}
 */

static irqreturn_t irq_ring_interrupt(int irq, void *dev_id)
{
  int status;
  backend.waiting_reqs = 1;
  status = atomic_read(&(backend.main_ring.sring->req_status));
  smp_wmb();
  if(status == SLEEPING)
    wake_up(&backend.wq);

  atomic_set(&(backend.main_ring.sring->req_status), RUNNING);
  smp_wmb();
//  printk("Waking up request thread. new status %d \n",atomic_read(&(backend.main_ring.sring->req_status)));
  return IRQ_HANDLED;
}

static void *idd_alloc_shared(uint32_t *gref)
{
	struct page *page = alloc_page(GFP_KERNEL | __GFP_ZERO);
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
	int i, mmap_pages;

	struct evtchn_alloc_unbound ring_alloc;
	struct evtchn_close close;
	idd_connect_t data;
	int err=0;
	struct idd_sring *sring;

	backend.bdev = blkdev_get_by_path("/dev/ramd", 
//bdev = blkdev_get_by_path("/dev/loop0", 
			FMODE_READ | FMODE_WRITE | FMODE_LSEEK | 
			FMODE_PREAD | FMODE_PWRITE, NULL);

	backend.dev = MKDEV(MAJOR(backend.bdev->bd_inode->i_rdev), 
			MINOR(backend.bdev->bd_inode->i_rdev));

/********************** EVERYTHING BELOW IS A RING BUFFER *******************/

	printk("Inseting module\n");

	sring = idd_alloc_shared(&backend.main_ring_gref);
	if (unlikely(sring == NULL)) {
		err = -ENOMEM;
		goto end;
	}
	SHARED_RING_INIT(sring);
	BACK_RING_INIT(&backend.main_ring, sring, PAGE_SIZE);

	data.domid = 0;
	data.main_ring_gref = backend.main_ring_gref;
        printk("DEBUG : main_ring_gref %u %u\n",data.main_ring_gref,backend.main_ring_gref);

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

	INIT_LIST_HEAD(&backend.pending_free);
	spin_lock_init(&backend.pending_free_lock);

	spin_lock_init(&backend.blk_ring_lock);

	init_waitqueue_head(&backend.pending_free_wq);
	init_waitqueue_head(&backend.wq);
  backend.waiting_reqs=1;

	mmap_pages = xen_idd_reqs * IDD_MAX_SEGMENTS_PER_REQUEST;

	printk("mmap_pages = %d\n", mmap_pages);

	backend.pending_reqs = kzalloc(sizeof(backend.pending_reqs[0])* 
					xen_idd_reqs, GFP_KERNEL);
	
	backend.pending_grant_handles = kmalloc(sizeof(backend.pending_grant_handles[0]) *
					mmap_pages, GFP_KERNEL);

	backend.pending_pages = kzalloc(sizeof(backend.pending_pages[0]) * 
					mmap_pages, GFP_KERNEL);

	if (!backend.pending_reqs || !backend.pending_pages) {
		err = -ENOMEM;
		goto out_of_memory;
	}

	for(i=0; i < mmap_pages ; i++){
		backend.pending_grant_handles[i] = IDD_INVALID_HANDLE;
		backend.pending_pages[i] = alloc_page(GFP_KERNEL);
		if(backend.pending_pages[i] == NULL){
			err = -ENOMEM;
			goto out_of_memory;
		}
	}

	for (i = 0; i < xen_idd_reqs; i++){
		list_add_tail(&backend.pending_reqs[i].free_list,
			&backend.pending_free);
	}

	backend.request_thread = kthread_run(idd_request_schedule, &backend, "request_thread");

	return 0;

out_of_memory:
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
  kthread_stop (backend.request_thread);
	unbind_from_irqhandler(backend.ring_irq, &backend);
}

module_init(blk_init);
module_exit(blk_cleanup);
