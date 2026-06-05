/*
 * Simple demonstration system call used by input/proc/sc3.
 */

#include "common.h"
#include "syscall.h"
#include <stdio.h>

int __sys_xxxhandler(struct krnl_t *krnl, uint32_t pid, struct sc_regs *regs)
{
  (void)krnl;
  printf("The first system call parameter %llu from PID %u\n",
         (unsigned long long)regs->a1, pid);
  return 0;
}
