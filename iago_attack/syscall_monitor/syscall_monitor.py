import os
import time
import threading
import ctypes as ct
from bcc import BPF
from inotify_simple import INotify, flags

# 定义开关文件路径
SWITCH_FILE = "/dev/shm/switch_file"

# 定义 eBPF 程序
bpf_program = """
#include <linux/binfmts.h>
#include <linux/ptrace.h>
#include <linux/cgroup-defs.h>
#include <linux/kernfs.h>
#include <linux/nsproxy.h>
#include <linux/mount.h>
#include <linux/ns_common.h>

int MAX_ARGS = 20;
  //过滤的进程名
struct data_t {
    u64 pid;
    u64 cgroup_id;
    u64 syscall_id;
    u64 timestamp; // 添加时间戳字段
    u64 num_args;
    char comm[TASK_COMM_LEN];
    void *args[20];
};

BPF_ARRAY(switch_state, int, 1);
BPF_PERF_OUTPUT(myevents); // 注册events

static inline bool compare_cgroup_id(u64 cgroup_id) {
    u64 current_cgroup_id = bpf_get_current_cgroup_id();
    if (cgroup_id != current_cgroup_id)
            return false;

    return true;
}


TRACEPOINT_PROBE(raw_syscalls, sys_enter) {
    struct data_t data = {};
    int index = 0;
    u64 target_cgroup = 27724;
    int *value = switch_state.lookup(&index);
    if (!value)
       return 0;

    if(!compare_cgroup_id(target_cgroup))
        return 0;

    data.pid = bpf_get_current_pid_tgid() >> 32; // 获取pid
    data.syscall_id = args->id; // 获取系统调用ID
    data.cgroup_id = bpf_get_current_cgroup_id();
    bpf_trace_printk("cgroup_id = %d\\n", data.cgroup_id);
    data.timestamp = bpf_ktime_get_ns(); // 获取时间戳

    myevents.perf_submit(args, &data, sizeof(data)); // 将data结构体的内容发送到用户空间的events性能事件输出
    return 0;
}
"""
syscall_names = {
    0: b"read",
    1: b"write",
    2: b"open",
    3: b"close",
    4: b"stat",
    5: b"fstat",
    6: b"lstat",
    7: b"poll",
    8: b"lseek",
    9: b"mmap",
    10: b"mprotect",
    11: b"munmap",
    12: b"brk",
    13: b"rt_sigaction",
    14: b"rt_sigprocmask",
    15: b"rt_sigreturn",
    16: b"ioctl",
    17: b"pread64",
    18: b"pwrite64",
    19: b"readv",
    20: b"writev",
    21: b"access",
    22: b"pipe",
    23: b"select",
    24: b"sched_yield",
    25: b"mremap",
    26: b"msync",
    27: b"mincore",
    28: b"madvise",
    29: b"shmget",
    30: b"shmat",
    31: b"shmctl",
    32: b"dup",
    33: b"dup2",
    34: b"pause",
    35: b"nanosleep",
    36: b"getitimer",
    37: b"alarm",
    38: b"setitimer",
    39: b"getpid",
    40: b"sendfile",
    41: b"socket",
    42: b"connect",
    43: b"accept",
    44: b"sendto",
    45: b"recvfrom",
    46: b"sendmsg",
    47: b"recvmsg",
    48: b"shutdown",
    49: b"bind",
    50: b"listen",
    51: b"getsockname",
    52: b"getpeername",
    53: b"socketpair",
    54: b"setsockopt",
    55: b"getsockopt",
    56: b"clone",
    57: b"fork",
    58: b"vfork",
    59: b"execve",
    60: b"exit",
    61: b"wait4",
    62: b"kill",
    63: b"uname",
    64: b"semget",
    65: b"semop",
    66: b"semctl",
    67: b"shmdt",
    68: b"msgget",
    69: b"msgsnd",
    70: b"msgrcv",
    71: b"msgctl",
    72: b"fcntl",
    73: b"flock",
    74: b"fsync",
    75: b"fdatasync",
    76: b"truncate",
    77: b"ftruncate",
    78: b"getdents",
    79: b"getcwd",
    80: b"chdir",
    81: b"fchdir",
    82: b"rename",
    83: b"mkdir",
    84: b"rmdir",
    85: b"creat",
    86: b"link",
    87: b"unlink",
    88: b"symlink",
    89: b"readlink",
    90: b"chmod",
    91: b"fchmod",
    92: b"chown",
    93: b"fchown",
    94: b"lchown",
    95: b"umask",
    96: b"gettimeofday",
    97: b"getrlimit",
    98: b"getrusage",
    99: b"sysinfo",
    100: b"times",
    101: b"ptrace",
    102: b"getuid",
    103: b"syslog",
    104: b"getgid",
    105: b"setuid",
    106: b"setgid",
    107: b"geteuid",
    108: b"getegid",
    109: b"setpgid",
    110: b"getppid",
    111: b"getpgrp",
    112: b"setsid",
    113: b"setreuid",
    114: b"setregid",
    115: b"getgroups",
    116: b"setgroups",
    117: b"setresuid",
    118: b"getresuid",
    119: b"setresgid",
    120: b"getresgid",
    121: b"getpgid",
    122: b"setfsuid",
    123: b"setfsgid",
    124: b"getsid",
    125: b"capget",
    126: b"capset",
    127: b"rt_sigpending",
    128: b"rt_sigtimedwait",
    129: b"rt_sigqueueinfo",
    130: b"rt_sigsuspend",
    131: b"sigaltstack",
    132: b"utime",
    133: b"mknod",
    134: b"uselib",
    135: b"personality",
    136: b"ustat",
    137: b"statfs",
    138: b"fstatfs",
    139: b"sysfs",
    140: b"getpriority",
    141: b"setpriority",
    142: b"sched_setparam",
    143: b"sched_getparam",
    144: b"sched_setscheduler",
    145: b"sched_getscheduler",
    146: b"sched_get_priority_max",
    147: b"sched_get_priority_min",
    148: b"sched_rr_get_interval",
    149: b"mlock",
    150: b"munlock",
    151: b"mlockall",
    152: b"munlockall",
    153: b"vhangup",
    154: b"modify_ldt",
    155: b"pivot_root",
    156: b"_sysctl",
    157: b"prctl",
    158: b"arch_prctl",
    159: b"adjtimex",
    160: b"setrlimit",
    161: b"chroot",
    162: b"sync",
    163: b"acct",
    164: b"settimeofday",
    165: b"mount",
    166: b"umount2",
    167: b"swapon",
    168: b"swapoff",
    169: b"reboot",
    170: b"sethostname",
    171: b"setdomainname",
    172: b"iopl",
    173: b"ioperm",
    174: b"create_module",
    175: b"init_module",
    176: b"delete_module",
    177: b"get_kernel_syms",
    178: b"query_module",
    179: b"quotactl",
    180: b"nfsservctl",
    181: b"getpmsg",
    182: b"putpmsg",
    183: b"afs_syscall",
    184: b"tuxcall",
    185: b"security",
    186: b"gettid",
    187: b"readahead",
    188: b"setxattr",
    189: b"lsetxattr",
    190: b"fsetxattr",
    191: b"getxattr",
    192: b"lgetxattr",
    193: b"fgetxattr",
    194: b"listxattr",
    195: b"llistxattr",
    196: b"flistxattr",
    197: b"removexattr",
    198: b"lremovexattr",
    199: b"fremovexattr",
    200: b"tkill",
    201: b"time",
    202: b"futex",
    203: b"sched_setaffinity",
    204: b"sched_getaffinity",
    205: b"set_thread_area",
    206: b"io_setup",
    207: b"io_destroy",
    208: b"io_getevents",
    209: b"io_submit",
    210: b"io_cancel",
    211: b"get_thread_area",
    212: b"lookup_dcookie",
    213: b"epoll_create",
    214: b"epoll_ctl_old",
    215: b"epoll_wait_old",
    216: b"remap_file_pages",
    217: b"getdents64",
    218: b"set_tid_address",
    219: b"restart_syscall",
    220: b"semtimedop",
    221: b"fadvise64",
    222: b"timer_create",
    223: b"timer_settime",
    224: b"timer_gettime",
    225: b"timer_getoverrun",
    226: b"timer_delete",
    227: b"clock_settime",
    228: b"clock_gettime",
    229: b"clock_getres",
    230: b"clock_nanosleep",
    231: b"exit_group",
    232: b"epoll_wait",
    233: b"epoll_ctl",
    234: b"tgkill",
    235: b"utimes",
    236: b"vserver",
    237: b"mbind",
    238: b"set_mempolicy",
    239: b"get_mempolicy",
    240: b"mq_open",
    241: b"mq_unlink",
    242: b"mq_timedsend",
    243: b"mq_timedreceive",
    244: b"mq_notify",
    245: b"mq_getsetattr",
    246: b"kexec_load",
    247: b"waitid",
    248: b"add_key",
    249: b"request_key",
    250: b"keyctl",
    251: b"ioprio_set",
    252: b"ioprio_get",
    253: b"inotify_init",
    254: b"inotify_add_watch",
    255: b"inotify_rm_watch",
    256: b"migrate_pages",
    257: b"openat",
    258: b"mkdirat",
    259: b"mknodat",
    260: b"fchownat",
    261: b"futimesat",
    262: b"newfstatat",
    263: b"unlinkat",
    264: b"renameat",
    265: b"linkat",
    266: b"symlinkat",
    267: b"readlinkat",
    268: b"fchmodat",
    269: b"faccessat",
    270: b"pselect6",
    271: b"ppoll",
    272: b"unshare",
    273: b"set_robust_list",
    274: b"get_robust_list",
    275: b"splice",
    276: b"tee",
    277: b"sync_file_range",
    278: b"vmsplice",
    279: b"move_pages",
    280: b"utimensat",
    281: b"epoll_pwait",
    282: b"signalfd",
    283: b"timerfd_create",
    284: b"eventfd",
    285: b"fallocate",
    286: b"timerfd_settime",
    287: b"timerfd_gettime",
    288: b"accept4",
    289: b"signalfd4",
    290: b"eventfd2",
    291: b"epoll_create1",
    292: b"dup3",
    293: b"pipe2",
    294: b"inotify_init1",
    295: b"preadv",
    296: b"pwritev",
    297: b"rt_tgsigqueueinfo",
    298: b"perf_event_open",
    299: b"recvmmsg",
    300: b"fanotify_init",
    301: b"fanotify_mark",
    302: b"prlimit64",
    303: b"name_to_handle_at",
    304: b"open_by_handle_at",
    305: b"clock_adjtime",
    306: b"syncfs",
    307: b"sendmmsg",
    308: b"setns",
    309: b"getcpu",
    310: b"process_vm_readv",
    311: b"process_vm_writev",
    312: b"kcmp",
    313: b"finit_module",
    314: b"sched_setattr",
    315: b"sched_getattr",
    316: b"renameat2",
    317: b"seccomp",
    318: b"getrandom",
    319: b"memfd_create",
    320: b"kexec_file_load",
    321: b"bpf",
    322: b"execveat",
    323: b"userfaultfd",
    324: b"membarrier",
    325: b"mlock2",
    326: b"copy_file_range",
    327: b"preadv2",
    328: b"pwritev2",
    329: b"pkey_mprotect",
    330: b"pkey_alloc",
    331: b"pkey_free",
    332: b"statx",
    333: b"io_pgetevents",
    334: b"rseq",
}
bpf = BPF(text=bpf_program)
is_monitoring = False
# 方法：读取开关文件内容
def read_switch_file():
    try:
        with open(SWITCH_FILE, "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        return "0"

# 线程1：监听 eBPF 事件
def ebpf_event_listener():
    print("Starting eBPF event listener...")
    while True:
        bpf.perf_buffer_poll()

# 线程2：监控文件并修改 bpf 状态
def file_monitor_and_update():
    inotify = INotify()
    inotify.add_watch(SWITCH_FILE, flags.MODIFY)
    global is_monitoring

    print(f"Monitoring file: {SWITCH_FILE}")
    while True:
        for event in inotify.read():  # 等待文件变化事件
            if event.mask & flags.MODIFY:
                switch_value = read_switch_file()
                if switch_value != '0' and not is_monitoring:
                    print(f"\n\n\n\nStarting system call monitoring, {switch_value}...", flush=True)
                    is_monitoring = True
                    bpf["switch_state"][ct.c_int(0)] = ct.c_int(1)
                elif switch_value == '0' and is_monitoring:
                    is_monitoring = False
                    bpf["switch_state"][ct.c_int(0)] = ct.c_int(0)
                    print("Stopping system call monitoring...")

# 监听系统调用的逻辑
def handle_event(cpu, data, size):
    global is_monitoring
    if not is_monitoring:  # 如果不是监听状态，忽略事件
        return
    event = bpf["myevents"].event(data)
    syscall_name = syscall_names.get(event.syscall_id, "unknown")
    print(f"PID: {event.pid}, Comm: {event.comm.decode()}, Syscall_name: {syscall_name}, Syscall: {event.syscall_id}")



# 初始化 inotify 监听文件
# 主函数
if __name__ == "__main__":
    # 启动两个线程
    print("Monitoring switch file for changes...")
    bpf["myevents"].open_perf_buffer(handle_event)
    bpf["switch_state"][ct.c_int(0)] = ct.c_int(0) 

    ebpf_thread = threading.Thread(target=ebpf_event_listener)
    file_monitor_thread = threading.Thread(target=file_monitor_and_update)

    ebpf_thread.start()
    file_monitor_thread.start()

    try:
        ebpf_thread.join()
        file_monitor_thread.join()
    except KeyboardInterrupt:
        print("Exiting...")