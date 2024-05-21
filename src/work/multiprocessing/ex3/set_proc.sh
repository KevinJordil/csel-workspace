#/bin/sh

if [ $# -ne 2 ]
    then
        echo "Usage: $0 <high_pid> <low_pid>"
        exit 1
fi

HIGH_PID="$1"
LOW_PID="$2"

echo $HIGH_PID > /sys/fs/cgroup/cpuset/high/tasks
echo $LOW_PID > /sys/fs/cgroup/cpuset/low/tasks