#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <asm/msr.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Bailey <peter.eldridge.bailey@gmail.com>");
MODULE_DESCRIPTION("export aperf and mperf counters on Nehalem");

//#define perfmod_DEBUG

#define procfs_name "mperf_aperf"

struct proc_dir_entry *proc_file;

int
procfile_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
  int ret;
  
#ifdef perfmod_DEBUG
  printk(KERN_INFO "procfile_read (/proc/%s) called\n", procfs_name);
#endif
  
  /* 
   * We give all of our information in one go, so if the
   * user asks us if we have more information the
   * answer should always be no.
   *
   * This is important because the standard read
   * function from the library would continue to issue
   * the read system call until the kernel replies
   * that it has no more information, or until its
   * buffer is filled.
   */
  if (offset > 0) {
    /* we have finished to read, return 0 */
    ret  = 0;
  } else {
    u64 aperf, mperf;
    int size;

    rdmsrl(MSR_IA32_MPERF, mperf);
    rdmsrl(MSR_IA32_APERF, aperf);

#ifdef perfmod_DEBUG
    printk(KERN_ALERT "aperf: 0x%llx mperf: 0x%llx\n", aperf, mperf);
#endif

    /* fill the buffer, return the buffer size */
    size = buffer_length > 16 ? 16 : buffer_length;
    ret = snprintf(buffer, size, "HelloWorld!!!!\n");
  }

  return ret;
}

int __init init_module(void)
{
  proc_file = create_proc_entry(procfs_name, 0644, NULL);
    
  if (proc_file == NULL) {
    remove_proc_entry(procfs_name, 0);
    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	   procfs_name);
    return -ENOMEM;
  }

  proc_file->read_proc = procfile_read;
  proc_file->mode  = S_IFREG | S_IRUGO;
  proc_file->uid  = 0;
  proc_file->gid  = 0;
  proc_file->size  = 16;

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
