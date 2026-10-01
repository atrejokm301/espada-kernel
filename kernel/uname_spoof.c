// SPDX-License-Identifier: GPL-2.0
/*
 * Per-process uname spoofing with sysfs runtime configuration.
 * Spoofs uname() release string for targeted processes (DroidGuard, etc.).
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/uname_spoof.h>

#define SPOOF_MAX_RELEASE	256
#define SPOOF_MAX_TARGETS	16
#define SPOOF_MAX_TARGET_LEN	128

static bool spoof_on = true;
static char spoof_release[SPOOF_MAX_RELEASE] = CONFIG_UNAME_SPOOF_RELEASE;
static DEFINE_MUTEX(spoof_lock);

struct spoof_target {
	char name[SPOOF_MAX_TARGET_LEN];
	size_t len;
};

static struct spoof_target targets[SPOOF_MAX_TARGETS];
static int nr_targets;

static const char * const default_targets[] = {
	"com.google.android.gms.unstable",
	"com.google.android.apps.messaging",
	"com.google.android.apps.walletnfcrel",
	"zygote64",
};

static void init_default_targets(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(default_targets); i++) {
		strscpy(targets[i].name, default_targets[i],
			SPOOF_MAX_TARGET_LEN);
		targets[i].len = strlen(targets[i].name);
	}
	nr_targets = ARRAY_SIZE(default_targets);
}

static bool spoof_enabled(void)
{
	return READ_ONCE(spoof_on) && spoof_release[0];
}

static bool match_task_cmdline(struct task_struct *task, int n, bool pkg_only)
{
	struct mm_struct *mm;
	char buf[SPOOF_MAX_TARGET_LEN];
	unsigned long arg_start, arg_end;
	int i, len;

	mm = get_task_mm(task);
	if (!mm)
		return false;
	arg_start = mm->arg_start;
	arg_end = mm->arg_end;
	mmput(mm);

	if (!arg_start || arg_end <= arg_start)
		return false;

	len = min_t(unsigned long, arg_end - arg_start, sizeof(buf) - 1);
	if (task == current) {
		if (copy_from_user(buf, (void __user *)arg_start, len))
			return false;
	} else {
		len = access_process_vm(task, arg_start, buf, len, 0);
		if (len <= 0)
			return false;
	}
	buf[len] = '\0';

	for (i = 0; i < n; i++) {
		if (pkg_only && !strchr(targets[i].name, '.'))
			continue;
		if (!strncmp(buf, targets[i].name, targets[i].len))
			return true;
	}
	return false;
}

bool uname_spoof_should_spoof(void)
{
	struct task_struct *task, *next;
	int i, n, depth;

	if (!spoof_enabled())
		return false;

	n = READ_ONCE(nr_targets);

	for (i = 0; i < n; i++) {
		if (!strncmp(current->comm, targets[i].name, targets[i].len))
			return true;
	}

	if (match_task_cmdline(current, n, false))
		return true;

	/* Walk parent chain to catch subprocesses (app → sh → uname) */
	rcu_read_lock();
	task = rcu_dereference(current->real_parent);
	for (depth = 0; task && task->pid > 1 && depth < 3; depth++) {
		get_task_struct(task);
		rcu_read_unlock();

		if (match_task_cmdline(task, n, true)) {
			put_task_struct(task);
			return true;
		}

		rcu_read_lock();
		next = rcu_dereference(task->real_parent);
		put_task_struct(task);
		task = next;
	}
	rcu_read_unlock();

	return false;
}

const char *uname_spoof_get_release(void)
{
	if (!spoof_enabled())
		return NULL;
	return spoof_release;
}

/* sysfs interface: /sys/kernel/uname_spoof/ */

static ssize_t enabled_show(struct kobject *kobj,
			     struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", READ_ONCE(spoof_on));
}

static ssize_t enabled_store(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      const char *buf, size_t count)
{
	bool val;

	if (kstrtobool(buf, &val))
		return -EINVAL;
	WRITE_ONCE(spoof_on, val);
	return count;
}

static ssize_t release_show(struct kobject *kobj,
			     struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s\n", spoof_release);
}

static ssize_t release_store(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      const char *buf, size_t count)
{
	size_t len = count;

	if (len > 0 && buf[len - 1] == '\n')
		len--;
	if (len >= SPOOF_MAX_RELEASE)
		return -EINVAL;

	mutex_lock(&spoof_lock);
	memcpy(spoof_release, buf, len);
	spoof_release[len] = '\0';
	mutex_unlock(&spoof_lock);
	return count;
}

static ssize_t targets_show(struct kobject *kobj,
			     struct kobj_attribute *attr, char *buf)
{
	int i, len = 0, n;

	mutex_lock(&spoof_lock);
	n = nr_targets;
	for (i = 0; i < n; i++)
		len += sysfs_emit_at(buf, len, "%s\n", targets[i].name);
	mutex_unlock(&spoof_lock);
	return len;
}

static ssize_t targets_store(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      const char *buf, size_t count)
{
	struct spoof_target *new_targets;
	int new_nr = 0;
	char *input, *orig, *line;

	new_targets = kmalloc_array(SPOOF_MAX_TARGETS, sizeof(*new_targets),
				    GFP_KERNEL);
	if (!new_targets)
		return -ENOMEM;

	orig = kmemdup_nul(buf, count, GFP_KERNEL);
	if (!orig) {
		kfree(new_targets);
		return -ENOMEM;
	}
	input = orig;

	while ((line = strsep(&input, "\n")) != NULL) {
		if (!*line)
			continue;
		if (new_nr >= SPOOF_MAX_TARGETS)
			break;
		strscpy(new_targets[new_nr].name, line,
			SPOOF_MAX_TARGET_LEN);
		new_targets[new_nr].len = strlen(new_targets[new_nr].name);
		new_nr++;
	}

	mutex_lock(&spoof_lock);
	memcpy(targets, new_targets, sizeof(targets));
	WRITE_ONCE(nr_targets, new_nr);
	mutex_unlock(&spoof_lock);

	kfree(orig);
	kfree(new_targets);
	return count;
}

static struct kobj_attribute enabled_attr = __ATTR_RW(enabled);
static struct kobj_attribute release_attr = __ATTR_RW(release);
static struct kobj_attribute targets_attr = __ATTR_RW(targets);

static struct attribute *spoof_attrs[] = {
	&enabled_attr.attr,
	&release_attr.attr,
	&targets_attr.attr,
	NULL,
};

static const struct attribute_group spoof_attr_group = {
	.attrs = spoof_attrs,
};

static int __init uname_spoof_init(void)
{
	struct kobject *kobj;

	init_default_targets();

	kobj = kobject_create_and_add("uname_spoof", kernel_kobj);
	if (!kobj)
		return -ENOMEM;

	return sysfs_create_group(kobj, &spoof_attr_group);
}
late_initcall(uname_spoof_init);
