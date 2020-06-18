#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libaio.h>

#define fail(str) \
do { \
	perror(str); \
	exit(1); \
} while(0)

int main(int argc, char *argv[])
{
	int fd, r;
	char *buf;
	struct iocb cb;
	struct iocb *list_of_iocb[1] = {&cb};
	unsigned long pgsize = getpagesize();
	io_context_t ctx = {0};

	if (argc != 2) {
		fprintf(stderr, "Usage: %s device_path\n", argv[0]);
		exit(1);
	}

	if (posix_memalign((void **)&buf, pgsize, 4096) != 0)
		fail("posix_memalign()");

	fd = open(argv[1], O_RDONLY | O_DIRECT);
	if (fd < 0) {
		fprintf(stderr, "open(%s) failed: %m\n", argv[1]);
		exit(1);
	}

	r = io_setup(128, &ctx);
	if (r < 0)
		fail("io_setup()");

	memset(&cb, 0, sizeof(cb));
	io_prep_pread(&cb, fd, buf, 4096, 0);

	r = io_submit(ctx, 1, list_of_iocb);
	if (r != 1)
		fail("io_submit()");

#if 0
	struct io_event events[1] = {};
	r = io_getevents(ctx, 1, 1, events, NULL);
	if (r != 1)
		fail("io_getevents()");

	printf("read %lld bytes from %s\n", events[0].res, argv[1]);
	io_destroy(ctx);
#endif
	close(fd);
	fprintf(stderr, "exitting\n");
	return 0;
}
