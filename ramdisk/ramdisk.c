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
#include <linux/delay.h>

#include <linux/hardirq.h>

#define PART_NO 1
#define KERNEL_SECTOR_SIZE 512
//#define DISK_CAPACITY 20971520
#define DISK_CAPACITY 512000000
//#define DISK_CAPACITY 2048000000
#define DEVICE_NAME "ramd"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple block device");

static int blk_init(void);
static void blk_cleanup(void);

static struct new_device_t {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
	struct device dev;
}new_device;

static struct request_queue *Queue;
int major_num=0;

module_init(blk_init);
module_exit(blk_cleanup);


/*void hexdump(void *ptr, int index) {
  unsigned char *buffer = (unsigned char*)ptr;
  int width = 4;
  printk("========HexDump========\n");
  unsigned long i,spacer;
  for (i=0;i<index;i++){
	printk("%02x ",buffer[i]);
	if((i % 16)==0)
		printk("\n");
  }
  for (spacer=index;spacer<width;spacer++)
	printk("	");
  printk(": ");
  for (i=0;i<index;i++){
	if (buffer[i] < 32) printk(".");
	else printk("%c",buffer[i]);
	if((i % 48)==0)
		printk("\n");
  }
  printk("\n");
}*/

int blk_device_transfer(struct new_device_t *dev, unsigned long sector,
                unsigned long nsect, char *buffer, int write)
{
	unsigned long offset = sector * KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
	if( (offset + nbytes) > dev->size) {
		printk("Beyond-end write(%ld,%ld)\n",offset,nbytes);
		return -EIO;
	}
	if (write){
//		printk("Write %lu\n", nbytes);
		memcpy(dev->data + offset, buffer, nbytes);
		if(unlikely(in_interrupt())){
			printk("in interrupt\n");
		}
//		printk("(%.*s)\n",nbytes,buffer);
//		hexdump(buffer,nbytes);
	}
	else{
//		printk("read! %lu\n",offset);
		memcpy(buffer, dev->data + offset, nbytes);
		if(unlikely(in_interrupt())){
			printk("in interrupt\n");
		}
//		printk("(%.*s)\n",nbytes,dev->data + offset);
//		hexdump(buffer,nbytes);
	}
	return 0;
}


void blk_device_request(struct request_queue *q)
{
	struct request *req;
	sector_t start_sector;
	unsigned int sector_cnt;
	int err = 0 ;

	req = blk_fetch_request(q);
	while (req != NULL) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			break;
		}
		start_sector = blk_rq_pos(req);
		sector_cnt = blk_rq_cur_sectors(req);
//		printk("nr_phys_segments %d\n",req->nr_phys_segments);
		err = blk_device_transfer(&new_device, start_sector, sector_cnt, req->buffer, rq_data_dir(req));
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

#if 1
static int blk_open(struct block_device *bdev, fmode_t mode){
	return 0;
}

static int brd_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg){

//	printk("ioctl request for : %d\n",cmd);
//	dump_stack();

	return 0;
}

#endif

int brd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;
	size = new_device.size;
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
//	printk("ioctl request getgeo:\n");
	return 0;
}

struct block_device_operations blk_fops = {
	.open = blk_open,
	.owner = THIS_MODULE,
	.ioctl =  brd_ioctl,
	.getgeo = brd_getgeo
};

static int blk_init(void)
{
	//device registration
	new_device.size = DISK_CAPACITY;
	spin_lock_init(&new_device.lock);
	new_device.data=vmalloc(new_device.size);
	if(new_device.data == NULL)
		return -ENOMEM;

	Queue = blk_init_queue(blk_device_request, &new_device.lock);
	if(Queue == NULL)
		goto out;

	printk("Registering block device\n");
	major_num = register_blkdev(0,DEVICE_NAME);
	if(major_num <= 0){
		printk("failed to register\n");
		goto out;
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
	new_device.gd->fops = &blk_fops;
	new_device.gd->private_data = &new_device;
//	strcpy(new_device.gd->disk_name, DEVICE_NAME);
	snprintf(new_device.gd->disk_name,sizeof(DEVICE_NAME),DEVICE_NAME);
	set_capacity(new_device.gd,DISK_CAPACITY/KERNEL_SECTOR_SIZE );
	new_device.gd->queue = Queue;
	printk("before add disk\n");
	add_disk(new_device.gd);
	printk("after add disk\n");
	printk ("rxd: debug: Device successfully registered: Major No. = %d\n", new_device.gd->major);
	printk ("rxd: debug: Capacity of RAM disk is: %d MB\n", DISK_CAPACITY);

	return 0;
out_unregister:
	unregister_blkdev(major_num, DEVICE_NAME);
out:
	vfree(new_device.data);
	return -ENOMEM;
}

static void blk_cleanup(void)
{
	printk("Delete gendisk\n");
	del_gendisk(new_device.gd);
	put_disk(new_device.gd);
	printk("unregister blkdev\n");
	unregister_blkdev(major_num,DEVICE_NAME);
	blk_cleanup_queue(Queue);
	vfree(new_device.data);
}

