#ifndef EATS_H
#define EATS_H

#include <linux/types.h>

/* Constants for our "Big.LITTLE" logic */
#define EATS_SHORT_THRESH_NS   25000000ULL  /* 25ms threshold */

/* The WPBA History Card for a task */
struct eats_task_info {
    u64 last_burst;        /* Last measured CPU burst */
    u64 phase_avg;         /* Average of recent bursts (Phase detection) */
    u64 io_ratio;          /* I/O-to-CPU ratio signal */
    
    /* Dynamic Weights (stored as parts of 100 for integer math) */
    int w1;                /* Weight for last burst */
    int w2;                /* Weight for phase average */
    int w3;                /* Weight for IO ratio */
    
    u64 last_prediction;   /* The most recent WPBA output */
    u64 predicted_ns;      /* Alias for compatibility with previous logic */
    int error_count;       /* Tracking mispredictions for weight adjustment */
};

/* The wrapper entry for the hashtable */
struct eats_entry {
    pid_t pid;
    struct eats_task_info info;
    struct hlist_node node;
};

#endif
