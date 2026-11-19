// AUTO-DRAFT from postgres/postgres PR #4280abf88409d09e5828c4642219df053220a650
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct rfile
{
	char	   *filename;
	int			fd;
	size_t		header_length;
	unsigned	num_blocks;
	BlockNumber *relative_block_numbers;
	unsigned	truncation_block_length;
	unsigned	num_blocks_read;
	off_t		highest_offset_read;
} rfile;
/* …（同文件无关代码省略）… */
	rfile	  **sourcemap;
	off_t	   *offsetmap;
	unsigned	block_length;
	unsigned	sidx = n_prior_backups;
	bool		full_copy_possible = true;
	int			copy_source_index = -1;
/* …（同文件无关代码省略）… */
	latest_source = make_incremental_rfile(input_filename);
	source[n_prior_backups] = latest_source;
	block_length = find_reconstructed_block_length(latest_source);

	/*
	 * For each block in the output file, we need to know from which file we
/* …（同文件无关代码省略）… */
			 * later backup is one that we can't validate.
			 */
			blocklength = sb.st_size / BLCKSZ;
			for (b = 0; b < latest_source->truncation_block_length; ++b)
			{
				if (sourcemap[b] == NULL && b < blocklength)
				{
/* …（同文件无关代码省略）… */
		{
			BlockNumber b = s->relative_block_numbers[i];

			if (b < latest_source->truncation_block_length &&
				sourcemap[b] == NULL)
			{
				sourcemap[b] = s;
/* …（同文件无关代码省略）… */
				full_copy_possible = false;
			}
		}
	}

	/*
/* …（同文件无关代码省略）… */
static unsigned
find_reconstructed_block_length(const rfile *s)
{
	unsigned	block_length = s->truncation_block_length;
	unsigned	i;

	for (i = 0; i < s->num_blocks; ++i)
		if (s->relative_block_numbers[i] >= block_length)
			block_length = s->relative_block_numbers[i] + 1;

	return block_length;
}
/* …（同文件无关代码省略）… */
static rfile *
make_incremental_rfile(const char *filename)
{
	rfile	   *rf;
	unsigned	magic;

	rf = make_rfile(filename, false);

	/* Read and validate magic number. */
	read_bytes(rf, &magic, sizeof(magic));
	if (magic != INCREMENTAL_MAGIC)
		pg_fatal("file \"%s\" has bad incremental magic number (0x%x, expected 0x%x)",
				 filename, magic, INCREMENTAL_MAGIC);

	/* Read block count. */
	read_bytes(rf, &rf->num_blocks, sizeof(rf->num_blocks));
	if (rf->num_blocks > RELSEG_SIZE)
		pg_fatal("file \"%s\" has block count %u in excess of segment size %u",
				 filename, rf->num_blocks, RELSEG_SIZE);

	/* Read truncation block length. */
	read_bytes(rf, &rf->truncation_block_length,
			   sizeof(rf->truncation_block_length));
	if (rf->truncation_block_length > RELSEG_SIZE)
		pg_fatal("file \"%s\" has truncation block length %u in excess of segment size %u",
				 filename, rf->truncation_block_length, RELSEG_SIZE);

	/* Read block numbers if there are any. */
	if (rf->num_blocks > 0)
	{
		rf->relative_block_numbers =
			pg_malloc0_array(BlockNumber, rf->num_blocks);
		read_bytes(rf, rf->relative_block_numbers,
				   sizeof(BlockNumber) * rf->num_blocks);
		for (unsigned i = 0; i < rf->num_blocks; i++)
		{
			if (rf->relative_block_numbers[i] >= RELSEG_SIZE)
				pg_fatal("file \"%s\" has block number %u in excess of segment size %u",
						 filename, rf->relative_block_numbers[i], RELSEG_SIZE);
		}
	}

	/* Remember length of header. */
	rf->header_length = sizeof(magic) + sizeof(rf->num_blocks) +
		sizeof(rf->truncation_block_length) +
		sizeof(BlockNumber) * rf->num_blocks;

	/*
	 * Round header length to a multiple of BLCKSZ, so that blocks contents
	 * are properly aligned. Only do this when the file actually has data for
	 * some blocks.
	 */
	if ((rf->num_blocks > 0) && ((rf->header_length % BLCKSZ) != 0))
		rf->header_length += (BLCKSZ - (rf->header_length % BLCKSZ));

	return rf;
}
/* …（同文件无关代码省略）… */
static rfile *
make_rfile(const char *filename, bool missing_ok)
{
	rfile	   *rf;

	rf = pg_malloc0_object(rfile);
	rf->filename = pstrdup(filename);
	if ((rf->fd = open(filename, O_RDONLY | PG_BINARY, 0)) < 0)
	{
		if (missing_ok && errno == ENOENT)
		{
			pfree(rf->filename);
			pg_free(rf);
			return NULL;
		}
		pg_fatal("could not open file \"%s\": %m", filename);
	}

	return rf;
}
/* …（同文件无关代码省略）… */
static void
read_bytes(const rfile *rf, void *buffer, size_t length)
{
	ssize_t		rb;

	rb = read(rf->fd, buffer, length);

	if (rb != length)
	{
		if (rb < 0)
			pg_fatal("could not read file \"%s\": %m", rf->filename);
		else
			pg_fatal("could not read file \"%s\": read %zd of %zu",
					 rf->filename, rb, length);
	}
}
