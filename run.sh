#!/bin/sh
set -eu

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
  os_mem_protection \
  sched \
  sched_0 \
  sched_1
do
  echo "Running $test"
  ./os "$test" > "output/$test.output"
done
