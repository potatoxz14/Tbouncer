// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test verifies that ioctl() FICLONE/FICLONERANGE feature correctly raises
 * EOPNOTSUPP when an unsupported filesystem is used. In particular, filesystems
 * which don't support copy-on-write.
 */

#include "tst_test.h"
#include "lapi/ficlone.h"

#define MNTPOINT "mnt"
#define SRCPATH MNTPOINT "/file0"
#define DSTPATH MNTPOINT "/file1"

static struct file_clone_range *clone_range;

static void run(void)
{
    write_coordination_file("ioctl_ficlone02");
	int src_fd;
	int dst_fd;

	src_fd = SAFE_OPEN(SRCPATH, O_CREAT | O_RDWR, 0640);
	dst_fd = SAFE_OPEN(DSTPATH, O_CREAT | O_RDWR, 0640);

	clone_range->src_fd = src_fd;

	TST_EXP_FAIL(ioctl(dst_fd, FICLONE, src_fd), EOPNOTSUPP);
	TST_EXP_FAIL(ioctl(dst_fd, FICLONERANGE, clone_range), EOPNOTSUPP);

	SAFE_CLOSE(src_fd);
	SAFE_CLOSE(dst_fd);
    write_coordination_file("0");
}

static void setup(void)
{
	struct stat sb;

	SAFE_STAT(MNTPOINT, &sb);

	tst_fill_file(SRCPATH, 0x00, sb.st_blksize, 1);

	clone_range->src_offset = 0;
	clone_range->src_length = sb.st_blksize;
	clone_range->dest_offset = 0;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.min_kver = "4.5",
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *[]) {
		"bcachefs",
		"btrfs",
		"overlayfs",
		"nfs",
		"xfs",
		NULL,
	},
	.bufs = (struct tst_buffers []) {
		{&clone_range, .size = sizeof(struct file_clone_range)},
		{},
	}
};
