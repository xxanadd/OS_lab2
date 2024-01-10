#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
// #include <kernel/sched/sched.h>
#include <linux/pid.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/cpumask.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/sched/stat.h>
#include <linux/time.h>
#include <linux/time_namespace.h>
#include <linux/irqnr.h>
#include <linux/sched/cputime.h>
#include <linux/tick.h>
#include <linux/hugetlb.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/memblock.h>
#include <linux/percpu.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
// #include <linux/zswap.h>
#include <asm/page.h>

#define PROCFS_NAME "hello_world"
#define BUF_SIZE 2048

//Прототипы операций взаимодействия с procfs
static int      open_proc(struct inode *inode, struct file *file);
static int      release_proc(struct inode *inode, struct file *file);
static ssize_t  read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t  write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);

//Прототип функции создания данных для записи в фаил
static size_t write_info(char __user *ubuf);

// Объявление операций
static struct proc_ops proc_fops = {
    .proc_open = open_proc,
    .proc_read = read_proc,
    .proc_write = write_proc,
    .proc_release = release_proc
};

// static char hello_world_data[BUF_SIZE];

static int open_proc(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File opened");
    return 0;
}

static ssize_t read_proc(struct file *file, char __user *buffer, size_t count, loff_t *pos) {
    char *buf;
    size_t len = 0;

    printk(KERN_INFO "File read");

    // Allocate memory for the buffer
    buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }

    // Call write_info to fill the buffer with data
    len = write_info(buf);

    if (*pos > 0 || count < len) {
        kfree(buf); // Free the allocated memory
        return 0;
    }

    // Copy data from the kernel buffer to the user buffer
    if (copy_to_user(buffer, buf, len)) {
        kfree(buf); // Free the allocated memory
        return -EFAULT;
    }

    *pos = len;

    kfree(buf); // Free the allocated memory

    return len;
}

static ssize_t write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *pos) {
    // Запись в файл не поддерживается, возвращаем ошибку
    return -EINVAL;
}

static int release_proc(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File released");
    return 0;
}

static size_t write_info(char *buf) {
    size_t len = 0;

    /****Procs****/

    struct task_struct *task;
    int waiting_execution_count = 0;
    int waiting_io_count = 0;

    // Итерируем по всем процессам в системе
    for_each_process(task) {
        // Проверяем состояние процесса
        if (task->__state == TASK_RUNNING) {
            // Процесс ожидает выполнения
            waiting_execution_count++;
        } else if (task->__state == TASK_INTERRUPTIBLE || task->__state == TASK_UNINTERRUPTIBLE) {
            // Процесс находится в состоянии ожидания (ввода-вывода)
            waiting_io_count++;
        }
    };
    
    len += sprintf(buf, "r = %d\n",     waiting_execution_count);
    len += sprintf(buf + len, "b = %d\n", waiting_io_count);

    /****Memory****/

    struct sysinfo info;
    long cached;
    long available;
    
    si_meminfo(&info);
    // нет доступа у внешних модулей
    // si_swapinfo(&info);

    available = si_mem_available();
    
    cached = global_node_page_state(NR_FILE_PAGES) -
            total_swapcache_pages() - info.bufferram;
    if (cached < 0)
        cached = 0;
    // len += sprintf(buf + len, "total_swpd = %15lu\n",   info.totalswap << (PAGE_SHIFT - 10));
    // len += sprintf(buf + len, "free_swpd = %15lu\n",   info.freeswap << (PAGE_SHIFT - 10));
    // len += sprintf(buf + len, "swpd = %15lu\n",   (info.totalswap - info.freeswap) << (PAGE_SHIFT - 10));
    len += sprintf(buf + len, "free = %lu\n",   info.freeram << (PAGE_SHIFT - 10));
    len += sprintf(buf + len, "buff = %lu\n",   info.bufferram << (PAGE_SHIFT - 10));
    len += sprintf(buf + len, "cache = %lu\n",  cached << (PAGE_SHIFT - 10));

    /****CPU****/

    int i;
    u64 user, system;
    struct timespec64 boottime;

    user = system = 0;
    getboottime64(&boottime);
    timens_sub_boottime(&boottime);

    for_each_possible_cpu(i)
    {
        struct kernel_cpustat kcpustat;
        u64 *cpustat = kcpustat.cpustat;

        kcpustat_cpu_fetch(&kcpustat, i);

        user += cpustat[CPUTIME_USER];
        system += cpustat[CPUTIME_SYSTEM];
    };

    len += sprintf(buf + len, "sys = %lu\n",  ns_to_timespec64(system).tv_sec);
    len += sprintf(buf + len, "usr = %lu\n",  ns_to_timespec64(user).tv_sec);

    // printk(KERN_INFO "sys cpu: %lu", ns_to_timespec64(system).tv_sec);
    // printk(KERN_INFO "usr cpu: %lu", ns_to_timespec64(user).tv_sec);

    return len;
}

static int __init hello_world_init(void) {
    struct proc_dir_entry *entry;

    entry = proc_create(PROCFS_NAME, 0666, NULL, &proc_fops);

    if (!entry) {
        return -ENOMEM;
    }

    printk(KERN_INFO "Hello, World! Module Loaded\n");
    return 0;
}

static void __exit hello_world_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_INFO "Hello, World! Module Unloaded\n");
}

module_init(hello_world_init);
module_exit(hello_world_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
