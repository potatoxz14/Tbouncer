// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported to LTP: Wayne Boyer
 * Copyright (c) 2016 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * [Description]
 *
 * Verify that, nice(2) fails when, a non-root user attempts to increase
 * the priority of a process by specifying a negative increment value.
 */
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include "tst_test.h"

#define NICEINC -10

static void verify_nice(void)
{
    write_coordination_file("nice04");
	TEST(nice(NICEINC));

	if (TST_RET != -1) {
		tst_res(TFAIL, "nice(%i) succeded unexpectedly (returned %li)",
			NICEINC, TST_RET);
		return;
	}

	if (TST_ERR != EPERM) {
		tst_res(TFAIL | TTERRNO, "nice(%i) should fail with EPERM",
			NICEINC);
		return;
	}

	tst_res(TPASS, "nice(%i) failed with EPERM", NICEINC);
    write_coordination_file("0");
}

static void setup(void)
{
	struct passwd *ltpuser;

	ltpuser = SAFE_GETPWNAM("nobody");
	SAFE_SETUID(ltpuser->pw_uid);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = verify_nice,
	.needs_root = 1,
};
