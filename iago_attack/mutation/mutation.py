import os
import sys
import time
from bcc import BPF
from inotify_simple import INotify, flags
# 加载 eBPF 程序
# 检查参数
if len(sys.argv) != 2:
    print("Usage: python3 script.py <target_cgroup_id>")
    sys.exit(1)

# 获取传入的 cgroup_id
target_cgroup_id = int(sys.argv[1])
bpf_program = f"""
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

int handle_retvalue(struct pt_regs *ctx) {{
    u64 target_cgroup = {target_cgroup_id};
    if (!compare_cgroup_id(target_cgroup))
        return 0;

    u64 success = bpf_override_return(ctx, 4099); 
    bpf_trace_printk("success = %d, success");
    return 0;
}}
"""
# bpf_program = bpf_program_template % target_cgroup_id
# 初始化 BPF 程序
bpf = BPF(text=bpf_program)

switch_file = "/dev/shm/switch_file"

# 确保 switch_file 存在
# if not os.path.exists(switch_file):
#     with open(switch_file, "w") as f:
#         f.write("")
inotify = INotify()
watch_flags = flags.MODIFY
wd = inotify.add_watch(switch_file, watch_flags)
print("eBPF program loaded. Monitoring /dev/shm/switch_file for commands...")

# 持续监听文件并根据内容动态启用或禁用内核探针
try:
    attached = False  # 标记是否已附加 kretprobe
    syscall_name = "NULL"
    prefix = "mutation_"
    saved_name = ""
    while True:
        events = inotify.read(timeout=1000)
        for event in events:
            if event.mask & flags.MODIFY:  # 检查修改事件
                with open(switch_file, "r") as f:
                    content = f.read().strip()
                
                if content.startswith(prefix):
                    syscall_name = content[len(prefix):]
                    
                    if not attached and syscall_name != "done":
                        saved_name = syscall_name
                        print(f"Attaching kretprobe to {syscall_name}...")
                        attached = True
                        bpf.attach_kretprobe(event = bpf.get_syscall_fnname(syscall_name), fn_name = "handle_retvalue")
                    
                    elif attached and syscall_name == "done":
                        print(f"Detaching kretprobe...")
                        attached = False
                        # bpf.detach_kretprobe(event=bpf.get_syscall_fnname(saved_name))
        
except KeyboardInterrupt:
    if attached:
        bpf.detach_kretprobe(event=bpf.get_syscall_fnname(syscall_name))
    print("Detaching probes and exiting...")
