// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * 06/2017 modified by Xiao Yang <yangx.jy@cn.fujitsu.com>
 */

/*
 * Description:
 *  lseek() succeeds to set the specified offset according to whence
 *  and read valid data from this location.
 */

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "tst_test.h"

#define WRITE_STR "abcdefg"
#define TFILE "tfile"

static int fd;
static struct tcase {
	off_t off;
	int whence;
	char *wname;
	off_t exp_off;
	ssize_t exp_size;
	char *exp_data;
} tcases[] = {
	{4, SEEK_SET, "SEEK_SET", 4, 3, "efg"},
	{-2, SEEK_CUR, "SEEK_CUR", 5, 2, "fg"},
	{-4, SEEK_END, "SEEK_END", 3, 4, "defg"},
	{0, SEEK_END, "SEEK_END", 7, 0, NULL},
};

static void verify_lseek(unsigned int n)
{
    write_coordination_file("lseek01");
	char read_buf[64];
	struct tcase *tc = &tcases[n];

	// reset the offset to end of file
	SAFE_READ(0, fd, read_buf, sizeof(read_buf));

	memset(read_buf, 0, sizeof(read_buf));

	TEST(lseek(fd, tc->off, tc->whence));
	if (TST_RET == (off_t) -1) {
		tst_res(TFAIL | TTERRNO, "lseek(%s, %lld, %s) failed", TFILE,
			(long long int)tc->off, tc->wname);
		return;
	}

	if (TST_RET != tc->exp_off) {
		tst_res(TFAIL, "lseek(%s, %lld, %s) returned %ld, expected %lld",
			TFILE, (long long int)tc->off, tc->wname, TST_RET,
			(long long int)tc->exp_off);
		return;
	}

	SAFE_READ(1, fd, read_buf, tc->exp_size);

	if (tc->exp_data && strcmp(read_buf, tc->exp_data)) {
		tst_res(TFAIL, "lseek(%s, %lld, %s) read incorrect data",
			TFILE, (long long int)tc->off, tc->wname);
	} else {
		tst_res(TPASS, "lseek(%s, %lld, %s) read correct data",
			TFILE, (long long int)tc->off, tc->wname);
	}
    write_coordination_file("0");
}

static void setup(void)
{
	fd = SAFE_OPEN(TFILE, O_RDWR | O_CREAT, 0644);

	SAFE_WRITE(SAFE_WRITE_ALL, fd, WRITE_STR, sizeof(WRITE_STR) - 1);
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_lseek,
	.needs_tmpdir = 1,
};
