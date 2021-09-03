#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/workqueue.h>

MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A kernel module that kills ptrace tracer and its tracees");

#define JIFFIES_DELAY 1
#define DONT_TRACE_WQ_NAME "dont_trace_worker"

static void periodic_routine(struct work_struct *);
static DECLARE_DELAYED_WORK(dont_trace_task, periodic_routine);
static struct workqueue_struct *wq;
static bool loaded;

/* Send SIGKILL from kernel space */
static void kill_task(struct task_struct *task)
{
    send_sig(SIGKILL, task, 1);
}

/* @return true if the process has tracees */
static bool is_tracer(struct list_head *children)
{
    struct list_head *list;
    list_for_each (list, children) {
        struct task_struct *task =
            list_entry(list, struct task_struct, ptrace_entry);
        if (task)
            return true;
    }
    return false;
}

/* Traverse the element in the linked list of the ptraced proccesses and
 * finally kills them.
 */
static void kill_tracee(struct list_head *children)
{
    struct list_head *list;
    list_for_each (list, children) {
        struct task_struct *task_ptraced =
            list_entry(list, struct task_struct, ptrace_entry);
        pr_info("ptracee -> comm: %s, pid: %d, gid: %d, ptrace: %d\n",
                task_ptraced->comm, task_ptraced->pid, task_ptraced->tgid,
                task_ptraced->ptrace);
        kill_task(task_ptraced);
    }
}

static void check(void)
{
    struct task_struct *task;
    for_each_process (task) {
        if (!is_tracer(&task->ptraced))
            continue;

        kill_tracee(&task->ptraced);
        kill_task(task); /* Kill the tracer once all tracees are killed */
    }
}

static void periodic_routine(struct work_struct *ws)
{
    if (likely(loaded))
        check();
    queue_delayed_work(wq, &dont_trace_task, JIFFIES_DELAY);
}

static int __init dont_trace_init(void)
{
    wq = create_workqueue(DONT_TRACE_WQ_NAME);
    queue_delayed_work(wq, &dont_trace_task, JIFFIES_DELAY);

    loaded = true;
    pr_info("Loaded!\n");
    return 0;
}

static void __exit dont_trace_exit(void)
{
    loaded = false;

    /* No new routines will be queued */
    cancel_delayed_work(&dont_trace_task);

    /* Wait for the completion of all routines */
    flush_workqueue(wq);
    destroy_workqueue(wq);

    pr_info("Unloaded.\n");
}

module_init(dont_trace_init);
module_exit(dont_trace_exit);
