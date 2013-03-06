#sai:/etc/xen$ sudo parted /etc/xen/xen_disk/disk.img 
#Password:
#GNU Parted 3.0
#Using /etc/xen/xen_disk/disk.img
#Welcome to GNU Parted! Type 'help' to view a list of commands.
#(parted) unit                                                             
#Unit?  [compact]? B                                                       
#(parted) print                                                            
#Model:  (file)
#Disk /etc/xen/xen_disk/disk.img: 3145728000B
#Sector size (logical/physical): 512B/512B
#Partition Table: msdos

#Number  Start       End          Size         Type     File system     Flags
# 1      512B        104857599B   104857088B   primary  ext4            boot
# 2      104857600B  599785471B   494927872B   primary  linux-swap(v1)
# 3      599785472B  3145727999B  2545942528B  primary  ext4

#sudo umount /hvm/boot
#sudo umount /hvm

sudo /root/dev/scripts/xumount.sh

#sudo mount -o loop,rw,offset=599785472 /etc/xen/xen_disk/disk.img /hvm
#sudo mount -o loop,rw,offset=512 /etc/xen/xen_disk/disk.img /hvm/boot

sudo mount /dev/sda5 /hvm

