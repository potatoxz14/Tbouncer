// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * AUTHOR: William Roske, CO-PILOT: Dave Fenner
 * Copyright (c) 2023 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that getpgrp(2) syscall executes successfully.
 */

#include "tst_test.h"

static void run(void)
{
    write_coordination_file("getpgrp01");
	TST_EXP_PID(getpgrp());
	TST_EXP_EQ_LI(TST_RET, SAFE_GETPGID(0));
    write_coordination_file("0");
}

static struct tst_test test = {
	.test_all = run
};
