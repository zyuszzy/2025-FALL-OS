#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE2(revstr, char __user *, str, size_t, n)
{
    char kbuf[256];  // kernel space buffer

    // copy string from user space to kernel space
    if (copy_from_user(kbuf, str, n))
        return -EFAULT;

    kbuf[n] = '\0';
    // print to kernel ring buffer
    printk(KERN_INFO "Ori: %s\n", kbuf);

    // reverse
    for(int i=0 ; i<n/2 ; i++){
        char tmp = kbuf[i];
        kbuf[i] = kbuf[n - i - 1];
        kbuf[n - i - 1] = tmp;
    }

    // print reversed string to kernel ring buffer
    printk(KERN_INFO "Rev: %s\n", kbuf);

    // copy result to user space
    if (copy_to_user(str, kbuf, n))
        return -EFAULT;

    return 0;
}