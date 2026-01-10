#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/string.h>

enum mode {
    PRINT,
    ADD,
    REMOVE
};

struct temp_record{
    char *data;
    struct list_head list;
};

static LIST_HEAD(temp_list);

SYSCALL_DEFINE3(tempbuf, int, mode, void __user *, data, size_t, size)
{
    struct temp_record *rec, *nextt;
    char *kbuf;
    int ret = 0;

    if(!data || size == 0)
        return -EFAULT;

    // Allocate kernel buffer
    kbuf = kmalloc(size + 1, GFP_KERNEL);
    if(!kbuf)
        return -ENOMEM;

    if(copy_from_user(kbuf, data, size)){
        kfree(kbuf);
        return -EFAULT;
    }
    kbuf[size] = '\0';

    switch (mode) {
        case ADD:
            rec = kmalloc(sizeof(*rec), GFP_KERNEL);
            if(!rec){
                kfree(kbuf);
                return -ENOMEM;
            }
            rec->data = kbuf;
            list_add_tail(&rec->list, &temp_list);
            printk(KERN_INFO "[tempbuf] Added: %s\n", rec->data);
            break;
        case REMOVE:
            list_for_each_entry_safe(rec, nextt, &temp_list, list) {
                if (strcmp(rec->data, kbuf) == 0) {
                    list_del(&rec->list);
                    printk(KERN_INFO "[tempbuf] Removed: %s\n", rec->data);
                    kfree(rec->data);
                    kfree(rec);
                    kfree(kbuf);
                    return 0;
                }
            }
            kfree(kbuf);
            return -ENOENT; //nothing to remove
        case PRINT: {
            char *out;
            size_t total_len = 0;
            size_t used = 0;

            out = kmalloc(512, GFP_KERNEL);
            if (!out) {
                kfree(kbuf);
                return -ENOMEM;
            }
            out[0] = '\0';

            list_for_each_entry(rec, &temp_list, list) {
                size_t len = strlen(rec->data);
                if (used + len + 1 >= 512)
                    break;
                if (used > 0) {
                    strcat(out, " ");
                    used++;
                }
                strcat(out, rec->data);
                used += len;
            }

            if (copy_to_user(data, out, min(size, strlen(out) + 1))) {
                kfree(out);
                kfree(kbuf);
                return -EFAULT;
            }

            printk(KERN_INFO "[tempbuf] %s\n", out);
            ret = strlen(out);
            kfree(out);
            kfree(kbuf);
            return ret;
        }

        default:
            kfree(kbuf);
            return -EINVAL;
    }

    return ret;
}