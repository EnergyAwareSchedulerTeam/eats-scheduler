#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include "eats.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EATS Team");
MODULE_DESCRIPTION("Energy-Aware Task Scheduler — FNN + WPBA");

/* ── FNN hint table ─────────────────────────────────────────── */
#define FNN_HINT_SIZE 256
static struct {
    pid_t pid;
    u64   predicted_ns;
    int   valid;
} fnn_hints[FNN_HINT_SIZE];
static DEFINE_SPINLOCK(hint_lock);

/* ── /proc/eats_hints ───────────────────────────────────────── */
static ssize_t eats_proc_write(struct file *f, const char __user *buf,
                                size_t len, loff_t *off)
{
    char *kbuf, *line, *rest;
    pid_t pid;
    u64 pred;
    int i;

    kbuf = kmalloc(len + 1, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;
    if (copy_from_user(kbuf, buf, len)) { kfree(kbuf); return -EFAULT; }
    kbuf[len] = '\0';

    rest = kbuf;
    while ((line = strsep(&rest, "\n")) != NULL) {
        if (sscanf(line, "%d %llu", &pid, &pred) != 2) continue;
        spin_lock(&hint_lock);
        for (i = 0; i < FNN_HINT_SIZE; i++) {
            if (!fnn_hints[i].valid || fnn_hints[i].pid == pid) {
                fnn_hints[i].pid          = pid;
                fnn_hints[i].predicted_ns = pred;
                fnn_hints[i].valid        = 1;
                break;
            }
        }
        spin_unlock(&hint_lock);
    }

    kfree(kbuf);
    return len;
}

static ssize_t eats_proc_read(struct file *f, char __user *buf,
                               size_t len, loff_t *off)
{
    char kbuf[64];
    int n = snprintf(kbuf, sizeof(kbuf), "EATS: running\n");
    if (*off >= n) return 0;
    if (copy_to_user(buf, kbuf, n)) return -EFAULT;
    *off += n;
    return n;
}

static const struct proc_ops eats_proc_ops = {
    .proc_write = eats_proc_write,
    .proc_read  = eats_proc_read,
};

/* ── Hash table ─────────────────────────────────────────────── */
DEFINE_HASHTABLE(eats_table, 8);
struct eats_entry {
    pid_t pid;
    struct eats_task_info info;
    struct hlist_node node;
};

static struct eats_entry *get_entry(pid_t pid)
{
    struct eats_entry *e;
    hash_for_each_possible(eats_table, e, node, pid)
        if (e->pid == pid) return e;

    e = kmalloc(sizeof(*e), GFP_ATOMIC);
    if (!e) return NULL;
    e->pid                 = pid;
    e->info.predicted_ns   = 20000000ULL;
    e->info.last_burst_ns  = 0;
    e->info.start_time_ns  = 0;
    e->info.phase_avg_ns   = 0;
    e->info.burst_count    = 0;
    e->info.assigned_core  = -1;
    e->info.mispredictions = 0;
    e->info.w1             = WPBA_W1_INIT;
    e->info.w2             = WPBA_W2_INIT;
    e->info.w3             = WPBA_W3_INIT;
    e->info.io_ratio       = 50;
    e->info.fnn_used       = false;
    hash_add(eats_table, &e->node, pid);
    return e;
}

/* ── WPBA predictor ─────────────────────────────────────────── */
static u64 wpba_predict(struct eats_task_info *info)
{
    u64 last  = info->last_burst_ns ? info->last_burst_ns : info->predicted_ns;
    u64 phase = info->phase_avg_ns  ? info->phase_avg_ns  : last;
    u64 hint  = (u64)(info->io_ratio) * 1000000ULL;
    int total = info->w1 + info->w2 + info->w3;
    if (total == 0) total = 100;
    return ((u64)info->w1 * last  +
            (u64)info->w2 * phase +
            (u64)info->w3 * hint) / (u64)total;
}

/* ── Core assignment ────────────────────────────────────────── */
static void assign_core(struct task_struct *task, struct eats_task_info *info)
{
    cpumask_t mask;
    const char *engine, *core_name;
    u64 fnn_hint = 0;
    int i;

    spin_lock(&hint_lock);
    for (i = 0; i < FNN_HINT_SIZE; i++) {
        if (fnn_hints[i].valid && fnn_hints[i].pid == task->pid) {
            fnn_hint = fnn_hints[i].predicted_ns;
            break;
        }
    }
    spin_unlock(&hint_lock);

    if (fnn_hint > 0) {
        info->predicted_ns = fnn_hint;
        info->fnn_used     = true;
        engine             = "FNN";
    } else {
        info->predicted_ns = wpba_predict(info);
        info->fnn_used     = false;
        engine             = "WPBA";
    }

    cpumask_clear(&mask);
    if (info->predicted_ns <= EATS_LITTLE_THRESH_NS) {
        cpumask_set_cpu(0, &mask);
        cpumask_set_cpu(1, &mask);
        info->assigned_core = 0;
        core_name = "LITTLE";
    } else if (info->predicted_ns >= EATS_BIG_THRESH_NS) {
        cpumask_set_cpu(2, &mask);
        cpumask_set_cpu(3, &mask);
        info->assigned_core = 1;
        core_name = "BIG";
    } else {
        cpumask_copy(&mask, cpu_online_mask);
        info->assigned_core = -1;
        core_name = "ANY";
    }

    set_cpus_allowed_ptr(task, &mask);
    pr_info("EATS [%s]: [%s] PID:%d -> %s (Pred:%llu ns)\n",
            engine, task->comm, task->pid,
            core_name, info->predicted_ns);
}

/* ── Timer — runs every 2 seconds ───────────────────────────── */
static struct timer_list eats_timer;

static void eats_timer_fn(struct timer_list *t)
{
    struct task_struct *task;

    /* Walk all processes and assign cores based on predictions */
    for_each_process(task) {
        struct eats_entry *e;
        if (task->pid == 0) continue;
        e = get_entry(task->pid);
        if (e) assign_core(task, &e->info);
    }

    /* Reschedule every 2 seconds */
    mod_timer(&eats_timer, jiffies + msecs_to_jiffies(2000));
}

/* ── proc entry ─────────────────────────────────────────────── */
static struct proc_dir_entry *eats_proc;

/* ── Init / Exit ────────────────────────────────────────────── */
static int __init eats_init(void)
{
    hash_init(eats_table);
    memset(fnn_hints, 0, sizeof(fnn_hints));

    eats_proc = proc_create("eats_hints", 0666, NULL, &eats_proc_ops);
    if (!eats_proc) {
        pr_err("EATS: failed to create /proc/eats_hints\n");
        return -ENOMEM;
    }

    timer_setup(&eats_timer, eats_timer_fn, 0);
    mod_timer(&eats_timer, jiffies + msecs_to_jiffies(2000));

    pr_info("EATS: Loaded — timer-based, FNN+WPBA, 2s interval\n");
    return 0;
}

static void __exit eats_cleanup(void)
{
    struct eats_entry *e;
    struct hlist_node *tmp;
    int bkt;

    timer_delete_sync(&eats_timer);
    proc_remove(eats_proc);

    hash_for_each_safe(eats_table, bkt, tmp, e, node) {
        hash_del(&e->node);
        kfree(e);
    }
    pr_info("EATS: Unloaded.\n");
}

module_init(eats_init);
module_exit(eats_cleanup);

