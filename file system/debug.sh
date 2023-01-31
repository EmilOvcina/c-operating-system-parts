DIR=/tmp/lfs-mountpoint
if [ ! -d  "$DIR" ]; then
	mkdir $DIR
	chmod 755 $DIR
fi
./lfs -d $DIR