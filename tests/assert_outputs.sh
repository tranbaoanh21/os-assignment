#!/bin/sh
set -eu

assert_contains() {
  file=$1
  pattern=$2
  description=$3
  if ! grep -Fq "$pattern" "$file"; then
    echo "ASSERTION FAILED: $description" >&2
    echo "  missing '$pattern' in $file" >&2
    exit 1
  fi
}

for file in output/*.output; do
  test -s "$file" || {
    echo "ASSERTION FAILED: empty output $file" >&2
    exit 1
  }
done

assert_contains output/os_mem_protection.output \
  "Kernel denied PID 1 physical read" "kernel must reject raw physical reads"
assert_contains output/os_mem_protection.output \
  "Kernel denied PID 1 physical write" "kernel must reject raw physical writes"
assert_contains output/os_cross_pid_protection.output \
  "MEM alloc PID 1 region 1 [0,100)" "owner process must allocate frame zero"
assert_contains output/os_cross_pid_protection.output \
  "Kernel denied PID 2 physical read at 0x0" \
  "one PID must not read a physical frame owned by another PID"
assert_contains output/os_cross_pid_protection.output \
  "Kernel denied PID 2 physical write at 0x0" \
  "one PID must not write a physical frame owned by another PID"
assert_contains output/os_mem_reuse.output \
  "MEM free PID 1 region 1 [0,100)" "free must return the data region"
assert_contains output/os_mem_reuse.output \
  "MEM alloc PID 1 region 2 [0,50)" "allocation must reuse a freed data region"
assert_contains output/os_mm64_canonical.output \
  "indexes=[0,0,0,1,0]" "MM64 must allocate and walk a non-zero PMD branch"
assert_contains output/os_mm64_canonical.output \
  "indexes=[256,0,0,0,0]" "MM64 must accept a high canonical address"
assert_contains output/os_mm64_canonical.output \
  "MM64 rejected non-canonical address" "MM64 must reject non-canonical addresses"
assert_contains output/os_mem_roundtrip.output \
  "MEM read PID 1 region 3 offset 0 value 65" \
  "user-kernel-user copy must preserve the byte value"
assert_contains output/os_mem_roundtrip_spec.output \
  "MEM read PID 1 region 3 offset 0 value 65" \
  "spec-form copy instructions must preserve the byte value"
assert_contains output/os_paging_same_va.output \
  "MEM read PID 1 region 1 offset 0 value 65" \
  "PID 1 must preserve its value at virtual address zero"
assert_contains output/os_paging_same_va.output \
  "MEM read PID 2 region 1 offset 0 value 66" \
  "PID 2 must preserve a different value at the same virtual address"
assert_contains output/os_paging_boundary.output \
  "MEM read PID 1 region 1 offset 255 value 65" \
  "last byte of the first data page must preserve its value"
assert_contains output/os_paging_boundary.output \
  "MEM read PID 1 region 1 offset 256 value 66" \
  "first byte of the second data page must preserve its value"
assert_contains output/os_syscall_list.output "0-sys_listsyscall" \
  "syscall table must include listsyscall"
assert_contains output/os_syscall_list.output "17-sys_memmap" \
  "syscall table must include memmap"
assert_contains output/os_syscall_list.output "440-sys_xxxhandler" \
  "syscall table must include xxxhandler"

echo "All semantic output assertions passed."
