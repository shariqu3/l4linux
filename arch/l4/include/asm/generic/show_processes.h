#pragma once

#include <linux/sched.h>

void l4x_show_process(struct task_struct *t);
void l4x_show_processes(void);
void l4x_show_sigpending_processes(void);
