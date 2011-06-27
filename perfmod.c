#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Bailey <peter.eldridge.bailey@gmail.com>");
MODULE_DESCRIPTION("export aperf and mperf counters");

int __init init_module(void)
{
  return 0;
}

void __exit cleanup_module(void)
{
}
