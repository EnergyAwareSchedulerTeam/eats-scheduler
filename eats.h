#ifndef EATS_H
#define EATS_H

#include <linux/types.h>

/* ── Thresholds ────────────────────────────────────────────── */
#define EATS_LITTLE_THRESH_NS  20000000ULL   /* 20ms */
#define EATS_BIG_THRESH_NS     50000000ULL   /* 50ms */

/* ── WPBA weights ──────────────────────────────────────────── */
#define WPBA_W1_INIT  40
#define WPBA_W2_INIT  35
#define WPBA_W3_INIT  25
#define WPBA_W_MAX    90
#define WPBA_W_MIN    10

/* ── FNN config ────────────────────────────────────────────── */
#define FNN_INPUTS   3
#define FNN_HIDDEN   16
#define FNN_OUTPUTS  1
#define FNN_CONFIDENCE_MIN  0ULL        /* 0ns  */
#define FNN_CONFIDENCE_MAX  5000000000ULL  /* 5s  */

/* ── Per-process record ────────────────────────────────────── */
struct eats_task_info {
    u64 predicted_ns;
    u64 last_burst_ns;
    u64 start_time_ns;
    u64 phase_avg_ns;
    int burst_count;
    int assigned_core;    /* 0=LITTLE 1=BIG -1=ANY */
    int mispredictions;
    int w1, w2, w3;      /* WPBA weights */
    int io_ratio;         /* 0-100 */
    bool fnn_used;        /* was FNN used last prediction? */
};

#endif
