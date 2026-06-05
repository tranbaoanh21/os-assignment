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
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#ifdef MM64
#include "mm64.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MEMPHY_MAX_EAGER_FRAMES 4096
static pthread_mutex_t memphy_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, addr_t offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      /* Traverse sequentially */
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;
   if (addr >= (addr_t)mp->maxsz)
      return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, addr_t addr, BYTE value)
{

   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data)
{
   if (mp == NULL)
      return -1;
   if (addr >= (addr_t)mp->maxsz)
      return -1;

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   /* This setting come with fixed constant PAGESZ */
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;
   if (numfp > MEMPHY_MAX_EAGER_FRAMES)
      numfp = MEMPHY_MAX_EAGER_FRAMES;

   /* Init head of free framephy list */
   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;
   mp->free_fp_list = fst;

   /* We have list with first element, fill in the rest num-1 element member*/
   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }

   return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)
{
   struct framephy_struct *fp;

   if (mp == NULL || retfpn == NULL)
      return -1;
   pthread_mutex_lock(&memphy_lock);
   fp = mp->free_fp_list;
   if (fp == NULL) {
      pthread_mutex_unlock(&memphy_lock);
      return -1;
   }

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp);
   pthread_mutex_unlock(&memphy_lock);

   return 0;
}

int MEMPHY_get_contiguous_fps(struct memphy_struct *mp, int count,
                              addr_t *start_fpn)
{
   struct framephy_struct *candidate;
   struct framephy_struct *node;
   int offset;

   if (mp == NULL || count <= 0 || start_fpn == NULL)
      return -1;

   pthread_mutex_lock(&memphy_lock);
   for (candidate = mp->free_fp_list; candidate != NULL;
        candidate = candidate->fp_next) {
      for (offset = 0; offset < count; offset++) {
         for (node = mp->free_fp_list; node != NULL; node = node->fp_next)
            if (node->fpn == candidate->fpn + (addr_t)offset)
               break;
         if (node == NULL)
            break;
      }
      if (offset == count) {
         addr_t first = candidate->fpn;
         for (offset = 0; offset < count; offset++) {
            struct framephy_struct **link = &mp->free_fp_list;
            while ((*link)->fpn != first + (addr_t)offset)
               link = &(*link)->fp_next;
            node = *link;
            *link = node->fp_next;
            free(node);
         }
         *start_fpn = first;
         pthread_mutex_unlock(&memphy_lock);
         return 0;
      }
   }
   pthread_mutex_unlock(&memphy_lock);
   return -1;
}

int MEMPHY_dump(struct memphy_struct *mp)
{
   int index;
   int nonzero = 0;

   if (mp == NULL)
      return -1;
   for (index = 0; index < mp->maxsz; index++)
      if (mp->storage[index] != 0)
         nonzero++;
   printf("MEMPHY size=%d nonzero_bytes=%d\n", mp->maxsz, nonzero);
   return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn)
{
   struct framephy_struct *fp;
   struct framephy_struct *newnode;

   if (mp == NULL || mp->maxsz == 0)
      return -1;
   newnode = malloc(sizeof(struct framephy_struct));
   if (newnode == NULL)
      return -1;
   pthread_mutex_lock(&memphy_lock);
   fp = mp->free_fp_list;

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;
   pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, addr_t max_size, int randomflg)
{
   addr_t alloc_size = max_size;
   mp->storage = NULL;
   mp->free_fp_list = NULL;
   mp->used_fp_list = NULL;
   mp->cursor = 0;
   mp->rdmflg = (randomflg != 0) ? 1 : 0;
   if (max_size == 0) {
      mp->maxsz = 0;
      return 0;
   }
   if (alloc_size > (addr_t)MEMPHY_MAX_EAGER_FRAMES * PAGING_PAGESZ)
      alloc_size = (addr_t)MEMPHY_MAX_EAGER_FRAMES * PAGING_PAGESZ;

   mp->storage = (BYTE *)malloc(alloc_size * sizeof(BYTE));
   mp->maxsz = (int)alloc_size;
   memset(mp->storage, 0, alloc_size * sizeof(BYTE));

   MEMPHY_format(mp, PAGING_PAGESZ);

   if (!mp->rdmflg) /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

// #endif
