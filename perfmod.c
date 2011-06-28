#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <asm/msr.h>
#include <linux/errno.h>

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
  u64 aperf, mperf;
  
#ifdef perfmod_DEBUG
  printk(KERN_INFO "procfile_read (/proc/%s) called\n", procfs_name);
#endif

  if (offset > 0) {
    /* we have finished to read, return 0 */
    ret  = 0;
  } else if(buffer_length < 8){
    // this should be an error...
    ret = 0;
  } else if(buffer_length < 16){
    rdmsrl(MSR_IA32_MPERF, mperf);
    *((u64*)buffer) = mperf;
    ret = sizeof(u64);
  } else {
    //int size;

    rdmsrl(MSR_IA32_MPERF, mperf);
    rdmsrl(MSR_IA32_APERF, aperf);

#ifdef perfmod_DEBUG
    printk(KERN_ALERT "aperf: 0x%llx mperf: 0x%llx\n", aperf, mperf);
#endif

    *((u64*)buffer) = mperf;
    *((u64*)buffer + 1) = aperf;
    ret = 2 * sizeof(u64);
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
