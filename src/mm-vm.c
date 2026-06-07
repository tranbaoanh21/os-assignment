/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* Provide a thin wrapper for vm_map_range that forwards to vm_map_ram
 * Some implementations define vm_map_ram; expose vm_map_range to match
 * the prototype in include/mm.h.
 */
int vm_map_range(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  extern int vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg);
  return vm_map_ram(caller, astart, aend, mapstart, incpgnum, ret_rg);
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL || pvma->vm_next == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

int __mm_swap_page_in(struct pcb_t *caller, addr_t swpfpn, addr_t ramfpn)
{
    __swap_cp_page(caller->krnl->active_mswp, swpfpn, caller->krnl->mram, ramfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->kernl->mm, vmaid);

  //newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + alignedsz;
  newrg->vmaid = vmaid;
  newrg->rg_next = NULL;
  /* END TODO */

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  struct vm_area_struct *vma = caller->mm->mmap;
  if (vma == NULL)
  {
    return -1;
  }

  /* TODO validate the planned memory area is not overlapped */

  while (vma != NULL)
  {
    if ((int)vma->vm_id != vmaid && OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  /* End TODO*/

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma;
  addr_t inc_amt;
  int incnumpage;
  addr_t old_end;
  addr_t old_sbrk;

  /* TOTO with new address scheme, the size need tobe aligned 
   *      the raw inc_sz maybe not fit pagesize
   */ 
  inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  incnumpage = (int)(inc_amt / PAGING_PAGESZ);

  cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
    return -1;

  old_end = cur_vma->vm_end;
  old_sbrk = cur_vma->sbrk;
  newrg = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  if (newrg == NULL)
    return -1;

  /* TODO Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, newrg->rg_start, newrg->rg_end) < 0) {
    free(newrg);
    return -1;
  }

  /* TODO: Obtain the new vm area based on vmaid */
  cur_vma->vm_end = old_end + inc_amt;
  cur_vma->sbrk = old_sbrk + inc_sz;

  /* The obtained vm area (only)
   * now will be alloc real ram region */

  if (vm_map_range(caller, newrg->rg_start, newrg->rg_end,
                   old_end, incnumpage, newrg) < 0) {
    cur_vma->vm_end = old_end;
    cur_vma->sbrk = old_sbrk;
    free(newrg);
    return -1;
  }

  if (newrg->rg_end > cur_vma->sbrk) {
    struct vm_rg_struct *left = init_vm_rg(cur_vma->sbrk, newrg->rg_end);
    left->vmaid = vmaid;
    enlist_vm_rg_node(&cur_vma->vm_freerg_list, left);
  }

  free(newrg);

  return 0;
}

// #endif
