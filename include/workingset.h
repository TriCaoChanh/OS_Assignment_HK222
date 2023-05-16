#ifndef WORKING_SET_H
#define WORKING_SET_H

#include "common.h"
#include "mm.h"

// struct WorkingSet {
//     int* working_set_arr;

//     int max_size;
//     int cursor;
// };

void init_workingset(struct pcb_t *proc);
void increment_cursor(struct WorkingSet *working_set);
void put_workingset(struct WorkingSet *working_set, int pgn);
int is_in_working_set(struct WorkingSet *working_set, int pgn);
void remove_from_pgn_node_list(struct pgn_t **plist, int pgn);
void put_sleep(struct pcb_t *proc);
void print_working_set(struct WorkingSet *working_set);

#endif