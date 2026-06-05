/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "os-mm.h"
#include "syscall.h"
#include "libmem.h"
#include "queue.h"
#include "sched.h"
#include <stdlib.h>

#ifdef MM64
#include "mm64.h"
#else
#include "mm.h"
#endif

//typedef char BYTE;

static int caller_owns_phyaddr(struct pcb_t *caller, addr_t phyaddr)
{
   addr_t pgn;
   addr_t fpn = phyaddr / PAGING_PAGESZ;

   if (caller == NULL || caller->mm == NULL)
      return 0;
#ifdef MM64
   for (pgn = 0; pgn < PAGING64_MAX_PGN; pgn++) {
      uint32_t pte = (uint32_t)caller->mm->pt[pgn];
      if (PAGING_PAGE_PRESENT(pte) &&
          !(pte & PAGING_PTE_SWAPPED_MASK) &&
          PAGING_FPN(pte) == fpn)
         return 1;
   }
#else
   for (pgn = 0; pgn < PAGING_MAX_PGN; pgn++) {
      uint32_t pte = caller->mm->pgd[pgn];
      if (PAGING_PAGE_PRESENT(pte) &&
          !(pte & PAGING_PTE_SWAPPED_MASK) &&
          PAGING_FPN(pte) == fpn)
         return 1;
   }
#endif
   return 0;
}

int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
   int memop;
   int ret = -1;
   BYTE value;
   addr_t addr;
   struct pcb_t *caller;

   if (krnl == NULL || regs == NULL)
      return -1;
   memop = regs->a1;
   caller = find_proc_by_pid(pid);
   if (caller == NULL || caller->mm == NULL)
      return -1;

   switch (memop) {
   case SYSMEM_MAP_OP:
      return vmap_pgd_memset(caller, regs->a2, regs->a3);
   case SYSMEM_INC_OP:
      return inc_vma_limit(caller, regs->a2, regs->a3);
   case SYSMEM_SWP_OP:
      return __mm_swap_page(caller, regs->a2, regs->a3);
   case SYSMEM_IO_READ:
      if (!caller_owns_phyaddr(caller, regs->a2)) {
         printf("Kernel denied PID %u physical read at 0x%llx\n", pid,
                (unsigned long long)regs->a2);
         return -1;
      }
      ret = MEMPHY_read(krnl->mram, regs->a2, &value);
      regs->a3 = value;
      return ret;
   case SYSMEM_IO_WRITE:
      if (!caller_owns_phyaddr(caller, regs->a2)) {
         printf("Kernel denied PID %u physical write at 0x%llx\n", pid,
                (unsigned long long)regs->a2);
         return -1;
      }
      return MEMPHY_write(krnl->mram, regs->a2, regs->a3);
   case SYSMEM_ALLOC_OP:
      ret = __alloc(caller, regs->a2, regs->a3, regs->a4, &addr);
      regs->a4 = addr;
      return ret;
   case SYSMEM_FREE_OP:
      return __free(caller, regs->a2, regs->a3);
   case SYSMEM_READ_OP:
      ret = __read(caller, regs->a2, regs->a3, regs->a4, &value);
      regs->a4 = value;
      return ret;
   case SYSMEM_WRITE_OP:
      return __write(caller, regs->a2, regs->a3, regs->a4, regs->a5);
   case SYSMEM_KMALLOC_OP:
      ret = __kmalloc(caller, -1, regs->a2, regs->a3, &addr);
      regs->a4 = addr;
      return ret;
   case SYSMEM_CACHE_CREATE_OP:
      return __kmem_cache_pool_create(caller, regs->a2, regs->a3, regs->a4);
   case SYSMEM_CACHE_ALLOC_OP:
      ret = __kmem_cache_alloc(caller, -1, regs->a2, regs->a3, &addr);
      regs->a4 = addr;
      return ret;
   case SYSMEM_COPY_FROM_USER_OP:
      return __kmem_copy_from_user(caller, regs->a2, regs->a3,
                                   regs->a4, regs->a5);
   case SYSMEM_COPY_TO_USER_OP:
      return __kmem_copy_to_user(caller, regs->a2, regs->a3,
                                 regs->a4, regs->a5);
   default:
      return -1;
   }
}
