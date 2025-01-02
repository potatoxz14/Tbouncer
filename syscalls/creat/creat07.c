// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (c) 2012-2016 Cyril Hrubis <chrubis@suse.cz>
 */

/*
 * Testcase to check creat(2) sets ETXTBSY correctly.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include "tst_test.h"

#define TEST_APP "creat07_child"

static void verify_creat(void)
{
    write_coordination_file("creat07");
	pid_t pid;

	pid = SAFE_FORK();
	if (pid == 0) {
		SAFE_EXECL(TEST_APP, TEST_APP, NULL);
		exit(1);
	}

	TST_CHECKPOINT_WAIT(0);

	TEST(creat(TEST_APP, O_WRONLY));

	if (TST_RET != -1) {
		tst_res(TFAIL, "creat() succeeded unexpectedly");
		return;
	}

	if (TST_ERR == ETXTBSY)
		tst_res(TPASS, "creat() received EXTBSY");
	else
		tst_res(TFAIL | TTERRNO, "creat() failed unexpectedly");

	SAFE_KILL(pid, SIGKILL);
	SAFE_WAITPID(pid, NULL, 0);
    write_coordination_file("0");
}

static void setup(void)
{
	if ((tst_kvercmp(6, 11, 0)) >= 0) {
		tst_brk(TCONF, "Skipping test, write to executed file is "
			"allowed since 6.11-rc1.\n"
			"2a010c412853 (\"fs: don't block i_writecount during exec\")");
	}
}

static struct tst_test test = {
	.setup = setup,
	.test_all = verify_creat,
	.needs_checkpoints = 1,
	.forks_child = 1,
	.resource_files = (const char *const []) {
		TEST_APP,
		NULL
	}
};
