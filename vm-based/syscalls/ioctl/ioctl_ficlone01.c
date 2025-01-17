// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test verifies that ioctl() FICLONE feature clones file content from
 * one file to an another.
 *
 * [Algorithm]
 *
 * - populate source file
 * - clone source content inside destination file
 * - verify that source content has been cloned inside destination file
 * - write a single byte inside destination file
 * - verify that source content didn't change while destination did
 */

#include "tst_test.h"
#include "lapi/ficlone.h"

#define MNTPOINT "mnt"
#define SRCPATH MNTPOINT "/file0"
#define DSTPATH MNTPOINT "/file1"

#define FILEDATA "qwerty"
#define FILESIZE sizeof(FILEDATA)

static int src_fd = -1;
static int dst_fd = -1;

static void run(void)
{
    write_coordination_file("ioctl_ficlone01");
	char buff[FILESIZE];
	struct stat src_stat;
	struct stat dst_stat;

	src_fd = SAFE_OPEN(SRCPATH, O_CREAT | O_RDWR, 0640);
	dst_fd = SAFE_OPEN(DSTPATH, O_CREAT | O_RDWR, 0640);

	tst_res(TINFO, "Writing data inside src file");

	SAFE_WRITE(1, src_fd, FILEDATA, FILESIZE);
	SAFE_FSYNC(src_fd);

	TST_EXP_PASS(ioctl(dst_fd, FICLONE, src_fd));
	if (TST_RET == -1)
		return;

	SAFE_FSYNC(dst_fd);

	tst_res(TINFO, "Verifing that data is cloned between files");

	SAFE_FSTAT(src_fd, &src_stat);
	SAFE_FSTAT(dst_fd, &dst_stat);

	TST_EXP_EXPR(src_stat.st_ino != dst_stat.st_ino,
		"inode is different. %lu != %lu",
		src_stat.st_ino,
		dst_stat.st_ino);

	TST_EXP_EQ_LI(src_stat.st_size, dst_stat.st_size);

	SAFE_READ(0, dst_fd, buff, FILESIZE);

	TST_EXP_EXPR(!strncmp(buff, FILEDATA, FILESIZE),
		"dst file has the src file content (\"%s\" - %ld bytes)",
		buff,
		FILESIZE);

	tst_res(TINFO, "Writing a byte inside dst file");

	SAFE_LSEEK(dst_fd, 0, SEEK_SET);
	SAFE_WRITE(SAFE_WRITE_ALL, dst_fd, "+", 1);
	SAFE_FSYNC(dst_fd);

	tst_res(TINFO, "Verifing that src file content didn't change");

	SAFE_FSTAT(src_fd, &src_stat);
	SAFE_FSTAT(dst_fd, &dst_stat);

	TST_EXP_EQ_LI(dst_stat.st_size, src_stat.st_size);

	SAFE_READ(0, src_fd, buff, FILESIZE);

	TST_EXP_EXPR(!strncmp(buff, FILEDATA, FILESIZE),
		"src file content didn't change");

	SAFE_CLOSE(src_fd);
	SAFE_CLOSE(dst_fd);

	SAFE_UNLINK(SRCPATH);
	SAFE_UNLINK(DSTPATH);
    write_coordination_file("0");
}

static void cleanup(void)
{
	if (src_fd != -1)
		SAFE_CLOSE(src_fd);

	if (dst_fd != -1)
		SAFE_CLOSE(dst_fd);
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.min_kver = "4.5",
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.filesystems = (struct tst_fs []) {
		{.type = "btrfs"},
		{.type = "bcachefs"},
		{
			.type = "xfs",
			.min_kver = "4.16",
			.mkfs_ver = "mkfs.xfs >= 1.5.0",
			.mkfs_opts = (const char *const []) {"-m", "reflink=1", NULL},
		},
		{}
	}
};
