/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if defined(MM64)

int paging64_is_canonical(addr_t addr)
{
  addr_t upper = addr >> 57;
  addr_t sign = (addr >> 56) & 1ULL;

  return sign ? upper == 0x7fULL : upper == 0;
}

static addr_t *alloc_table(struct mm_struct *mm)
{
  addr_t *table = calloc(PAGING64_ENTRIES, sizeof(addr_t));

  if (table != NULL)
    mm->pgtable_bytes += PAGING64_ENTRIES * sizeof(addr_t);
  return table;
}

static int walk_pte(struct pcb_t *caller, addr_t pgn, addr_t **ret_pte,
                    int create)
{
  struct mm_struct *mm;
  addr_t addr;
  addr_t pgd, p4d, pud, pmd, pt;
  addr_t *p4d_tbl;
  addr_t *pud_tbl;
  addr_t *pmd_tbl;
  addr_t *pt_tbl;

  if (caller == NULL || caller->mm == NULL || ret_pte == NULL)
    return -1;

  mm = caller->mm;
  addr = pgn << PAGING64_ADDR_PT_SHIFT;
  if (!paging64_is_canonical(addr))
    return -1;
  get_pd_from_address(addr, &pgd, &p4d, &pud, &pmd, &pt);

  mm->pgtable_walks++;
  mm->pgtable_accesses++;
  p4d_tbl = (addr_t *)mm->pgd[pgd];
  if (p4d_tbl == NULL && create) {
    p4d_tbl = alloc_table(mm);
    mm->pgd[pgd] = (addr_t)p4d_tbl;
  }
  if (p4d_tbl == NULL) return -1;
  mm->pgtable_accesses++;
  pud_tbl = (addr_t *)p4d_tbl[p4d];
  if (pud_tbl == NULL && create) {
    pud_tbl = alloc_table(mm);
    p4d_tbl[p4d] = (addr_t)pud_tbl;
  }
  if (pud_tbl == NULL) return -1;
  mm->pgtable_accesses++;
  pmd_tbl = (addr_t *)pud_tbl[pud];
  if (pmd_tbl == NULL && create) {
    pmd_tbl = alloc_table(mm);
    pud_tbl[pud] = (addr_t)pmd_tbl;
  }
  if (pmd_tbl == NULL) return -1;
  mm->pgtable_accesses++;
  pt_tbl = (addr_t *)pmd_tbl[pmd];
  if (pt_tbl == NULL && create) {
    pt_tbl = alloc_table(mm);
    pmd_tbl[pmd] = (addr_t)pt_tbl;
  }
  if (pt_tbl == NULL) return -1;
  if (pgd == 0 && p4d == 0 && pud == 0 && pmd == 0) {
    mm->p4d = p4d_tbl;
    mm->pud = pud_tbl;
    mm->pmd = pmd_tbl;
    mm->pt = pt_tbl;
  }
  mm->pgtable_accesses++;
  *ret_pte = &pt_tbl[pt];
  return 0;
}

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  if (!paging64_is_canonical(addr))
    return -1;

	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

  (void)pgd;
  (void)p4d;
  (void)pud;
  (void)pmd;
  (void)pt;

	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  addr_t *pte;
	
  pte = NULL;
  if (walk_pte(caller, pgn, &pte, 1) != 0)
    return -1;
	
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  addr_t *pte;
	
  pte = NULL;
  if (walk_pte(caller, pgn, &pte, 1) != 0)
    return -1;

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  addr_t *pte;

  if (walk_pte(caller, pgn, &pte, 0) != 0)
    return 0;
  return (uint32_t)*pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	addr_t *pte;

	if (walk_pte(caller, pgn, &pte, 1) != 0)
		return -1;
	*pte = pte_val;
	
	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  int pgit = 0;
  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;

  if (!paging64_is_canonical(addr)) {
    printf("MM64 rejected non-canonical address 0x%llx\n",
           (unsigned long long)addr);
    return -1;
  }
  if ((addr & (PAGING64_PAGESZ - 1)) != 0 || pgnum < 0 ||
      (pgnum > 0 &&
       (addr > ~(addr_t)0 - (addr_t)(pgnum - 1) * PAGING64_PAGESZ ||
        !paging64_is_canonical(addr + (addr_t)(pgnum - 1) * PAGING64_PAGESZ))))
    return -1;

  for (pgit = 0; pgit < pgnum; pgit++)
    if (pte_set_entry(caller, pgn + pgit, 0) != 0)
      return -1;

  printf("MM64 mapped %d dummy page(s) at 0x%llx\n", pgnum,
         (unsigned long long)addr);
  print_pgtbl(caller, addr, addr + (addr_t)pgnum * PAGING64_PAGESZ);
  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
int vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
struct framephy_struct *fpit;
int pgit = 0;
addr_t pgn;

  if (ret_rg == NULL)
    return -1;

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + (addr_t)pgnum * PAGING_PAGESZ;

  fpit = frames;
  pgn = PAGING_PGN(addr);
  while (pgit < pgnum && fpit != NULL)
  {
    pte_set_fpn(caller, pgn + pgit, fpit->fpn);
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn + pgit);
    fpit = fpit->fp_next;
    pgit++;
  }

  return (pgit == pgnum) ? 0 : -1;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str = NULL;
  struct framephy_struct *tail = NULL;

  *frm_lst = NULL;
  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) != 0)
    {
      struct framephy_struct *it = *frm_lst;
      while (it != NULL)
      {
        MEMPHY_put_freefp(caller->krnl->mram, it->fpn);
        struct framephy_struct *tmp = it;
        it = it->fp_next;
        free(tmp);
      }
      *frm_lst = NULL;
      return -3000;
    }

    newfp_str = malloc(sizeof(struct framephy_struct));
    newfp_str->fpn = fpn;
    newfp_str->fp_next = NULL;
    newfp_str->owner = caller->mm;

    if (*frm_lst == NULL)
      *frm_lst = newfp_str;
    else
      tail->fp_next = newfp_str;

    tail = newfp_str;
  }

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
int vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc = 0;
//int pgnum = incpgnum;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  (void)caller;

  mm->pgd = calloc(PAGING64_ENTRIES, sizeof(addr_t));
  mm->p4d = mm->pud = mm->pmd = mm->pt = NULL;
  if (vma0 == NULL || mm->pgd == NULL)
    return -1;
  mm->pgtable_accesses = 0;
  mm->pgtable_walks = 0;
  mm->pgtable_bytes = PAGING64_ENTRIES * sizeof(addr_t);


  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  vma0->vm_freerg_list = NULL;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
  vma0->vm_next = NULL;

  /* Point vma owner backward */
  vma0->vm_mm = mm;

  /* TODO: update mmap */
  mm->mmap = vma0;
  mm->fifo_pgn = NULL;
  mm->kcpooltbl = calloc(PAGING_MAX_SYMTBL_SZ, sizeof(struct kcache_pool_struct));
  mm->kmem_sbrk = PAGING_KMEM_BASE;

  int i;
  for (i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
    mm->symrgtbl[i].vmaid = 0;
    mm->symrgtbl[i].rg_start = 0;
    mm->symrgtbl[i].rg_end = 0;
    mm->symrgtbl[i].rg_next = NULL;
    mm->ksymrgtbl[i].vmaid = -1;
    mm->ksymrgtbl[i].rg_start = 0;
    mm->ksymrgtbl[i].rg_end = 0;
    mm->ksymrgtbl[i].rg_next = NULL;
    mm->kphy_start[i] = 0;
    mm->kphy_npages[i] = 0;
  }

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

  if (caller == NULL || caller->mm == NULL ||
      get_pd_from_address(start, &pgd, &p4d, &pud, &pmd, &pt) != 0)
    return -1;
  (void)end;

  printf("MM64 pid=%u va=0x%llx indexes=[%llu,%llu,%llu,%llu,%llu] "
         "walks=%llu accesses=%llu bytes=%llu\n",
         caller->pid, (unsigned long long)start,
         (unsigned long long)pgd, (unsigned long long)p4d,
         (unsigned long long)pud, (unsigned long long)pmd,
         (unsigned long long)pt,
         (unsigned long long)caller->mm->pgtable_walks,
         (unsigned long long)caller->mm->pgtable_accesses,
         (unsigned long long)caller->mm->pgtable_bytes);

  return 0;
}

static int table_owns_fpn(addr_t *table, int depth, addr_t fpn)
{
  int i;

  if (table == NULL)
    return 0;
  if (depth == 4) {
    for (i = 0; i < PAGING64_ENTRIES; i++) {
      uint32_t pte = (uint32_t)table[i];
      if (PAGING_PAGE_PRESENT(pte) &&
          !(pte & PAGING_PTE_SWAPPED_MASK) && PAGING_FPN(pte) == fpn)
        return 1;
    }
    return 0;
  }
  for (i = 0; i < PAGING64_ENTRIES; i++)
    if (table[i] != 0 && table_owns_fpn((addr_t *)table[i], depth + 1, fpn))
      return 1;
  return 0;
}

int paging64_owns_fpn(struct mm_struct *mm, addr_t fpn)
{
  return mm != NULL && table_owns_fpn(mm->pgd, 0, fpn);
}

static void release_table_frames(addr_t *table, int depth,
                                 struct memphy_struct *mram,
                                 struct memphy_struct *active_mswp)
{
  int i;

  if (table == NULL)
    return;
  if (depth == 4) {
    for (i = 0; i < PAGING64_ENTRIES; i++) {
      uint32_t pte = (uint32_t)table[i];
      if ((pte & PAGING_PTE_SWAPPED_MASK) != 0)
        MEMPHY_put_freefp(active_mswp, PAGING_SWP(pte));
      else if (PAGING_PAGE_PRESENT(pte))
        MEMPHY_put_freefp(mram, PAGING_FPN(pte));
      table[i] = 0;
    }
    return;
  }
  for (i = 0; i < PAGING64_ENTRIES; i++)
    if (table[i] != 0)
      release_table_frames((addr_t *)table[i], depth + 1, mram, active_mswp);
}

int paging64_release_user_frames(struct mm_struct *mm,
                                 struct memphy_struct *mram,
                                 struct memphy_struct *active_mswp)
{
  if (mm == NULL)
    return -1;
  release_table_frames(mm->pgd, 0, mram, active_mswp);
  return 0;
}

static void free_page_table_tree(addr_t *table, int depth)
{
  int i;

  if (table == NULL)
    return;
  if (depth < 4)
    for (i = 0; i < PAGING64_ENTRIES; i++)
      if (table[i] != 0)
        free_page_table_tree((addr_t *)table[i], depth + 1);
  free(table);
}

int destroy_mm(struct mm_struct *mm, struct memphy_struct *mram,
               struct memphy_struct *active_mswp)
{
  addr_t pgn;
  struct vm_area_struct *vma;
  struct vm_rg_struct *rg;
  struct pgn_t *pgn_node;

  if (mm == NULL)
    return 0;

  paging64_release_user_frames(mm, mram, active_mswp);
  for (pgn = 0; pgn < PAGING_MAX_SYMTBL_SZ; pgn++) {
    int page;
    for (page = 0; page < mm->kphy_npages[pgn]; page++)
      MEMPHY_put_freefp(mram, mm->kphy_start[pgn] / PAGING_PAGESZ + page);
  }

  while ((pgn_node = mm->fifo_pgn) != NULL) {
    mm->fifo_pgn = pgn_node->pg_next;
    free(pgn_node);
  }
  while ((vma = mm->mmap) != NULL) {
    mm->mmap = vma->vm_next;
    while ((rg = vma->vm_freerg_list) != NULL) {
      vma->vm_freerg_list = rg->rg_next;
      free(rg);
    }
    free(vma);
  }
  free(mm->kcpooltbl);
  free_page_table_tree(mm->pgd, 0);
  free(mm);
  return 0;
}

#endif  //def MM64
