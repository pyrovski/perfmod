#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <asm/msr.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Bailey <peter.eldridge.bailey@gmail.com>");
MODULE_DESCRIPTION("export aperf and mperf counters on Nehalem");

//#define perfmod_DEBUG

#define procfs_name "mperf_aperf"

struct proc_dir_entry *proc_file;

int
procfile_read(struct file *file, char __user *buf,
	      size_t len, loff_t *ppos)
{
  int ret;
  u64 mperf_aperf[2];
  
#ifdef perfmod_DEBUG
  printk(KERN_INFO "procfile_read (/proc/%s) called\n", procfs_name);
#endif

  if (!buf || len < 2 * sizeof(u64)) {
    /* we have finished to read, return 0 */
    return 0;
  } else {
    //int size;

    rdmsrl(MSR_IA32_MPERF, mperf_aperf[0]);
    rdmsrl(MSR_IA32_APERF, mperf_aperf[1]);

#ifdef perfmod_DEBUG
    printk(KERN_ALERT "aperf: 0x%llx mperf: 0x%llx\n", mperf_aperf[0], mperf_aperf[1]);
#endif

    /*
    *((u64*)buf) = mperf;
    *((u64*)buf + 1) = aperf;
    */
    return 2 * sizeof(u64) - copy_to_user(buf, mperf_aperf, 2 * sizeof(u64));
  }
}

static const struct file_operations ops = {
  .owner = THIS_MODULE,
  .read = procfile_read
};

int __init init_module(void)
{
  proc_file = proc_create(procfs_name, 0, 0, &ops);
    
  if (proc_file == NULL) {
    remove_proc_entry(procfs_name, 0);
    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	   procfs_name);
    return -ENOMEM;
  }

#ifdef perfmod_DEBUG
  printk(KERN_INFO "/proc/%s created\n", procfs_name);
#endif
  return 0;
}

void __exit cleanup_module(void)
{
  remove_proc_entry(procfs_name, 0);
#ifdef perfmod_DEBUG
  printk(KERN_INFO "/proc/%s removed\n", procfs_name);
#endif
}
