// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test is checking if waitid() syscall recognizes a process that has been
 * killed with SIGKILL.
 */

#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "tst_test.h"

static siginfo_t *infop;

static void run(void)
{
    write_coordination_file("waitid11");
	pid_t pidchild;

	pidchild = SAFE_FORK();
	if (!pidchild) {
		pause();
		return;
	}

	SAFE_KILL(pidchild, SIGKILL);

	TST_EXP_PASS(waitid(P_ALL, 0, infop, WEXITED));
	TST_EXP_EQ_LI(infop->si_pid, pidchild);
	TST_EXP_EQ_LI(infop->si_status, SIGKILL);
	TST_EXP_EQ_LI(infop->si_signo, SIGCHLD);
	TST_EXP_EQ_LI(infop->si_code, CLD_KILLED);
    write_coordination_file("0");
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.bufs =	(struct tst_buffers[]) {
		{&infop, .size = sizeof(*infop)},
		{},
	},
};
