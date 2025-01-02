import os
import sys
import time
from bcc import BPF

# 检查参数
if len(sys.argv) != 2:
    print("Usage: python3 script.py <target_cgroup_id>")
    sys.exit(1)

# 获取传入的 cgroup_id
target_cgroup_id = int(sys.argv[1])

# 定义 eBPF 程序模板
bpf_program_template = """
#include <uapi/linux/ptrace.h>
#include <linux/sched.h>
#include <linux/errno.h>

BPF_HASH(cgroup_id_map, u32, u64);

static inline bool compare_cgroup_id(u64 cgroup_id) {
    u64 current_cgroup_id = bpf_get_current_cgroup_id();
    if (cgroup_id != current_cgroup_id)
        return false;

    return true;
}

int handle_sys_accept(struct pt_regs *ctx) {
    u64 target_cgroup = %d;
    if (!compare_cgroup_id(target_cgroup))
        return 0;

    // Override syscall return value to simulate failure (-4096)
    u64 success = bpf_override_return(ctx, -8192);
    return 0;
}
"""

# 用 target_cgroup_id 替换占位符 %d
bpf_program = bpf_program_template % target_cgroup_id
# bpf_program = bpf_program_template
# 初始化 BPF 程序
bpf = BPF(text=bpf_program)

# 文件路径
switch_file = "/dev/shm/switch_file"

# 确保 switch_file 存在
if not os.path.exists(switch_file):
    with open(switch_file, "w") as f:
        f.write("")

print("eBPF program loaded. Monitoring /dev/shm/switch_file for commands...")

# 持续监听文件并根据内容动态启用或禁用内核探针
try:
    attached = False  # 标记是否已附加 kretprobe

    while True:
        with open(switch_file, "r") as f:
            content = f.read().strip()

        if content == "iago_attack_accept" and not attached:
            print("Attaching kretprobe to sys_accept...")
            bpf.attach_kretprobe(event=bpf.get_syscall_fnname("accept"), fn_name="handle_sys_accept")
            bpf.attach_kretprobe(event=bpf.get_syscall_fnname("accept4"), fn_name="handle_sys_accept")
            attached = True
        elif content != "iago_attack_accept" and attached:
            print("Detaching kretprobe from sys_accept...")
            bpf.detach_kretprobe(event=bpf.get_syscall_fnname("accept"))
            bpf.detach_kretprobe(event=bpf.get_syscall_fnname("accept4"))
            attached = False

        # 稍作等待，避免频繁读取文件
        time.sleep(1)
except KeyboardInterrupt:
    if attached:
        bpf.detach_kretprobe(event=bpf.get_syscall_fnname("accept"))
        bpf.detach_kretprobe(event=bpf.get_syscall_fnname("accept4"))
    print("Detaching probes and exiting...")
