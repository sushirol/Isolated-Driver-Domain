#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include<linux/module.h>
#include <linux/hdreg.h>

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
#include <linux/workqueue.h>

#include "frontend.h"

#define PART_NO 1
#define KERNEL_SECTOR_SIZE 512
//#define DISK_CAPACITY 20971520
#define DISK_CAPACITY 419430400
#define DEVICE_NAME "ramd"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple block device");

static int idd_init(void);
static void idd_cleanup(void);

static struct new_device_t {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
	struct device dev;
}new_device;

struct grant {
	grant_ref_t gref;
	unsigned long pfn;
	struct llist_node node;
};

idd_irq_info_t info;
static struct request_queue *Queue;
int major_num=0;

module_init(idd_init);
module_exit(idd_cleanup);

int workaround = 0;
//static int counter=0;

void idd_device_request(struct request_queue *);

static const char *op_name(int op)
{
	static const char *const names[] = {
		[0] = "read",
		[1] = "write"};
	if (op < 0 || op >= ARRAY_SIZE(names))
		return "unknown";
 
	if (!names[op])
		return "reserved";

	return names[op];
}

static void idd_restart_queue_callback(void *arg)
{
	idd_irq_info_t *cb_info = (idd_irq_info_t *)arg;
	schedule_work(&cb_info->work);
}


static inline void flush_requests(idd_irq_info_t *flush_info)
{
	int notify;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&flush_info->main_ring, notify);
	notify_remote_via_irq(flush_info->ring_irq);
	printk("notify read backend! seq_no \n");
}

static void kick_pending_request_queues(idd_irq_info_t *kick_info)
{
	if (!RING_FULL(&kick_info->main_ring)) {
		/* Re-enable calldowns. */
		blk_start_queue(kick_info->rq);
		/* Kick things off immediately. */
		idd_device_request(kick_info->rq);
	}
}

static void idd_restart_queue(struct work_struct *work)
{
	idd_irq_info_t *rq_info = container_of(work, idd_irq_info_t, work);
	spin_lock_irq(&rq_info->io_lock);
	kick_pending_request_queues(rq_info);
	spin_unlock_irq(&rq_info->io_lock);
}

static int get_id_from_freelist(idd_irq_info_t *ptr_info)
{
	unsigned long free = ptr_info->shadow_free;
	BUG_ON(free >= IDD_RING_SIZE);
	ptr_info->shadow_free = ptr_info->shadow[free].req.seq_no;
	printk("free %lu shadow_free %lu shadow[%lu].req.seq_no %llu\n", free,ptr_info->shadow_free, free, ptr_info->shadow[free].req.seq_no);
	ptr_info->shadow[free].req.seq_no = 0x0fffffee; /* debug */
	printk("free %lu shadow_free %lu shadow[%lu].req.seq_no %llx\n", free,ptr_info->shadow_free, free, ptr_info->shadow[free].req.seq_no);
	printk("return free %lu\n", free);
	printk("\n");
	return free;
}

static int add_id_to_freelist(idd_irq_info_t *ptr_info,
		unsigned long id)
{
	printk("before shadow_free %lu shadow[%ld].req.seq_no %llu, id %ld\n", ptr_info->shadow_free, id, ptr_info->shadow[id].req.seq_no,id);
	if (ptr_info->shadow[id].req.seq_no != id){
		printk("Went wrong, both equal\n");
		return -EINVAL;
	}
	if (ptr_info->shadow[id].request == NULL){
		printk("Went wrong, NULL\n");
		return -EINVAL;
	}
	ptr_info->shadow[id].req.seq_no  = ptr_info->shadow_free;
	ptr_info->shadow[id].request = NULL;
	ptr_info->shadow_free = id;
	printk("after shadow_free %lu shadow[%ld].req.seq_no %llu, id %ld\n", ptr_info->shadow_free, id, ptr_info->shadow[id].req.seq_no,id);
	printk("\n");
	return 0;
}

static int idd_queue_request(struct request *req){

	unsigned long start_sector, sector_cnt, offset, nbytes;
	struct idd_request *ring_req;
	int write, i, ref;
	unsigned int fsect, lsect;
	bool new_persistent_gnts;
	unsigned long id;

	struct scatterlist *sg;
	struct grant *gnt_list_entry = NULL;
	unsigned long buffer_mfn;
	struct page *granted_page;
	grant_ref_t gref_head;

	start_sector = blk_rq_pos(req);
	sector_cnt = blk_rq_cur_sectors(req);

	offset = start_sector * KERNEL_SECTOR_SIZE;
	nbytes = sector_cnt * KERNEL_SECTOR_SIZE;
	write = rq_data_dir(req);
		
	if( (offset + nbytes) > new_device.size) {
		printk("Beyond-end write(%ld,%ld)\n",offset,nbytes);
		return 1;
	}

	if(workaround == 0)
		workaround = 1;
        smp_mb();

#if 1
	if (info.persistent_gnts_c < IDD_MAX_SEGMENTS_PER_REQUEST) {
		new_persistent_gnts = 1;
		if (gnttab_alloc_grant_references(
			IDD_MAX_SEGMENTS_PER_REQUEST - info.persistent_gnts_c,
			&gref_head) < 0){
				printk("IS THIS AN ERROR ? \n");
#if 0
				gnttab_request_free_callback(
					&info.callback,
					idd_restart_queue_callback,
					&info,
					IDD_MAX_SEGMENTS_PER_REQUEST);
#endif
				return 1;
		}
	}else{
		new_persistent_gnts = 0;
	}
#endif

	ring_req = RING_GET_REQUEST(&info.main_ring, info.main_ring.req_prod_pvt);
	if(ring_req == NULL){
		printk("NULL RING_GET_REQUEST\n");
		BUG_ON(1);
		return 1;
	}
#if 0
	ring_req->priv_data = req;
	ring_req->nbytes = nbytes;
	ring_req->offset = offset;
#endif
	id = get_id_from_freelist(&info);
	info.shadow[id].request = req;

	ring_req->seq_no = id;
	ring_req->data_direction=write;

//	printk("req_prod %d rsp_prod %d\n", info.main_ring.sring->req_prod, info.main_ring.sring->rsp_prod);
//	printk("seq_no %lu offset %lu sector %lu\n",ring_req->seq_no, ring_req->offset,start_sector);
	printk("id %llu \n",ring_req->seq_no);
        smp_mb();

	if (unlikely(req->cmd_flags & (REQ_DISCARD | REQ_SECURE))) {
		printk("DISCARD\n");
	}else{
		ring_req->nr_segments = blk_rq_map_sg(req->q, req, info.sg);
		BUG_ON(req->nr_phys_segments > IDD_MAX_SEGMENTS_PER_REQUEST);
		BUG_ON(ring_req->nr_segments > IDD_MAX_SEGMENTS_PER_REQUEST);
	}
#if 1
	for_each_sg(info.sg, sg, ring_req->nr_segments, i) {
		// KERNEL_SECTOR_SHIFT
		fsect = sg->offset >> KERNEL_SECTOR_SHIFT;
		lsect = fsect + (sg->length >> KERNEL_SECTOR_SHIFT) - 1;
		
		/* Check if we have grants allocated requests */
		if (info.persistent_gnts_c) {
#if 1
			BUG_ON(llist_empty(&info.persistent_gnts));
			gnt_list_entry = llist_entry(
						llist_del_first(&info.persistent_gnts),
						struct grant, node);
			ref = gnt_list_entry->gref;
			buffer_mfn = pfn_to_mfn(gnt_list_entry->pfn);
#endif
			info.persistent_gnts_c--;
#if 1
		}else{
		/* if not then allocate grants */
			ref = gnttab_claim_grant_reference(&gref_head);
			BUG_ON(ref == -ENOSPC);
			
			gnt_list_entry = kmalloc(sizeof(struct grant), GFP_ATOMIC);
			if (!gnt_list_entry)
				return -ENOMEM;

			granted_page = alloc_page(GFP_ATOMIC);
			if (!granted_page) {
				kfree(gnt_list_entry);
				return -ENOMEM;
			}
			
			gnt_list_entry->pfn = page_to_pfn(granted_page);
			gnt_list_entry->gref = ref;
			buffer_mfn = pfn_to_mfn(page_to_pfn(granted_page));
			gnttab_grant_foreign_access_ref(ref, info.domid, buffer_mfn, 0);
#endif
		}
		info.shadow[id].grants_used[i] = gnt_list_entry;
		
		if(write){
			printk("SENDING WRITE !\n");
			printk("offset %u + length %u = %u PAGE SIZE %lu\n", sg->offset, sg->length, sg->offset + sg->length, PAGE_SIZE);
#if 1
			char *bvec_data;
			void *shared_data;

			BUG_ON(sg->offset + sg->length > PAGE_SIZE);

			shared_data = kmap_atomic(pfn_to_page(gnt_list_entry->pfn));
			bvec_data = kmap_atomic(sg_page(sg));

			/*
			 * this does not wipe data stored outside the
			 * range sg->offset..sg->offset+sg->length.
			 * Therefore, blkback *could* see data from
			 * previous requests. This is OK as long as
			 * persistent grants are shared with just one
			 * domain. It may need refactoring if this
			 * changes
			 */

			memcpy(shared_data + sg->offset, bvec_data + sg->offset, sg->length);
			kunmap_atomic(bvec_data);
			kunmap_atomic(shared_data);
#endif
		}
#if 1
		info.shadow[id].frame[i] = mfn_to_pfn(buffer_mfn);
		ring_req->seg[i] = (struct idd_request_segment) {
					.gref       = ref,
					.first_sect = fsect,
					.last_sect  = lsect };
#endif
	}
#endif
	info.main_ring.req_prod_pvt++;
	/* Keep a private copy so we can reissue requests when recovering. */
	info.shadow[id].req = *ring_req;
#if 1
	if (new_persistent_gnts)
		gnttab_free_grant_references(gref_head);
#endif
	return 0;
}

void idd_device_request(struct request_queue *q)
{
	struct request *req;
	int queued = 0;

	while ((req = blk_peek_request(q)) != NULL) {
		if (RING_FULL(&info.main_ring))
			goto wait;

		blk_start_request(req);
                if (req->cmd_type != REQ_TYPE_FS) {
                        printk(KERN_NOTICE ": Non-fs request ignored\n");
                        __blk_end_request_all(req, -EIO);
                        continue;
                }
		
		printk("do_blk_req %p: cmd %p, sec %lx, (%u/%u) buffer:%p [%s]\n",
			req, req->cmd, (unsigned long)blk_rq_pos(req),
			blk_rq_cur_sectors(req), blk_rq_sectors(req),
			req->buffer, rq_data_dir(req) ? "write" : "read");

		if (idd_queue_request(req)) {
			blk_requeue_request(q, req);
wait:
			blk_stop_queue(q);
			break;
		}
	
		queued++;
	}
	
	if(queued!=0){
		flush_requests(&info);
	}
}

struct block_device_operations idd_fops = {
	.owner = THIS_MODULE
};


static void idd_completion(struct idd_shadow *s, struct idd_response *ring_rsp){
	int i = 0;
	struct bio_vec *bvec;
	struct req_iterator iter;
	unsigned long flags;
	char *bvec_data;
	void *shared_data;
	unsigned int offset = 0;

#if 0	
	if (ring_rsp->op == 0){
		rq_for_each_segment(bvec, s->request, iter) {
			BUG_ON((bvec->bv_offset + bvec->bv_len) > PAGE_SIZE);
			if (bvec->bv_offset < offset)
				i++;
			
			BUG_ON(i >= s->req.nr_segments);
			shared_data = kmap_atomic(pfn_to_page(s->grants_used[i]->pfn));
			bvec_data = bvec_kmap_irq(bvec, &flags);
			memcpy(bvec_data, shared_data + bvec->bv_offset,
				bvec->bv_len);
			bvec_kunmap_irq(bvec_data, &flags);
			kunmap_atomic(shared_data);
			offset = bvec->bv_offset + bvec->bv_len;
		}
	}
#endif
	for (i = 0; i < s->req.nr_segments; i++) {
		llist_add(&s->grants_used[i]->node, &info.persistent_gnts);
		info.persistent_gnts_c++;
	}
}

static irqreturn_t irq_ring_interrupt(int irq, void *dev_id)
{

	RING_IDX rp,i;
	struct idd_response *ring_rsp;
	struct request *req;
	unsigned long flags;
//	int error;

//	char * buffer;
//	unsigned long start_sector, sector_cnt, offset, nbytes;

	spin_lock_irqsave(&info.io_lock, flags);

	if(workaround == 0){
		printk("fake interrrupt handled !\n");
		spin_unlock_irqrestore(&info.io_lock, flags);
		return IRQ_HANDLED;
	}

again:
	rp = info.main_ring.sring->rsp_prod;
        rmb();
	
	for (i = info.main_ring.rsp_cons; i != rp; i++) {

		unsigned long id;
		
		ring_rsp = RING_GET_RESPONSE(&info.main_ring, i);
		id  = ring_rsp->seq_no;

		printk("INTERRUPT : Write result %d seq no %lu\n", ring_rsp->res, ring_rsp->seq_no);
		printk("Debug: INTERRUPT req_prod_pvt %d ", info.main_ring.req_prod_pvt);
		printk("Debug: INTERRUPT req_prod %d rsp_prod %d\n", info.main_ring.sring->req_prod, info.main_ring.sring->rsp_prod);

#if 0
		if (id >= BLK_RING_SIZE) {
			WARN(1, "%s: response to %s has incorrect id (%ld)\n",
				info.gd->disk_name, op_name(ring_rsp->op), id);
			continue;
		}
#endif
		
		req  = info.shadow[id].request;
		
		if(req!=NULL)
			idd_completion(&info.shadow[id], ring_rsp);
		
		if (add_id_to_freelist(&info, id)) {
			WARN(1, "response to %s (id %ld) couldn't be recycled!\n",
				op_name(ring_rsp->op), id);
			continue;
		}
		
//		req = ring_rsp->priv_data;
		switch(ring_rsp->op){
			case 0:
			case 1:
				__blk_end_request_all(req, 0);
				break;
			default:
				BUG();
		}
	}
	info.main_ring.rsp_cons = i;
	if (i != info.main_ring.req_prod_pvt) {
		int more_to_do;
		RING_FINAL_CHECK_FOR_RESPONSES(&info.main_ring, more_to_do);
		if (more_to_do)
			goto again;
	} else
		info.main_ring.sring->rsp_event = i + 1;
	kick_pending_request_queues(&info);
	spin_unlock_irqrestore(&info.io_lock, flags);


#if 0
				buffer = req->buffer;
				start_sector = blk_rq_pos(req);
				sector_cnt = blk_rq_cur_sectors(req);

				offset = start_sector * KERNEL_SECTOR_SIZE;
				nbytes = sector_cnt * KERNEL_SECTOR_SIZE;

//				memcpy(buffer,(void *)info.io_data_page,nbytes);
#endif
	
	return IRQ_HANDLED;
}

static struct vm_struct *idd_alloc_shared(uint32_t gref, uint32_t domid,
        grant_handle_t *handle)
{
        struct vm_struct *area;
        struct gnttab_map_grant_ref map_op;
        pte_t *pte[2];

        area = alloc_vm_area( PAGE_SIZE, pte);
        if (!area)
                return NULL;

        map_op.host_addr = arbitrary_virt_to_machine(pte[0]).maddr;
        map_op.ref = gref;
        map_op.dom = domid;
        map_op.flags = GNTMAP_host_map | GNTMAP_contains_pte;

        if (HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &map_op, 1))
                goto error;
        if (map_op.status != GNTST_okay)
                goto error;

        *handle = map_op.handle;
        return area;

error:
        free_vm_area(area);
        return NULL;
}

static int idd_free_shared(struct vm_struct *area, grant_handle_t handle)
{
        struct gnttab_unmap_grant_ref unmap_op;
        unsigned int level;

        unmap_op.host_addr = arbitrary_virt_to_machine(
                lookup_address((unsigned long) area->addr, &level)).maddr;
        unmap_op.handle = handle;
        unmap_op.dev_bus_addr = 0;

        if (HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_op, 1))
                return -EFAULT;
        if (unmap_op.status != GNTST_okay)
                return -EFAULT;
        free_vm_area(area);
        return 0;
}


static int idd_init(void)
{

//frontend.c
	idd_connect_t data;
	int err=0,i = 0;
//	struct vm_struct* shared_vm;
	printk("Inseting module frontend\n");

/************************* EVERYTHING BELOW IS RELATED TO RING BUFFER ******************/
	sema_init(&info.notify_sem, 0);
	sema_init(&info.rw_sem, 1);

	err = HYPERVISOR_idd_service_op(IDD_SERVICE_CONNECT, IDD_SYSID_ONE, &data);
	if (unlikely(err != 0)) {
		printk(KERN_ERR "Cannot connect to the remote domain\n");
		goto end;
	}
        printk("dom.port %u data.domid %u\n",data.ring_port,data.domid);

        printk("DEBUG : main_ring_gref %u\n",data.main_ring_gref);
        printk("DEBUG : data_ring_gref %u\n",data.data_ring_gref);

	info.domid = data.domid;
	info.main_ring_area = idd_alloc_shared(data.main_ring_gref, data.domid, &info.main_ring_handle);
	if (info.main_ring_area == NULL) {
		err = -ENOMEM;
                goto end;
        }
	FRONT_RING_INIT(&info.main_ring, (struct idd_sring *) info.main_ring_area->addr, PAGE_SIZE);
	spin_lock_init(&info.io_lock);

/*********************** SHARED IO DATA PAGE **************************************/
//	shared_vm = idd_alloc_shared(data.data_ring_gref, data.domid, &info.data_ring_handle);
//	info.io_data_page = (void *)shared_vm->addr;

	sg_init_table(info.sg, IDD_MAX_SEGMENTS_PER_REQUEST);
	init_llist_head(&info.persistent_gnts);
	info.persistent_gnts_c = 0;
	info.shadow_free = 0;

	INIT_WORK(&info.work, idd_restart_queue);

	memset(&info.shadow, 0, sizeof(info.shadow));	

	printk("IDD_RING_SIZE %d \n", IDD_RING_SIZE);

	for (i = 0; i < IDD_RING_SIZE; i++){
		info.shadow[i].req.seq_no = i+1;
	}
	info.shadow_free = info.main_ring.req_prod_pvt;
	info.shadow[IDD_RING_SIZE-1].req.seq_no = 0x0fffffff;

/************************* EVERYTHING BELOW IS RELATED TO EVENT CH******************/
	info.ring_irq = -1;
	
        err = bind_interdomain_evtchn_to_irqhandler(data.domid, data.ring_port,
                irq_ring_interrupt, 0, "ring_irq", NULL);

        if (unlikely(err < 0)) {
                printk(KERN_WARNING "Cannot bind (main) event channel\n");
                goto end2;
        }
        info.ring_irq = err;
        printk("dom.port %u\n",data.ring_port);
        smp_mb();


/************************* EVERYTHING BELOW IS RELATED TO RAMDISK ******************/
	//device registration
	new_device.size = DISK_CAPACITY;
	new_device.data=vmalloc(new_device.size);
	if(new_device.data == NULL){
		err = -ENOMEM;
		goto end3;
	}

	Queue = blk_init_queue(idd_device_request, &info.io_lock);
	if(Queue == NULL)
		goto end4;
	blk_queue_max_segments(Queue, IDD_MAX_SEGMENTS_PER_REQUEST);
//	blk_queue_logical_block_size(Queue, sector_size);
	blk_queue_max_hw_sectors(Queue, 512);
	blk_queue_segment_boundary(Queue, PAGE_SIZE - 1);
	blk_queue_max_segment_size(Queue, PAGE_SIZE);
	blk_queue_dma_alignment(Queue, 511);
	blk_queue_bounce_limit(Queue, IDD_BOUNCE_ANY);

	printk("Registering block device\n");
	major_num = register_blkdev(0,DEVICE_NAME);
	if(major_num <= 0){
		printk("failed to register\n");
		goto end4;
	}else{
		printk("registeration successful major_num %d\n",major_num);
	}
	new_device.gd = alloc_disk(PART_NO);
	if(!new_device.gd){
		printk("Disk allocation failed\n");
		goto out_unregister;
	}
	printk("Disk allocation successful\n");

	new_device.gd->major = major_num;
	printk ("rxd: debug: Device successfully registered: Major No. = %d %d\n", new_device.gd->major,major_num);
	new_device.gd->first_minor = 0;
	new_device.gd->minors = PART_NO;
	new_device.gd->fops = &idd_fops;
	new_device.gd->private_data = &new_device;
	snprintf(new_device.gd->disk_name,sizeof(DEVICE_NAME),DEVICE_NAME);
	set_capacity(new_device.gd,DISK_CAPACITY/KERNEL_SECTOR_SIZE );
	new_device.gd->queue = Queue;
	add_disk(new_device.gd);

	info.rq = new_device.gd->queue;

        smp_mb();

        return 0;
out_unregister:
	unregister_blkdev(major_num, DEVICE_NAME);
end4:
	vfree(new_device.data);
end3:
	unbind_from_irqhandler(info.ring_irq, &info);
end2:
	idd_free_shared(info.main_ring_area, info.main_ring_handle);
end :
	return err;
}

static void idd_cleanup(void)
{
	del_gendisk(new_device.gd);
	put_disk(new_device.gd);
	printk("unregister blkdev\n");
	unregister_blkdev(major_num,DEVICE_NAME);
	blk_cleanup_queue(Queue);
	vfree(new_device.data);
}

