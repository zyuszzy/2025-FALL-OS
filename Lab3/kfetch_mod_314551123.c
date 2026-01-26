# include "kfetch.h"
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/utsname.h>
#include <linux/mm.h>
#include <linux/sched/stat.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/cdev.h>

static DEFINE_MUTEX(kfetch_mutex);
static int kfetch_mask = KFETCH_FULL_INFO;
static int major; // 儲存主裝置號
static struct class *cls;

// set up protections
// ============================================================= open ===============================================================
static int kfetch_open(struct inode *inode, struct file *file){
    if(!mutex_trylock(&kfetch_mutex)){
        pr_alert("Device is being used by another process\n");
        return -EBUSY;
    }
    return 0;
}
// ==================================================================================================================================


// clean up protections
// ============================================================ release =============================================================
static int kfetch_release(struct inode *inode, struct file *file){
    mutex_unlock(&kfetch_mutex);
    return 0;
}
// ==================================================================================================================================


// ============================================================= read ===============================================================
static ssize_t kfetch_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    // has been read
    if(*offset > 0)
        return 0;

    char *kfetch_buf;
    char info[6][128];
    int info_count = 0;
    int len = 0;    // record last writed position
    const char *logo[] = {
        "        .-.        ", 
        "       (.. |       ", 
        "       <>  |       ", 
        "      / --- \\      ", 
        "     ( |   | )     ",
        "   |\\\\_)__(_//|    ",
        "  <__)------(__>   "
    };

    struct sysinfo si;
    si_meminfo(&si);

    kfetch_buf = kmalloc(KFETCH_BUF_SIZE, GFP_KERNEL);
    if (!kfetch_buf) {
        pr_alert("kfetch: Failed to allocate memory\n");
        return -ENOMEM; 
    }

    /* fetching the information */
    if(kfetch_mask & KFETCH_RELEASE)
        snprintf(info[info_count++], 128, "Kernel: %s", utsname()->release);
    if(kfetch_mask & KFETCH_CPU_MODEL)
        snprintf(info[info_count++], 128, "CPU:    %s", "RISC-V Processor");
    if(kfetch_mask & KFETCH_NUM_CPUS)
        snprintf(info[info_count++], 128, "CPUs:   %d / %d", num_online_cpus(), num_possible_cpus());
    if(kfetch_mask & KFETCH_MEM)
        snprintf(info[info_count++], 128, "Mem:    %lu / %lu MB", (si.freeram * si.mem_unit) / 1024 / 1024, (si.totalram * si.mem_unit) / 1024 / 1024);   // si.freeram & si.totalram return nums of page
    if (kfetch_mask & KFETCH_NUM_PROCS)
        snprintf(info[info_count++], 128, "Procs:  %u", si.procs);
    if (kfetch_mask & KFETCH_UPTIME)
        snprintf(info[info_count++], 128, "Uptime: %lu mins", si.uptime / 60);

    /* connect logo & info */
    // first
    len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "                   %s\n", utsname()->nodename);
    // second (logo[0] & line---)
    len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "%s", logo[0]);
    for(int i=0 ; i<strlen(utsname()->nodename) ; i++) 
        len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "-");
    len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "\n");
    // others
    for(int i=1 ; i<7 ; i++){
        int info_idx = i - 1; 
        if (info_idx < info_count){
            len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "%s%s\n", logo[i], info[info_idx]);
        }else{
            len += snprintf(kfetch_buf + len, KFETCH_BUF_SIZE - len, "%s\n", logo[i]);
        }
    }
    

    if (copy_to_user(buffer, kfetch_buf, len)) {
        kfree(kfetch_buf);
        pr_alert("Failed to copy data to user");
        return -EFAULT;
    }

    *offset += len;
   
    /* cleaning up */
    kfree(kfetch_buf);

    return len;
}
// ==================================================================================================================================


// ============================================================= write ===============================================================
static ssize_t kfetch_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return -EFAULT;
    }

    /* setting the information mask */
    kfetch_mask = mask_info;

    return length;
}
// ==================================================================================================================================

const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

static int __init kfetch_init(void) {
    major = register_chrdev(0, "kfetch", &kfetch_ops);
    cls = class_create("kfetch_class");
    device_create(cls, NULL, MKDEV(major, 0), NULL, "kfetch");
    return 0;
}

static void __exit kfetch_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "kfetch");
}

module_init(kfetch_init);
module_exit(kfetch_exit);

MODULE_LICENSE("GPL"); 