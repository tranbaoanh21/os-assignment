#!/bin/sh
set -eu

export MPLBACKEND=Agg
export MPLCONFIGDIR="${TMPDIR:-/tmp}/ossim-matplotlib"
export XDG_CACHE_HOME="${TMPDIR:-/tmp}/ossim-cache"

for test in \
  os_0_mlq_paging \
  os_1_mlq_paging \
  os_1_mlq_paging_small_1K \
  os_1_mlq_paging_small_4K \
  os_1_singleCPU_mlq \
  os_1_singleCPU_mlq_paging \
  os_2_mlq_paging \
  os_2_singleCPU_mlq_paging \
  os_sc \
  os_syscall \
  os_syscall_list \
  os_mm64_canonical \
  os_cross_pid_protection \
  os_mem_reuse \
  os_mem_roundtrip \
  os_mem_roundtrip_spec \
  os_mem_protection \
  os_paging_same_va \
  os_paging_boundary \
  sched \
  sched_0 \
  sched_1 \
  sched_prio \
  sched_out_of_slot \
  sched_2_cpu
do
  echo "Running $test"
  ./os "$test" > "output/$test.output"
done

sh tests/assert_outputs.sh

for test in sched_prio sched_out_of_slot sched_2_cpu
do
  python3 gantt.py "input/$test" "output/$test.output" "assets/$test.png"
done
