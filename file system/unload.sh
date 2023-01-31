DIR=/tmp/lfs-mountpoint
if [ -d  "$DIR" ]; then
	fusermount -u $DIR
	rmdir $DIR
fi