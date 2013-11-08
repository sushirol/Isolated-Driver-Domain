./xmount.sh
rm /hvm/root/*.ko
echo "copying $PWD/backend.ko to /hvm/root"
cp backend.ko /hvm/root/
cp ../../ramdisk/ramdisk.ko /hvm/root
./xumount.sh
