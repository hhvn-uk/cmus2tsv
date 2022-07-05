/*
 * cmus2tsv.c
 *
 * Copyright (c) 2022 hhvn <dev@hhvn.uk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define ALIGN(size) (((size) + sizeof(long) - 1) & ~(sizeof(long) - 1))

#define CACHE_VERSION   0x0d

#define CACHE_64_BIT	0x01
#define CACHE_BE	0x02

#define ENTRY_USED_SIZE		28
#define ENTRY_RESERVED_SIZE	52
#define ENTRY_TOTAL_SIZE	(ENTRY_RESERVED_SIZE + ENTRY_USED_SIZE)

char cache_header[8] = "CTC\0\0\0\0\0";
struct entry {
	uint32_t size;

	int32_t play_count;
	int64_t mtime;
	int32_t duration;
	int32_t bitrate;
	int32_t bpm;

	uint8_t _reserved[ENTRY_RESERVED_SIZE];

	char strings[];
};

/* all (significant) metadata stored in track_info */
enum {
	META_TITLE,
	META_ARTIST,
	META_GENRE,
	META_ALBUM,
	META_ALBUMARTIST,
	META_TRACK,
	META_COMMENT,
	META_LAST
};

void
display_header(void) {
	printf("Filename\tTitle\tArtist\tGenre\tAlbum\tAlbumArtist\tTrack\tDuration\tDuration(s)\tPlay count\tComment\n");
}

void
display(struct entry *e) {
	int size, pos, i, count;
	char *metakey, *metaval;
	char *meta[META_LAST];

	for (size = e->size - sizeof(*e), count = i = 0; i < size; i++) {
		if (!e->strings[i])
			count++;
	}
	count = (count - 3) / 2;

	printf("%s\t", e->strings); /* filename */
	pos = strlen(e->strings) + 1;
	pos += strlen(e->strings + pos) + 1;
	pos += strlen(e->strings + pos) + 1;

	for (i = 0; i < META_LAST; i++)
		meta[i] = "";

	for (i = 0; i < count; i++) {
		size = strlen(e->strings + pos) + 1;
		metakey = e->strings + pos;
		pos += size;

		size = strlen(e->strings + pos) + 1;
		metaval = e->strings + pos;
		pos += size;

		if (strcmp(metakey, "title") == 0)
			meta[META_TITLE] = metaval;
		else if (strcmp(metakey, "artist") == 0)
			meta[META_ARTIST] = metaval;
		else if (strcmp(metakey, "album") == 0)
			meta[META_ALBUM] = metaval;
		else if (strcmp(metakey, "tracknumber") == 0)
			meta[META_TRACK] = metaval;
		else if (strcmp(metakey, "genre") == 0)
			meta[META_GENRE] = metaval;
		else if (strcmp(metakey, "albumartist") == 0)
			meta[META_ALBUMARTIST] = metaval;
		else if (strcmp(metakey, "comment") == 0)
			meta[META_COMMENT] = metaval;
	}

	if (!*meta[META_ALBUMARTIST])
		meta[META_ALBUMARTIST] = meta[META_ARTIST];

	printf("%s\t%s\t%s\t%s\t%s\t%s\t", meta[META_TITLE],
			meta[META_ARTIST], meta[META_GENRE],
			meta[META_ALBUM], meta[META_ALBUMARTIST],
			meta[META_TRACK]);

	if (e->duration < 60)
		printf("00:%02d\t", e->duration);
	else
		printf("%02d:%02d\t", ((int)e->duration / 60), e->duration - ((int)e->duration / 60) * 60);

	printf("%ld\t%d\t", e->duration, e->play_count);
	printf("%s\n", meta[META_COMMENT]);
}

void
read_cache(char *file) {
	struct entry *e;
	unsigned int flags = 0;
	unsigned int size, offset = 0;
	struct stat st;
	char *buf;
	int fd;

#ifdef BIG_ENDIAN
	flags |= CACHE_BE;
#endif

	if (sizeof(long) == 8)
		flags |= CACHE_64_BIT;

	cache_header[7] = flags & 0xff; flags >>= 8;
	cache_header[6] = flags & 0xff; flags >>= 8;
	cache_header[5] = flags & 0xff; flags >>= 8;
	cache_header[4] = flags & 0xff;

	cache_header[3] = CACHE_VERSION;

	if ((fd = open(file, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: %s\n", strerror(errno), file);
		exit(EXIT_FAILURE);
	}
	fstat(fd, &st);
	if (st.st_size < sizeof(cache_header))
		goto close;
	size = st.st_size;

	buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buf == MAP_FAILED)
		goto close;

	if (memcmp(buf, cache_header, sizeof(cache_header)))
		goto corrupt;

	offset = sizeof(cache_header);
	while (offset < size) {
		e = (void *)buf + offset;
		display(e);
		offset += ALIGN(e->size);
	}

	munmap(buf, size);
	close(fd);
	return;

corrupt:
	fprintf(stderr, "Corrupt: %s\n", file);
	munmap(buf, size);
close:
	close(fd);
}

int
main(int argc, char *argv[]) {
	char *base;

	base = basename(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s <cachefile>\n", base);
		return 2;
	}

	display_header();
	read_cache(argv[1]);
}
