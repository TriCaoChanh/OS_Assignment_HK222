#include "workingset.h"
#include <stdio.h>
#include <stdlib.h>

void init_workingset(struct pcb_t *proc)
{
    proc->working_set = malloc(sizeof(struct WorkingSet));
    proc->working_set->max_size = proc->mram->maxsz / (PAGING_PAGESZ * 2);
    proc->working_set->working_set_arr = malloc(sizeof(int) * proc->working_set->max_size);

    for (int i = 0; i < proc->working_set->max_size; i++)
        proc->working_set->working_set_arr[i] = -1;

    proc->working_set->cursor = 0;
}

void increment_cursor(struct WorkingSet *working_set)
{
    working_set->cursor++;
    if (working_set->cursor == working_set->max_size)
        working_set->cursor = 0;
}

void put_workingset(struct WorkingSet *working_set, int pgn)
{
    working_set->working_set_arr[working_set->cursor] = pgn;
    increment_cursor(working_set);
}

/**
 * @brief
 *  Return 0: Working Set is not full.
 *  Return 1: In Working set.
 *  Return -1: Not in Working set.
 */
int is_in_working_set(struct WorkingSet *working_set, int pgn)
{
    // printf("WORKING SET: \n");
    for (int i = 0; i < working_set->max_size; i++)
    {
        // printf("%d, ", working_set->working_set_arr[i]);
        if (working_set->working_set_arr[i] == -1)
            return 0;
        if (pgn == working_set->working_set_arr[i])
            return 1;
    }
    // printf("\n");

    return -1;
}

void put_sleep(struct pcb_t *proc)
{
#ifndef WORKING_SET
    return;
#endif
    /* Working Set is not full - Return */
    if (proc->working_set->working_set_arr[0] != -1)
        print_working_set(proc->working_set);

    if (proc->working_set->working_set_arr[proc->working_set->cursor] == -1)
        return;

    int pgn_start, pgn_end;
    int pgit;

    struct vm_area_struct *cur_vma = get_vma_by_num(proc->mm, 0);
    int end = cur_vma->vm_end;

    pgn_start = PAGING_PGN(0);
    pgn_end = PAGING_PGN(end);

    // printf("PID: %d\n", proc->pid);
    // printf("SIZE OF WINDOW: %d", proc->working_set->max_size);

    for (pgit = pgn_start; pgit < pgn_end; pgit++)
    {
        uint32_t pte = proc->mm->pgd[pgit];
        if (PAGING_PAGE_PRESENT(pte) && !PAGING_PAGE_IN_SWAP(pte) && is_in_working_set(proc->working_set, pgit) == -1)
        {
            printf("\t\tPut pgn %d to swap\n", pgit);

            int swpfpn;
            int ramfpn;

            ramfpn = GETVAL(pte, PAGING_PTE_FPN_MASK, 0);

            remove_from_pgn_node_list(&proc->mm->fifo_pgn, pgit);

            MEMPHY_get_freefp(proc->active_mswp, &swpfpn);

            __swap_cp_page(proc->mram, ramfpn, proc->active_mswp, swpfpn);
            pte_set_swap(&proc->mm->pgd[pgit], 0, swpfpn);

            MEMPHY_put_freefp(proc->mram, ramfpn);
            MEMPHY_delete_usefp(&(proc->mram), ramfpn);
        }
    }
}

void remove_from_pgn_node_list(struct pgn_t **plist, int pgn)
{
    struct pgn_t *pg = *plist;
    if (pg->pgn == pgn)
    {
        *plist = pg->pg_next;
        free(pg);
        return;
    }

    while (pg != NULL && pg->pg_next != NULL)
    {
        if (pg->pg_next->pgn == pgn)
            break;
        pg = pg->pg_next;
    }

    struct pgn_t *target = pg->pg_next;
    if (target == NULL)
        return;

    pg->pg_next = target->pg_next;

    free(target);
}

void print_working_set(struct WorkingSet *working_set)
{
    printf("\t\tWorking set: [ ");
    for (int i = 0; i < working_set->max_size; i++)
    {
        printf("%d ", working_set->working_set_arr[i]);
    }
    printf("]\n");
}