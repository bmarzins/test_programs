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

#define BUFSIZE 4096U
#define PATTERN "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234abcdefghijklmnopqrstuvwxyz00000000"
#define PATTERN_LEN (sizeof(PATTERN) - 1)
#define COUNT_OFFSET (PATTERN_LEN - 8)
#define NR_PATTERN (BUFSIZE / PATTERN_LEN)

void read_buf(int fd, char *buf)
{
	ssize_t bytes;
	size_t len = 0;

	while (len < BUFSIZE) {
		bytes = read(fd, buf + len, BUFSIZE - len);
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
		if (bytes % 512) {
			fprintf(stderr, "read %li bytes, unaligned\n", bytes);
			exit(1);
		}
		len += bytes;
	}
}

void update_pattern(unsigned int count, char *pattern)
{
	int i;
	char buf[9];
	char *ptr = pattern + COUNT_OFFSET;
	count /= NR_PATTERN;

	for (i = 0; i < NR_PATTERN; i++) {
		snprintf(buf, sizeof(buf), "%08u", count);
		memcpy(ptr, buf, 8);
		ptr += PATTERN_LEN;
		count++;
	}
}

void validate_file(size_t size, int fd, char *buf, char *pattern)
{
	size_t i;

	for (i = 0; i < size; i += BUFSIZE) {
		update_pattern(i, pattern);
		read_buf(fd, buf);
		if (memcmp(buf, pattern, BUFSIZE)) {
			int j;
			fprintf(stderr,
				"file not equal to the pattern at %zu\n",
				i >> 9);
			fprintf(stderr, "++++file++++\n");
			for (j = 0; j < NR_PATTERN; j++)
				fprintf(stderr, "%.64s\n",
					buf + (PATTERN_LEN * j));
			fprintf(stderr, "++++pattern++++\n");
			for (j = 0; j < NR_PATTERN; j++)
				fprintf(stderr, "%.64s\n",
					pattern + (PATTERN_LEN * j));
			exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	char pattern[BUFSIZE];
	alignas(BUFSIZE) char buf[BUFSIZE];
	struct stat sb;
	int fd;
	size_t i, size;
	char dummy;

	assert(BUFSIZE % 512 == 0);
	assert(512 % PATTERN_LEN == 0);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file> <bytes>\n", argv[0]);
		return 1;
	}

	if (sscanf(argv[2], "%zu%c", &size, &dummy) != 1) {
		fprintf(stderr, "Invalid size: '%s'\n", argv[2]);
		return 1;
	}
	if (size % BUFSIZE) {
		fprintf(stderr, "size %lu is not a multiple of %u\n", size,
			BUFSIZE);
		return 1;
	}
	if (size > (1UL << 32)) {
		fprintf(stderr, "size %zu is greated than %lu\n", size,
			(1UL << 32));
		return 1;
	}

	if ((fd = open(argv[1], O_RDONLY | O_DIRECT)) < 0) {
		fprintf(stderr, "open of '%s' failed: %m\n", argv[1]);
		return 1;
	}

	for (i = 0; i < NR_PATTERN; i++)
		memcpy(&pattern[i * PATTERN_LEN], PATTERN, PATTERN_LEN);

	while (1) {
		if (stat("check_file", &sb)) {
			if (errno != ENOENT) {
				perror("stat of check_file failed");
				return 1;
			}
			break;
		}
		printf("validating file\n");
		validate_file(size, fd, buf, pattern);
		if (lseek(fd, 0, SEEK_SET) < 0) {
			perror("resetting the file failed");
			return 1;
		}
	}

	if (close(fd)) {
		fprintf(stderr, "close of '%s' failed: %m\n", argv[1]);
		return 1;
	}
	return 0;
}
