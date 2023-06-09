#ifndef OSCFG_H
#define OSCFG_H

#define MLQ_SCHED 1
#define MAX_PRIO 140

/**
 * For small sized RAM with many processes run concurrently,
 * use Working_set to avoid thrashing
 */
//#define WORKING_SET 1

//#define MM_PAGING 1
// #define MM_FIXED_MEMSZ
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
#define RAM_DUMP 1

#endif
