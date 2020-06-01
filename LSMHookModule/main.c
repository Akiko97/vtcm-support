#include "scheme.h"
#include "mmu.h"
#include <linux/module.h>
#include <linux/security.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/string.h>
#include <asm/compat.h>
#include <linux/limits.h>
#include <linux/vmalloc.h>

char **argvs = NULL;
int argv_c = 0;

static int get_argv_from_bprm(struct linux_binprm *bprm)
{
	int ret = 0;
	unsigned long offset, pos;
	char *kaddr = NULL;
	struct page *page = NULL;
	char *argv = NULL;
	int i = 0;
	int argc = 0;
	int count = 0;
	argv = vzalloc(PAGE_SIZE);
	if (!bprm || !argv) {
		ret = -EFAULT;
		goto out;
	}
	argc = bprm->argc;
	pos = bprm->p;
	i = 0;
	offset = pos & ~PAGE_MASK;
	do {
		page = get_arg_page(bprm, pos, 0);
		if (!page) {
			ret = -EFAULT;
			goto out;
		}
		kaddr = kmap_atomic(page);
		for (/* Do nothing */;
		     offset < PAGE_SIZE && count < argc && i < PAGE_SIZE;
		     offset++, pos++) {
			argv[i++] = kaddr[offset];
			if (kaddr[offset] == '\0') {
				count++;
				pos++;
				memcpy(argvs[argv_c++], argv, strlen(argv) + 1);
				i = 0;
				continue;
			}
		}
		kunmap_atomic(kaddr);
		put_arg_page(page);
		offset = 0;
	} while (count < argc);
	ret = 0;
out:
	if (argv) {
		vfree(argv);
		argv = NULL;
	}
	return ret;
}

static int my_bprm_check_security(struct linux_binprm *bprm)
{
	int ret = 0;
	int i;
	int len = 0;
	int index;
	int insert_c = 4;
	char *insert[4] = {
		"-tpmdev",
		"passthrough,id=tpm0,path=/dev/vtcm1,cancel-path=/dev/null",
		"-device", "tpm-tis,tpmdev=tpm0"
	};
	if (!strcmp(bprm->filename, "/usr/libexec/qemu-kvm")) {
		printk("RUNING: %s\n", bprm->filename);
		argvs = vzalloc(bprm->argc * sizeof(char *));
		if (!argvs) {
			ret = -EFAULT;
			goto out;
		}
		for (i = 0; i < bprm->argc; i++) {
			argvs[i] = vzalloc(PAGE_SIZE);
			memset(argvs[i], 0, PAGE_SIZE);
		}
		ret = get_argv_from_bprm(bprm);
		for (i = 0; i < argv_c; i++) {
			printk("%s\n", argvs[i]);
			len += strlen(argvs[i]);
		}
		len += argv_c;
		printk("len: %d\n", len);
		bprm->p += len;
		copy_strings_kernel(insert_c, (const char *const *)insert,
				    bprm);
		bprm->argc += insert_c;
		// copy_strings_kernel(argv_c, argvs, bprm);
		// int i = 0;
		// while (i < argv_c) {
		// 	copy_strings_kernel(((i + 10 < argv_c) ? 10 : (argv_c - i)), &argvs[i], bprm);
		// 	i += 10;
		// }
		i = (argv_c % 10) ? argv_c / 10 + 1 : argv_c / 10;
		index = (--i) * 10;
		copy_strings_kernel(argv_c - index,
				    (const char *const *)(argvs + index), bprm);
		while (i > 0) {
			index -= 10;
			copy_strings_kernel(
				10, (const char *const *)(argvs + index), bprm);
			i--;
		}
		argv_c = 0;
		if (argvs) {
			for (i = 0; i < bprm->argc; i++) {
				vfree(argvs[i]);
			}
			vfree(argvs);
			argvs = NULL;
		}
	}
out:
	return ret;
}

static struct security_operations **security_ops_addr = NULL;
static struct security_operations *old_hooks = NULL;
static struct security_operations *new_hooks = NULL;

static inline void __hook(void)
{
	new_hooks = vmalloc(sizeof(struct security_operations));
	memcpy(new_hooks, old_hooks, sizeof(*new_hooks));
	new_hooks->bprm_check_security = my_bprm_check_security;
	*security_ops_addr = new_hooks;
}

static inline void __unhook(void)
{
	if (old_hooks) {
		*security_ops_addr = old_hooks;
	}
	vfree(new_hooks);
}

static int __init xsec_init(void)
{
	int retval = -EINVAL;
	security_ops_addr = (struct security_operations **)kallsyms_lookup_name(
		"security_ops");
	if (!security_ops_addr) {
		printk("no security hook heads\n");
		goto out;
	}
	printk("security ops addr is %p\n", security_ops_addr);
	old_hooks = *security_ops_addr;
	printk("security old ops is %p\n", old_hooks);
	__hook();
	printk("new hooks ops is  %p\n", &new_hooks);
	printk("security new ops is %p\n", *security_ops_addr);
	retval = 0;
out:
	return retval;
}

static void __exit xsec_exit(void)
{
	__unhook();
	return;
}

module_init(xsec_init);
module_exit(xsec_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(
	"LSM Hook Module: A Part of Cloud Environment Transparent TCM Support Mechanism");
MODULE_VERSION("0.1");
MODULE_ALIAS("LSMHookM");
MODULE_AUTHOR("Gabriel Z");
