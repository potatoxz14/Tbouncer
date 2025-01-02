// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) Jiri Palecek<jpalecek@web.de>, 2009 */

#include "libsigwait.h"

static int my_sigwaitinfo(const sigset_t * set, siginfo_t * info,
			  void *timeout LTP_ATTRIBUTE_UNUSED)
{
	return sigwaitinfo(set, info);
}

struct sigwait_test_desc tests[] = {
	{ test_empty_set, SIGUSR1},
	{ test_unmasked_matching, SIGUSR1},
	{ test_masked_matching, SIGUSR1},
	{ test_unmasked_matching_noinfo, SIGUSR1},
	{ test_masked_matching_noinfo, SIGUSR1},
	{ test_bad_address, SIGUSR1},
	{ test_bad_address2, SIGUSR1},
};

static void run(unsigned int i)
{
    write_coordination_file("sigwaitinfo01");
	struct sigwait_test_desc *tc = &tests[i];

	tc->tf(my_sigwaitinfo, tc->signo, TST_LIBC_TIMESPEC);
    write_coordination_file("0");
}

static struct tst_test test = {
	.test= run,
	.tcnt = ARRAY_SIZE(tests),
	.setup = sigwait_setup,
	.forks_child = 1,
};
