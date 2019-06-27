all: io_uring_read_blkdev

io_uring_read_blkdev: io_uring_read_blkdev.c
	gcc io_uring_read_blkdev.c -luring -o io_uring_read_blkdev
