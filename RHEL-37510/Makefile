all: compare viewer

compare: compare.c
	$(CC) -Wall -o $@ $^

viewer: viewer.c
	$(CC) -Wall -o $@ $^

clean:
	$(RM) compare viewer check_file
