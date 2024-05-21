#/bin/sh

mount -t tmpfs none /sys/fs/cgroup
mkdir /sys/fs/cgroup/memory
mount -t cgroup -o memory memory /sys/fs/cgroup/memory
mkdir /sys/fs/cgroup/memory/mem
echo $PPID > /sys/fs/cgroup/memory/mem/tasks
echo 20M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes
# echo 1 > /sys/fs/cgroup/memory/mem/memory.oom_control