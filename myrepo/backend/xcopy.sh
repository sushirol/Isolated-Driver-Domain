/root/dev/event-backend/xmount.sh
echo "copying $PWD/backend.ko to /hvm/root"
cp backend.ko /hvm/root/
/root/dev/event-backend/xumount.sh
