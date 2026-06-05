/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c 
 */

#include "string.h"
#include "mm.h"
#include "mm64.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

static int mem_syscall(struct pcb_t *proc, struct sc_regs *regs)
{
  if (proc == NULL || proc->krnl == NULL || regs == NULL)
    return -1;
  return _syscall(proc->krnl, proc->pid, 17, regs);
}

static int valid_region_id(int rgid)
{
  return rgid >= 0 && rgid < PAGING_MAX_SYMTBL_SZ;
}

static struct vm_rg_struct *get_ksymrg_byid(struct mm_struct *mm, int rgid)
{
  if (!valid_region_id(rgid))
    return NULL;
  return &mm->ksymrgtbl[rgid];
}

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (!valid_region_id(rgid))
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  /*Allocate at the toproof */
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (!valid_region_id(rgid) || cur_vma == NULL || size == 0) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /* Attempt to increase the VMA limit to get space. */
  addr_t old_sbrk;
  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * SYSCALL 1 sys_memmap
   */
  inc_vma_limit(caller, vmaid, size);
  if (cur_vma->sbrk == old_sbrk) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;

}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  pthread_mutex_lock(&mmvm_lock);

  if (!valid_region_id(rgid))
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  struct vm_rg_struct *krgnode = get_ksymrg_byid(caller->mm, rgid);
  if (caller->regs[rgid] >= PAGING_KMEM_BASE &&
      krgnode != NULL && krgnode->rg_start != 0 && krgnode->rg_end != 0)
  {
    int page;
    for (page = 0; page < caller->mm->kphy_npages[rgid]; page++)
      MEMPHY_put_freefp(caller->krnl->mram,
                        caller->mm->kphy_start[rgid] / PAGING_PAGESZ + page);
    caller->mm->kphy_start[rgid] = 0;
    caller->mm->kphy_npages[rgid] = 0;
    krgnode->rg_start = krgnode->rg_end = 0;
    krgnode->rg_next = NULL;
    caller->regs[rgid] = 0;
    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *rgnode = get_symrg_byid(caller->mm, rgid);

  if (rgnode->rg_start == 0 && rgnode->rg_end == 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  struct vm_rg_struct *freerg_node = malloc(sizeof(struct vm_rg_struct));
  freerg_node->rg_start = rgnode->rg_start;
  freerg_node->rg_end = rgnode->rg_end;
  freerg_node->rg_next = NULL;

  rgnode->rg_start = rgnode->rg_end = 0;
  rgnode->rg_next = NULL;
  caller->regs[rgid] = 0;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, freerg_node);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  struct sc_regs regs = {0};
  int val;

  if (!valid_region_id((int)reg_index))
    return -1;
  regs.a1 = SYSMEM_ALLOC_OP;
  regs.a2 = 0;
  regs.a3 = reg_index;
  regs.a4 = size;
  val = mem_syscall(proc, &regs);
  if (val != 0)
    return -1;
  proc->regs[reg_index] = regs.a4;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  /* By default using vmaid = 0 */
  return val;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  struct sc_regs regs = {0};

  if (!valid_region_id((int)reg_index))
    return -1;
  regs.a1 = SYSMEM_FREE_OP;
  regs.a2 = 0;
  regs.a3 = reg_index;
  if (mem_syscall(proc, &regs) != 0)
    return -1;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif
  return 0;
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = pte_get_entry(caller, pgn);

  if (!PAGING_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    addr_t vicpgn, swpfpn;
    addr_t vicfpn;
    uint32_t vicpte;
    struct sc_regs regs;

    /* Find victim page using FIFO */
    if (find_victim_page(caller->mm, &vicpgn) == -1)
    {
      return -1;
    }

    /* Get free frame in MEMSWP */
    if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
    {
      return -1; /* Out of swap space */
    }

    /* Lấy Frame vật lý của victim page */
    vicpte = pte_get_entry(caller, vicpgn);
    vicfpn = PAGING_FPN(vicpte);

    /* Đẩy victim frame từ RAM ra SWAP 
     * SYSCALL 17 sys_memmap (SYSMEM_SWP_OP)
     */
    regs.a1 = SYSMEM_SWP_OP; 
    regs.a2 = vicfpn;        /* Source: RAM frame */
    regs.a3 = swpfpn;        /* Dest: SWAP frame */
    _syscall(caller->krnl, caller->pid, 17, &regs);

    /* Cập nhật Page Table cho victim page (đánh dấu đã ra SWAP) */
    pte_set_swap(caller, vicpgn, 0, swpfpn);

    /* Trang mục tiêu (target page) giờ sẽ chiếm chỗ vicfpn vừa trống */
    addr_t tgtfpn = vicfpn;

    /* Nếu target page trước đó đang nằm trong SWAP, mang nó trở lại RAM */
    if (pte & PAGING_PTE_SWAPPED_MASK) {
        addr_t tgt_swpfpn = PAGING_SWP(pte);
        
        regs.a1 = SYSMEM_SWP_OP;
        regs.a2 = tgt_swpfpn; /* Lấy từ SWAP... */
        regs.a3 = tgtfpn;     /* ...bỏ vào RAM */
        _syscall(caller->krnl, caller->pid, 17, &regs);
        
        /* Giải phóng slot trong SWAP */
        MEMPHY_put_freefp(caller->krnl->active_mswp, tgt_swpfpn); 
    }

    /* Cập nhật trạng thái online cho target page */
    pte_set_fpn(caller, pgn, tgtfpn);

    /* Đưa target page vào danh sách theo dõi FIFO để làm victim cho lần sau */
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(pte_get_entry(caller,pgn));

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, addr_t addr, BYTE *data, struct pcb_t *caller)
{
  addr_t pgn = PAGING_PGN(addr);
  addr_t off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  /* Tính toán địa chỉ vật lý thật */
  addr_t phyaddr = ((addr_t)fpn << PAGING_ADDR_FPN_LOBIT) + off;

  /* Gọi SYSCALL 17 (SYSMEM_IO_READ) để yêu cầu Kernel đọc từ mram */
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0)
    return -1;
  
  /* Lấy dữ liệu trả về từ thanh ghi a3 */
  *data = regs.a3;

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, addr_t addr, BYTE value, struct pcb_t *caller)
{
  addr_t pgn = PAGING_PGN(addr);
  addr_t off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  /* Tính toán địa chỉ vật lý thật */
  addr_t phyaddr = ((addr_t)fpn << PAGING_ADDR_FPN_LOBIT) + off;

  /* Gọi SYSCALL 17 (SYSMEM_IO_WRITE) để yêu cầu Kernel ghi xuống mram */
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;
  if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0)
    return -1;

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

//struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || currg->rg_start == currg->rg_end ||
      currg->rg_start + offset >= currg->rg_end)
    return -1;

  if (pg_getval(caller->mm, currg->rg_start + offset, data, caller) != 0)
    return -1;

  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    addr_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  struct sc_regs regs = {0};
  int val;

  regs.a1 = SYSMEM_READ_OP;
  regs.a2 = 0;
  regs.a3 = source;
  regs.a4 = offset;
  val = mem_syscall(proc, &regs);
  if (val != 0)
    return val;

  *destination = regs.a4;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (currg->rg_start == currg->rg_end ||
      currg->rg_start + offset >= currg->rg_end)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  pthread_mutex_unlock(&mmvm_lock);
  if (pg_setval(caller->mm, currg->rg_start + offset, value, caller) != 0) {
    return -1;
  }
  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    addr_t offset)
{
  struct sc_regs regs = {0};
  int val;

  regs.a1 = SYSMEM_WRITE_OP;
  regs.a2 = 0;
  regs.a3 = destination;
  regs.a4 = offset;
  regs.a5 = data;
  val = mem_syscall(proc, &regs);
  if (val != 0)
    return -1;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return val;
}


/*libkmem_malloc- alloc region memory in kmem
 *@caller: caller
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 */

int libkmem_malloc(struct pcb_t * caller, uint32_t size, uint32_t reg_index)
{
  struct sc_regs regs = {0};

  if (!valid_region_id((int)reg_index))
    return -1;
  regs.a1 = SYSMEM_KMALLOC_OP;
  regs.a2 = reg_index;
  regs.a3 = size;
  if (mem_syscall(caller, &regs) != 0)
    return -1;

  caller->regs[reg_index] = regs.a4;
  return 0;
}


/*kmalloc - alloc region memory in kmem
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 *@alloc_addr: allocated address
 */
addr_t __kmalloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  struct mm_struct *mm = caller->mm;
  struct vm_rg_struct *krg;
  addr_t aligned;
  addr_t start_fpn;
  int page_count;

  (void)vmaid;
  if (mm == NULL || !valid_region_id(rgid) || size == 0 || alloc_addr == NULL)
    return -1;

  pthread_mutex_lock(&mmvm_lock);
  aligned = PAGING_PAGE_ALIGNSZ(size);
  page_count = (int)(aligned / PAGING_PAGESZ);
  if (mm->kmem_sbrk < PAGING_KMEM_BASE)
    mm->kmem_sbrk = PAGING_KMEM_BASE;
  if (mm->kmem_sbrk + aligned > PAGING_KMEM_LIMIT) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  krg = get_ksymrg_byid(mm, rgid);
  if (krg->rg_start != 0 && krg->rg_end != 0) {
    int page;
    for (page = 0; page < mm->kphy_npages[rgid]; page++)
      MEMPHY_put_freefp(caller->krnl->mram,
                        mm->kphy_start[rgid] / PAGING_PAGESZ + page);
    krg->rg_start = krg->rg_end = 0;
    mm->kphy_start[rgid] = 0;
    mm->kphy_npages[rgid] = 0;
  }
  if (MEMPHY_get_contiguous_fps(caller->krnl->mram, page_count,
                                &start_fpn) != 0) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  krg->vmaid = -1;
  krg->rg_start = mm->kmem_sbrk;
  krg->rg_end = mm->kmem_sbrk + size;
  krg->rg_next = NULL;
  *alloc_addr = krg->rg_start;
  mm->kphy_start[rgid] = start_fpn * PAGING_PAGESZ;
  mm->kphy_npages[rgid] = page_count;
  mm->kmem_sbrk += aligned;
  pthread_mutex_unlock(&mmvm_lock);
  return 0;

}

/*libkmem_cache_pool_create - create cache pool in kmem
 *@caller: caller
 *@size: memory size
 *@align: alignment size of each cache slot (identical cache slot size)
 *@cache_pool_id: cache pool ID
 */
int __kmem_cache_pool_create(struct pcb_t *caller, uint32_t size, uint32_t align, uint32_t cache_pool_id)
{
  struct kcache_pool_struct *pool;
  addr_t storage;

  if (!valid_region_id((int)cache_pool_id) || size == 0 || align == 0)
    return -1;

  pool = &caller->mm->kcpooltbl[cache_pool_id];
  if (__kmalloc(caller, -1, cache_pool_id, size, &storage) != 0)
    return -1;

  pool->size = size;
  pool->align = align;
  pool->used = 0;
  pool->capacity = size / align;
  pool->storage = storage;
  pool->physical_storage = caller->mm->kphy_start[cache_pool_id];
  return 0;
}

/*libkmem_cache_alloc - allocate cache slot in cache pool, cache slot has identical size
 * the allocated size is embedded in pool management mechanism
 *@caller: caller
 *@cache_pool_id: cache pool ID
 *@reg_index: memory region index
 */
int libkmem_cache_alloc(struct pcb_t *proc, uint32_t reg_index, uint32_t cache_pool_id)
{
  struct sc_regs regs = {0};

  if (!valid_region_id((int)reg_index) || !valid_region_id((int)cache_pool_id))
    return -1;
  regs.a1 = SYSMEM_CACHE_ALLOC_OP;
  regs.a2 = reg_index;
  regs.a3 = cache_pool_id;
  if (mem_syscall(proc, &regs) != 0)
    return -1;

  proc->regs[reg_index] = regs.a4;
  return 0;
}

/*kmem_cache_alloc - alloc region memory in kmem cache
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@cache_pool_id: cached pool ID
 *@alloc_addr: allocated address
 */

addr_t __kmem_cache_alloc(struct pcb_t *caller, int vmaid, int rgid, int cache_pool_id, addr_t *alloc_addr)
{
  struct kcache_pool_struct *pool;
  struct vm_rg_struct *krg;
  addr_t addr;

  (void)vmaid;
  if (caller->mm == NULL || !valid_region_id(rgid) ||
      !valid_region_id(cache_pool_id) || alloc_addr == NULL)
    return -1;

  pool = &caller->mm->kcpooltbl[cache_pool_id];
  if (pool->storage == 0 || pool->align <= 0 ||
      pool->used >= pool->capacity)
    return -1;

  pthread_mutex_lock(&mmvm_lock);
  addr = pool->storage + (addr_t)pool->used * (addr_t)pool->align;
  pool->used++;
  krg = get_ksymrg_byid(caller->mm, rgid);
  krg->vmaid = -1;
  krg->rg_start = addr;
  krg->rg_end = addr + pool->align;
  krg->rg_next = NULL;
  caller->mm->kphy_start[rgid] =
      pool->physical_storage + (addr_t)(pool->used - 1) * pool->align;
  caller->mm->kphy_npages[rgid] = 0;
  *alloc_addr = addr;
  pthread_mutex_unlock(&mmvm_lock);
  return 0;

}


int __kmem_copy_from_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  BYTE data;
  uint32_t i;
  struct vm_rg_struct *dst = get_ksymrg_byid(caller->mm, destination);
  addr_t addr;

  if (dst == NULL)
    return -1;
  if (dst->rg_start == dst->rg_end &&
      __kmalloc(caller, -1, destination, size, &addr) != 0)
    return -1;

  for (i = 0; i < size; i++) {
    if (__read_user_mem(caller, 0, source, offset + i, &data) != 0)
      return -1;
    if (__write_kernel_mem(caller, -1, destination, i, data) != 0)
      return -1;
  }
  return 0;
}

int __kmem_copy_to_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  BYTE data;
  uint32_t i;
  struct vm_rg_struct *dst = get_symrg_byid(caller->mm, destination);

  if (dst == NULL || dst->rg_start == dst->rg_end)
    return -1;

  for (i = 0; i < size; i++) {
    if (__read_kernel_mem(caller, -1, source, i, &data) != 0)
      return -1;
    if (__write_user_mem(caller, 0, destination, offset + i, data) != 0)
      return -1;
  }
  return 0;
}

int libkmem_cache_pool_create(struct pcb_t *caller, uint32_t size,
                              uint32_t align, uint32_t cache_pool_id)
{
  struct sc_regs regs = {0};

  regs.a1 = SYSMEM_CACHE_CREATE_OP;
  regs.a2 = size;
  regs.a3 = align;
  regs.a4 = cache_pool_id;
  return mem_syscall(caller, &regs);
}

int libkmem_copy_from_user(struct pcb_t *caller, uint32_t source,
                           uint32_t destination, uint32_t offset,
                           uint32_t size)
{
  struct sc_regs regs = {0};

  regs.a1 = SYSMEM_COPY_FROM_USER_OP;
  regs.a2 = source;
  regs.a3 = destination;
  regs.a4 = offset;
  regs.a5 = size;
  return mem_syscall(caller, &regs);
}

int libkmem_copy_to_user(struct pcb_t *caller, uint32_t source,
                         uint32_t destination, uint32_t offset,
                         uint32_t size)
{
  struct sc_regs regs = {0};

  regs.a1 = SYSMEM_COPY_TO_USER_OP;
  regs.a2 = source;
  regs.a3 = destination;
  regs.a4 = offset;
  regs.a5 = size;
  return mem_syscall(caller, &regs);
}


/*__read_kernel_mem - read value in kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  struct vm_rg_struct *krg;
  addr_t phyaddr;

  (void)vmaid;
  krg = get_ksymrg_byid(caller->mm, rgid);
  if (krg == NULL || krg->rg_start == krg->rg_end ||
      krg->rg_start + offset >= krg->rg_end)
    return -1;

  phyaddr = caller->mm->kphy_start[rgid] + offset;
  return MEMPHY_read(caller->krnl->mram, phyaddr, data);
}

/*__write_kernel_mem - write a kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  struct vm_rg_struct *krg;
  addr_t phyaddr;

  (void)vmaid;
  krg = get_ksymrg_byid(caller->mm, rgid);
  if (krg == NULL || krg->rg_start == krg->rg_end ||
      krg->rg_start + offset >= krg->rg_end)
    return -1;

  phyaddr = caller->mm->kphy_start[rgid] + offset;
  return MEMPHY_write(caller->krnl->mram, phyaddr, value);
}

/*__read_user_mem - read value in user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
   return __read(caller, vmaid, rgid, offset, data);
}


/*__write_user_mem - write a user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  return __write(caller, vmaid, rgid, offset, value);
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  pthread_mutex_lock(&mmvm_lock);
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING64_MAX_PGN; pagenum++)
  {
    pte = (uint32_t)caller->mm->pt[pagenum];

    if (pte & PAGING_PTE_SWAPPED_MASK)
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);
    }
    else if (PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
  }

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  if (!pg)
  {
    return -1;
  }
  struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  *retpgn = pg->pgn;
  if (prev == NULL)
    mm->fifo_pgn = NULL;
  else
    prev->pg_next = NULL;

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        {                                /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    }
    else
    {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif
