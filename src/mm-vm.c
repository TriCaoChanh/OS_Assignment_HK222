// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include "workingset.h"

#include <stdlib.h>
#include <stdio.h>

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt.rg_start >= rg_elmt.rg_end)
    return -1;

  /* Enlist the new region */
  struct vm_rg_struct *deep_copy_rg_elmt = malloc(sizeof(struct vm_rg_struct));
  deep_copy_rg_elmt->rg_start = rg_elmt.rg_start;
  deep_copy_rg_elmt->rg_end = rg_elmt.rg_end;
  deep_copy_rg_elmt->rg_next = rg_node;
  mm->mmap->vm_freerg_list = deep_copy_rg_elmt;

  return 0;
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

  int vmait = 0;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;
    vmait = pvma->vm_id; // update
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &(mm->symrgtbl[rgid]);
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;
  int flag;
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode, &flag) == 0)
  {
    //printf("FOUND FREE RG\n");
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;

    int pgn = PAGING_PGN(*alloc_addr);
    int inc_amt = PAGING_PAGE_ALIGNSZ(rgnode.rg_end - rgnode.rg_start);
    int incnumpage = inc_amt / PAGING_PAGESZ;
    int pgit = 0;
    uint32_t pte;
    // uint32_t pte = caller->mm->pgd[pgit];

    for (pgit = 0; pgit < incnumpage; pgit++)
    {
      pte = caller->mm->pgd[pgit];
      int temp = GETVAL(pte, PAGING_PTE_FPN_MASK, 0);
      struct framephy_struct* head = caller->mram->used_fp_list;
      while(head != NULL)
      {
        if(head->fpn == temp)
        {
          head->status = 1;
          break;
        }
        head = head->fp_next;
      }
    }

    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  // int inc_limit_ret
  int old_sbrk;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  if (inc_vma_limit(caller, vmaid, size) < 0)
    return -1;

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;

  // TRI THEM 1 DONG
  cur_vma->sbrk = old_sbrk + inc_sz;
  // cur_vma->sbrk = old_sbrk + size;
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
  struct vm_rg_struct *rgnode = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  // /* TODO: Manage the collect freed region to freerg_list */
  rgnode->rg_start = caller->mm->symrgtbl[rgid].rg_start;
  rgnode->rg_end = caller->mm->symrgtbl[rgid].rg_end;

  // caller->mm->symrgtbl[rgid].rg_start = -99;
  // caller->mm->symrgtbl[rgid].rg_end = -99;

  int addr = rgnode->rg_start;
  int pgn = PAGING_PGN(addr);
  int inc_amt = PAGING_PAGE_ALIGNSZ(rgnode->rg_end - rgnode->rg_start);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  int pgit = 0;

  //MEMPHY_dump(caller->mram);
  for (pgit = 0; pgit < incnumpage; pgit++)
  {
    //printf("##### The delete frame number is: %d \n", GETVAL(caller->mm->pgd[pgn + pgit], PAGING_PTE_FPN_MASK, 0));
    //printf("%d \n", caller->mram);
    //printf("##### The mm: %d \n",caller->mm);
    //MEMPHY_delete_usefp(&(caller->mram), GETVAL(caller->mm->pgd[pgn + pgit], PAGING_PTE_FPN_MASK, 0));
    MEMPHY_setfp_status(&(caller->mram),GETVAL(caller->mm->pgd[pgn + pgit], PAGING_PTE_FPN_MASK, 0), 0);

  }

  /*enlist the obsoleted memory region */
  enlist_vm_rg_node(&caller->mm->mmap->vm_freerg_list, rgnode);

  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
// int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
// {
//   uint32_t pte = mm->pgd[pgn];

//   if (!PAGING_PAGE_PRESENT(pte))
//   { /* Page is not online, make it actively living */
//     printf("SWAPPINGGGG\n");
//     int vicpgn, swpfpn;
//     // int vicfpn;
//     // uint32_t vicpte;

//     int tgtfpn = PAGING_SWP(pte); // the target frame storing our variable

//     /* TODO: Play with your paging theory here */
//     /* Find victim page */
//     find_victim_page(caller->mm, &vicpgn);

//     /* Get free frame in MEMSWP */
//     MEMPHY_get_freefp(caller->active_mswp, &swpfpn);

//     /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
//     /* Copy victim frame to swap */
//     // __swap_cp_page();
//     __swap_cp_page(caller->mram, vicpgn, caller->active_mswp, swpfpn);
//     /* Copy target frame from swap to mem */
//     // __swap_cp_page();
//     __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, pgn);

//     /* Update page table */
//     // pte_set_swap() &mm->pgd;
//     pte_set_swap(&(mm->pgd[vicpgn]), swpfpn, 0);

//     /* Update its online status of the target page */
//     // pte_set_fpn() & mm->pgd[pgn];
//     pte_set_fpn(&pte, tgtfpn);

//     enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
//   }

//   *fpn = PAGING_FPN(pte);

//   return 0;
// }

int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte))
  {
    printf("ACCESS INVALID MEMORY REGION!\n");
    return -1;
  }
  else if (PAGING_PAGE_IN_SWAP(pte))
  { /* Page is not online, make it actively living */
    int tgtfpn = PAGING_SWP(pte);
    int tmpfpn;
    if (MEMPHY_get_freefp(caller->mram, &tmpfpn) == 0)
    {
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, tmpfpn);
      MEMPHY_put_usefp(caller->mram, tmpfpn, caller->pid);

      // enlist_framephy_node(&caller->active_mswp->free_fp_list, tgtfpn);
      MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
      pte_set_fpn(&caller->mm->pgd[pgn], tmpfpn);
    }
    else
    {
      int vicpgn, vicfpn;
      int swpfpn;
      // the target frame storing our variable
      /* TODO: Play with your paging theory here */
      /* Find victim page */
      if (find_victim_page(caller->mm, &vicpgn) == -1)
        printf("Process %d: No victim page found\n", caller->pid);

      vicfpn = GETVAL(caller->mm->pgd[vicpgn], PAGING_PTE_FPN_MASK, 0);
      /* Get free frame in MEMSWP */

      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1)
        printf("Process %d: No free frame in MEMSWP\n", caller->pid);

      /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
      /* Copy victim frame to swap */
      __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
      MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
      MEMPHY_update_pid(&(caller->mram),vicfpn, caller->pid);

      /* Update page table */
      // pte_set_swap() &mm->pgd;
      pte_set_swap(&caller->mm->pgd[vicpgn], 0, swpfpn); /* Update its online status of the target page */ // pte_set_fpn() & mm->pgd[pgn];
      pte_set_fpn(&caller->mm->pgd[pgn], vicfpn);
    }
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = GETVAL(mm->pgd[pgn], PAGING_PTE_FPN_MASK, 0);

  put_workingset(caller->working_set, pgn);
  return 0;
}
/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram, phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram, phyaddr, value);

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
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL || currg->rg_start == currg->rg_end) /* Invalid memory identify */
  {
    return -1;
  }
  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    uint32_t offset,    // Source address = [source] + [offset]
    uint32_t destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  destination = (uint32_t)data;
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#ifdef RAM_DUMP
  MEMPHY_dump(proc->mram);
#endif
#endif
  if(val == -1)  printf("Read in invalid region !\n");
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
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL || currg->rg_start == currg->rg_end) /* Invalid memory identify */
  {
    printf("Write in invalid region !\n");
    return -1;
  }
  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#ifdef RAM_DUMP
  MEMPHY_dump(proc->mram);
#endif
#endif

  return __write(proc, 0, destination, offset, data);
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_IN_SWAP(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
      MEMPHY_delete_usefp(&(caller->mram), fpn);
    }
    else
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

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
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = cur_vma->vm_end;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  /* TODO validate the planned memory area is not overlapped */

  return 1;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  cur_vma->sbrk = cur_vma->vm_end + inc_sz;
  cur_vma->vm_end += inc_amt;

  area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  enlist_vm_rg_node(&caller->mm->mmap->vm_freerg_list, area);

  /*Validate overlap of obtained region */
  // if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  //   return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  // cur_vma->vm_end += inc_sz;

  // printf("INCREASE NUM PAGE: %d\n", incnumpage);
  // if (vm_map_ram(caller, area->rg_start, area->rg_end,
  //                old_end, incnumpage, newrg) < 0)
  if (vm_map_ram(caller, 0, 0, old_end, incnumpage, newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;
  if (pg == NULL)
  {
    printf("CANNOT FIND ANY PAGE\n");
    return -1;
  }

  // There is only 1 page online
  if (pg->pg_next == NULL)
  {
    *retpgn = pg->pgn;
    mm->fifo_pgn = NULL;
    free(pg);
    return 0;
  }

  // Traverse the list
  while (pg->pg_next && pg->pg_next->pg_next)
    pg = pg->pg_next;

  *retpgn = pg->pg_next->pgn;
  free(pg->pg_next);
  pg->pg_next = NULL;

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg, int* flag)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  *flag = 0;
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
        *flag = 1;
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
      break; // Found a fit region
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
