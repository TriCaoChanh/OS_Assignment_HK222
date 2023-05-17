// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
             int pre,    // present
             int fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             int swpoff) // swap offset
{
  if (pre != 0)
  {
    if (swp == 0)
    { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(uint32_t *pte, int fpn)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

// /*
//  * vmap_page_range - map a range of page at aligned address
//  */
// int vmap_page_range(struct pcb_t *caller,           // process call
//                     int addr,                       // start address which is aligned to pagesz
//                     int pgnum,                      // num of mapping page
//                     struct framephy_struct *frames, // list of the mapped frames
//                     struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
// {                                                   // no guarantee all given pages are mapped
//   // uint32_t * pte = malloc(sizeof(uint32_t));
//   uint32_t *pte;
//   // struct framephy_struct *fpit = malloc(sizeof(struct framephy_struct));
//   struct framephy_struct *fpit;
//   // int  fpn;
//   int pgit = 0;
//   int pgn = PAGING_PGN(addr);

//   ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable

//   // fpit->fp_next = frames;

//   /* TODO map range of frame to address space
//    *      [addr to addr + pgnum*PAGING_PAGESZ
//    *      in page table caller->mm->pgd[]
//    */

//   fpit = frames;
//   for (pgit = 0; pgit < pgnum; pgit++)
//   {
//     pte = &caller->mm->pgd[pgn + pgit];
//     if(PAGING_PAGE_PRESENT(*pte)){
//       MEMPHY_put_freefp(caller->mram, fpit->fpn);
//       struct framephy_struct *un_used_fp = fpit;
//       fpit = fpit->fp_next;
//       free(un_used_fp);
//       continue;
//     }
//     // init_pte(pte, 1, fpit->fpn, 0, 0, 0, 0);
//     pte_set_fpn(pte, fpit->fpn);
//     fpit = fpit->fp_next;
//     // update region
//     // struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
//     // newrg->rg_start = addr;
//     // newrg->rg_end = addr + pgit * PAGING_PAGESZ;
//     // newrg->rg_next = ret_rg;
//     // ret_rg = newrg;
//     /* Tracking for later page replacement activities (if needed)
//      * Enqueue new usage page */
//     enlist_pgn_node(&caller->mm->fifo_pgn, pgn + pgit); // put in the loop to track all the pages
//   }

//   return 0;
// }

int vmap_page_range2(struct pcb_t *caller,           // process call
                     int addr,                       // start address which is aligned to pagesz
                     int pgnum,                      // num of mapping page
                     struct framephy_struct *frames, // list of the mapped frames
                     struct vm_rg_struct *ret_rg,    // return mapped region, the real mapped fp
                     int *frm_loc                    // array of frame location, 0 for RAM, 1 for SWAP
)
// no guarantee all given pages are mapped
{
  uint32_t *pte;
  struct framephy_struct *fpit, *temp;

  int pgit = 0;
  int pgn = PAGING_PGN(addr);

  ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable

  fpit = frames;
  for (pgit = 0; pgit < pgnum; pgit++)
  {
    temp = fpit;
    pte = malloc(sizeof(uint32_t));
    *pte = 0;

    if (frm_loc[pgit] == 0)
    {
      pte_set_fpn(pte, fpit->fpn);
      //printf("##### The PID: %d \n",caller->pid);
      //printf("##### tenmp fpn: %d \n",temp->fpn);
      MEMPHY_put_usefp(caller->mram, temp->fpn, caller->pid);
    }
    else
    {
      pte_set_swap(pte, 0, fpit->fpn);
      MEMPHY_put_usefp(caller->active_mswp, temp->fpn, caller->pid);
    }

    caller->mm->pgd[pgn + pgit] = *pte;
    //printf("##### The update frame number is: %d \n", GETVAL(caller->mm->pgd[pgn + pgit], PAGING_PTE_FPN_MASK, 0));
    fpit = fpit->fp_next;

    enlist_pgn_node(&caller->mm->fifo_pgn, pgn + pgit); // put in the loop to track all the pages

    free(temp);
    free(pte);
  }
  free(frm_loc);
  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */
// int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
// {
//   int pgit, fpn;
//   struct framephy_struct *newfp_str;

//   for (pgit = 0; pgit < req_pgnum; pgit++)
//   {
//     if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
//     {
//       newfp_str = malloc(sizeof(struct framephy_struct));
//       newfp_str->fp_next = *frm_lst;
//       newfp_str->fpn = fpn;
//       newfp_str->owner = caller->mm;
//       *frm_lst = newfp_str;
//     }
//     else
//     { // ERROR CODE of obtaining somes but not enough frames
//       printf("SWAPPINGGGG FOR NEW ALLOC\n");
//       int vicpgn, swpfpn;

//       find_victim_page(caller->mm, &vicpgn);

//       /* Get free frame in MEMSWP */
//       MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
//       __swap_cp_page(caller->mram, vicpgn, caller->active_mswp, swpfpn);
//       MEMPHY_put_freefp(caller->mram, vicpgn);
//       pte_set_swap(&(caller->mm->pgd[vicpgn]), 0, swpfpn);

//       pgit--;
//       // printf("ERROR CODE of obtaining somes but not enough frames\n");
//       // return -1;
//     }
//   }
//   return 0;
// }

void enlist_frm_lst(struct pcb_t *caller, struct framephy_struct **frm_lst, struct framephy_struct *newfp, int fpn)
{
  newfp->fpn = fpn;
  newfp->owner = caller->mm;
  newfp->pid_owner = caller->pid;
  //printf("####### %d\n",newfp->status);
  newfp->fp_next = *frm_lst;
  *frm_lst = newfp;
}

/**
 * @brief Request MEMPHY for some frames
 *
 * @param caller pcb_t
 * @param req_pgnum number of requested pages
 * @param frm_lst the return frame list
 * @param frm_loc frame location, 0 for RAM, 1 for SWAP
 * @return int
 */
int alloc_pages_range2(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst, int *frm_loc)
{
  int pgit, fpn, swpfpn;
  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    struct framephy_struct *newfp = malloc(sizeof(struct framephy_struct));
    newfp->status = 1; // 1 is alloc
    if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
      // Successfully alloc some frame in RAM
      enlist_frm_lst(caller, frm_lst, newfp, fpn);
      frm_loc[req_pgnum - pgit - 1] = 0;
    }
    else
    {
      // There is no more free frame in RAM
      // Lazy Swapper: alloc in SWAP, never swaps a page into memory unless page will be needed
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == 0)
      {
        printf("#### Alloc in swap \n");
        enlist_frm_lst(caller, frm_lst, newfp, swpfpn);
        frm_loc[req_pgnum - pgit - 1] = 1;
      }
      else
        return -3000; // No free frame in SWAP
    }
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
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc;
  int *frm_loc = malloc(sizeof(int) * incpgnum);

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  // ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);
  ret_alloc = alloc_pages_range2(caller, incpgnum, &frm_lst, frm_loc);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
#ifdef MMDBG
    printf("OOM: vm_map_ram out of memory \n");
#endif
    return -1;
  }
  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  // vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  vmap_page_range2(caller, mapstart, incpgnum, frm_lst, ret_rg, frm_loc);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                   struct memphy_struct *mpdst, int dstfpn)
{
  int cellidx;
  int addrsrc, addrdst;
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
  struct vm_area_struct *vma = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(uint32_t));

  /* By default the owner comes with at least one vma */
  vma->vm_id = 1;
  vma->vm_start = 0;
  vma->vm_end = vma->vm_start;
  vma->sbrk = vma->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
  enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);

  vma->vm_next = NULL;
  vma->vm_mm = mm; /*point back to vma owner */

  mm->mmap = vma;

  return 0;
}

struct vm_rg_struct *init_vm_rg(int rg_start, int rg_end)
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

int enlist_pgn_node(struct pgn_t **plist, int pgn)
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

  printf("Print frames list in RAM: ");
  if (fp == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (fp != NULL)
  {
    printf("Frame num[%d]  PID:%d  Status:%d\n", fp->fpn, fp->pid_owner, fp->status);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[%ld->%ld]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[%ld->%ld]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[%d]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
  int pgn_start, pgn_end;
  int pgit;

  if (end == -1)
  {
    pgn_start = 0;
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    end = cur_vma->vm_end;
  }
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);

  printf("print_pgtbl: %d - %d", start, end);
  if (caller == NULL)
  {
    printf("NULL caller\n");
    return -1;
  }
  printf("\n");

  for (pgit = pgn_start; pgit < pgn_end; pgit++)
  {
    printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
  }

  return 0;
}

// #endif
