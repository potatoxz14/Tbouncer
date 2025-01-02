import os
import sys
import time
from bcc import BPF
# 加载 eBPF 程序
# 检查参数
if len(sys.argv) != 2:
    print("Usage: python3 script.py <target_cgroup_id>")
    sys.exit(1)

# 获取传入的 cgroup_id
target_cgroup_id = int(sys.argv[1])
bpf_program_template = f"""
#include <uapi/linux/ptrace.h>
#include <linux/bpf.h>
#include <linux/sched.h>
#include <linux/errno.h>

BPF_HASH(cgroup_id_map, u32, u64);

static inline bool compare_cgroup_id(u64 cgroup_id) {{
    u64 current_cgroup_id = bpf_get_current_cgroup_id();
    if (cgroup_id != current_cgroup_id)
        return false;

    return true;
}}

int handle_readlink(struct pt_regs *ctx) {{
    u64 target_cgroup = {target_cgroup_id};
    u64 success = -1;
    if (!compare_cgroup_id(target_cgroup))
        return 0;
    bpf_trace_printk("handle_readlink");
    success = bpf_override_return(ctx, 100);
    bpf_trace_printk("success = %d", success);
    return 0;
}}
"""
bpf_program = bpf_program_template % target_cgroup_id
# 初始化 BPF 程序
bpf = BPF(text=bpf_program)

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

        if content == "iago_attack_readlink" and not attached:
            print("Attaching kretprobe to sys_readlink...")
            bpf.attach_kretprobe(event = bpf.get_syscall_fnname("readlink"), fn_name = "handle_readlink")
            attached = True
        elif content != "iago_attack_readlink" and attached:
            print("Detaching kretprobe from sys_readlink...")
            bpf.detach_kretprobe(event=bpf.get_syscall_fnname("readlink"))
            attached = False

        # 稍作等待，避免频繁读取文件
        time.sleep(1)
except KeyboardInterrupt:
    if attached:
        bpf.detach_kretprobe(event=bpf.get_syscall_fnname("readlink"))
    print("Detaching probes and exiting...")





# # 附加到 sys_accept 的内核探针
# bpf.attach_kretprobe(event = bpf.get_syscall_fnname("readlink"), fn_name = "handle_readlink")
# # bpf["myevents"].open_perf_buffer(print_event)
# print("eBPF program loaded. Press Ctrl+C to exit.")

# try:
#     bpf.trace_print()
# except KeyboardInterrupt:
#     print("Detaching probes and exiting...")
