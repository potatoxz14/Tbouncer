// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Author: David Fenner, Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test checks that stat() executed on file provide the same information
 * of symlink linking to it.
 */

#include <stdlib.h>
#include "tst_test.h"
#include "tst_safe_stdio.h"

#define FILENAME "file.txt"
#define MNTPOINT "mntpoint"
#define SYMBNAME MNTPOINT"/file_symlink"

static char *symb_path;
static char *file_path;
static struct stat *file_stat;
static struct stat *symb_stat;

static void run(void)
{
    write_coordination_file("stat04");
	SAFE_STAT(file_path, file_stat);
	SAFE_STAT(symb_path, symb_stat);

	TST_EXP_EQ_LI(file_stat->st_dev, symb_stat->st_dev);
	TST_EXP_EQ_LI(file_stat->st_mode, symb_stat->st_mode);
	TST_EXP_EQ_LI(file_stat->st_nlink, symb_stat->st_nlink);
	TST_EXP_EQ_LI(file_stat->st_uid, symb_stat->st_uid);
	TST_EXP_EQ_LI(file_stat->st_gid, symb_stat->st_gid);
	TST_EXP_EQ_LI(file_stat->st_size, symb_stat->st_size);
	TST_EXP_EQ_LI(file_stat->st_atime, symb_stat->st_atime);
	TST_EXP_EQ_LI(file_stat->st_mtime, symb_stat->st_mtime);
	TST_EXP_EQ_LI(file_stat->st_ctime, symb_stat->st_ctime);
    write_coordination_file("0");
}

static void setup(void)
{
	char opt_bsize[32];
	const char *const fs_opts[] = {opt_bsize, NULL};
	struct stat sb;
	int pagesize;
	int fd;

	file_path = tst_tmpdir_genpath(FILENAME);
	symb_path = tst_tmpdir_genpath(SYMBNAME);

	/* change st_blksize / st_dev */
	SAFE_STAT(".", &sb);
	pagesize = sb.st_blksize == 4096 ? 1024 : 4096;

	snprintf(opt_bsize, sizeof(opt_bsize), "-b %i", pagesize);
	SAFE_MKFS(tst_device->dev, tst_device->fs_type, fs_opts, NULL);
	SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, 0, 0);

	SAFE_TOUCH(FILENAME, 0777, NULL);

	/* change st_nlink */
	SAFE_LINK(FILENAME, "linked_file");

	/* change st_uid and st_gid */
	SAFE_CHOWN(FILENAME, 1000, 1000);

	/* change st_size */
	fd = SAFE_OPEN(FILENAME, O_WRONLY, 0777);
	tst_fill_fd(fd, 'a', TST_KB, 500);
	SAFE_CLOSE(fd);

	/* change st_atime / st_mtime / st_ctime */
	usleep(1001000);

	SAFE_SYMLINK(file_path, symb_path);
}

static void cleanup(void)
{
	if (tst_is_mounted(MNTPOINT))
		SAFE_UMOUNT(MNTPOINT);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.needs_root = 1,
	.needs_device = 1,
	.mntpoint = MNTPOINT,
	.bufs = (struct tst_buffers []) {
		{&file_stat, .size = sizeof(struct stat)},
		{&symb_stat, .size = sizeof(struct stat)},
		{}
	}
};
