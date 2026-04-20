#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "eats.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Universal WPBA Energy-Aware Scheduler");
MODULE_AUTHOR("Linux Project Team");

DEFINE_HASHTABLE(eats_table, 8);
static struct task_struct *monitor_thread;

/* WPBA History Management */
static struct eats_entry *get_or_create_entry(pid_t pid) {
    struct eats_entry *e;
    hash_for_each_possible(eats_table, e, node, pid) {
        if (e->pid == pid) return e;
    }
    e = kmalloc(sizeof(*e), GFP_ATOMIC);
    if (!e) return NULL;
    e->pid = pid;
    e->info.w1 = 33; e->info.w2 = 33; e->info.w3 = 34; 
    e->info.phase_avg = 0;
    e->info.last_prediction = EATS_SHORT_THRESH_NS;
    hash_add(eats_table, &e->node, pid);
    return e;
}

/* The WPBA Core Logic */
static void apply_wpba_logic(struct task_struct *p) {
    struct eats_entry *e;
    struct eats_task_info *info;
    u64 actual_cpu, actual_io, prediction;
    struct cpumask mask;

    e = get_or_create_entry(p->pid);
    if (!e) return;
    info = &e->info;

    actual_cpu = p->utime; 
    actual_io = p->nvcsw + p->nivcsw;

    // THE WPBA FORMULA: Dynamic Weighting
    prediction = ((info->w1 * actual_cpu) + (info->w2 * info->phase_avg) + (info->w3 * actual_io)) / 100;

    // ADAPTIVE FEEDBACK: Increase W1 if task is growing heavier than predicted
    if (actual_cpu > info->last_prediction + 1000000) {
        if (info->w1 < 90) { info->w1 += 2; info->w2 -= 1; info->w3 -= 1; }
    }

    cpumask_clear(&mask);
    if (prediction < EATS_SHORT_THRESH_NS) {
        cpumask_set_cpu(0, &mask); 
        if (p->mm && actual_cpu > 0 && p->pid > 1000) {
             pr_info("EATS WPBA: [%s] PID:%d -> LITTLE (Pred:%llu, W1:%d)\n", p->comm, p->pid, prediction, info->w1);
        }
    } else {
        cpumask_set_cpu(0, &mask); 
        if (p->mm && actual_cpu > 0 && p->pid > 1000) {
             pr_info("EATS WPBA: [%s] PID:%d -> BIG (Pred:%llu, W1:%d)\n", p->comm, p->pid, prediction, info->w1);
        }
    }
    
    if (p->mm) set_cpus_allowed_ptr(p, &mask);

    info->last_prediction = prediction;
    info->phase_avg = (info->phase_avg + actual_cpu) / 2;
}

/* Background Monitoring Thread */
static int monitor_func(void *data) {
    struct task_struct *p;
    while (!kthread_should_stop()) {
        rcu_read_lock();
        for_each_process(p) {
            if (p->pid > 1 && p->mm) {
                apply_wpba_logic(p);
            }
        }
        rcu_read_unlock();
        msleep(3000); 
    }
    return 0;
}

static int __init eats_init(void) {
    hash_init(eats_table);
    monitor_thread = kthread_run(monitor_func, NULL, "eats_monitor");
    pr_info("EATS: Universal WPBA Sentinel Active.\n");
    return 0;
}

static void __exit eats_exit(void) {
    struct eats_entry *e;
    struct hlist_node *tmp;
    int bkt;
    if (monitor_thread) kthread_stop(monitor_thread);
    hash_for_each_safe(eats_table, bkt, tmp, e, node) {
        hash_del(&e->node);
        kfree(e);
    }
    pr_info("EATS: WPBA Sentinel Shutdown.\n");
}

module_init(eats_init);
module_exit(eats_exit);
