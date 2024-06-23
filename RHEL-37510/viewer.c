#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdalign.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#define SECTOR_SIZE 512
#define PATTERN_LEN 64
#define NR_PATTERN SECTOR_SIZE / PATTERN_LEN

void read_buf(int fd, char *buf, size_t size)
{
	ssize_t bytes;
	size_t len = 0;

	while (len < size) {
		bytes = read(fd, buf + len, size - len);
		if (bytes < 0) {
			if (errno == EINTR)
				continue;
			perror("read failed");
			exit(1);
		}
		if (bytes == 0) {
			fprintf(stderr, "unexpected end of file\n");
			exit(1);
		}
		len += bytes;
	}
}

int main(int argc, char *argv[])
{
	char *buf;
	int r, i, fd;
	size_t sector, nr_sectors = 1;
	char unit, dummy;

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s <file> <sector> [<nr_sectors>]\n",
			argv[0]);
		return 1;
	}

	r = sscanf(argv[2], "%zu%c%c", &sector, &unit, &dummy);
	if (r < 1 || r > 2) {
		fprintf(stderr, "Invalid sector: '%s'\n", argv[2]);
		return 1;
	}
	if (r == 2) {
		if (unit != 'p') {
			fprintf(stderr, "Invalid unit: '%c'\n", unit);
			return 1;
		}
		sector *= 8;
	}
	if (argc == 4) {
		r = sscanf(argv[3], "%zu%c%c", &nr_sectors, &unit, &dummy);
		if (r < 1 || r > 2) {
			fprintf(stderr, "Invalid nr_sectors: '%s'\n", argv[3]);
			return 1;
		}
		if (r == 2) {
			if (unit != 'p') {
				fprintf(stderr, "Invalid unit: '%c'\n", unit);
				return 1;
			}
			nr_sectors *= 8;
		}
	}
	if (!(buf = calloc(nr_sectors, SECTOR_SIZE))) {
		fprintf(stderr, "cannot allocate memory for %zu sectors: %m\n",
			nr_sectors);
		return 1;
	}
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open of '%s' failed: %m\n", argv[1]);
		free(buf);
		return 1;
	}
	if (lseek64(fd, sector * SECTOR_SIZE, SEEK_SET) < 0) {
		fprintf(stderr, "seek to %zu failed: %m\n",
			sector * SECTOR_SIZE);
		goto fail;
	}
	read_buf(fd, buf, nr_sectors * SECTOR_SIZE);
	for (i = 0; i < NR_PATTERN * nr_sectors; i++)
		printf("%.64s\n", buf + (PATTERN_LEN * i));

	free(buf);
	if (close(fd)) {
		fprintf(stderr, "close of '%s' failed: %m\n", argv[1]);
		return 1;
	}
	return 0;
fail:
	close(fd);
	free(buf);
	return 1;
}
