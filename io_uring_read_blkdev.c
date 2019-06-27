#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <liburing.h>

#define QD        (256UL)
#define BS        (4UL   * 1024UL)
#define FILE_SIZE (1024UL* 1024UL * 1024UL)

#define DELTA_TIMESPEC_US(END, START) \
	((int)(((END).tv_sec  - (START).tv_sec ) * 1000000 + \
	       ((END).tv_nsec - (START).tv_nsec) / 1000))

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("%s: blockdev\n", argv[0]);
		return 1;
	}

	int devfd = open(argv[1], O_RDONLY|O_DIRECT);

	struct io_uring ring;
	io_uring_queue_init(QD, &ring, 0);

	char* buf = mmap(NULL, BS, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	srand(time(NULL));

	int i, k = 0;
	int submit_time[QD] = {0};
	int submitted  [QD] = {0};
	for (i = 0; i < QD; i++) {
		if ((i != 0) && (i % 32 == 0)) {
			struct timespec start, stop;
			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
			submitted[k] = io_uring_submit(&ring);
			clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
			submit_time[k] = DELTA_TIMESPEC_US(stop, start);
			k++;
		}

		struct iovec iov = {
			.iov_base = buf,
			.iov_len  = BS,
		};
		io_uring_prep_readv(io_uring_get_sqe(&ring),
		                    devfd, &iov, 1,
		                    (rand() % ((FILE_SIZE) / BS)) * BS);
	}

	munmap(buf, BS);
	io_uring_queue_exit(&ring);
	close(devfd);

	int submitted_already = 0;
	for (i = 0; i < k; i++) {
		printf("submitted_already = %3d, submitted_now = %3d, submit_time = %7d us\n",
		        submitted_already,        submitted[i],       submit_time[i]);
		submitted_already += submitted[i];
	}
	return 0;
}
