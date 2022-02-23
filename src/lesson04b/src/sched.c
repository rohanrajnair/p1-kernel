#include "sched.h"
#include "irq.h"
#include "printf.h"
#include "timer.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct * task[NR_TASKS] = {&(init_task), };
struct switch_struct context_switches[50];
int nr_tasks = 1;
int num_switches = 0;
// global vars to keep track of sp/pc
unsigned long prev_sp = 0;
unsigned long prev_pc = 0;
unsigned long next_sp = 0;
unsigned long next_pc = 0;

void preempt_disable(void)
{
	current->preempt_count++;
}

void preempt_enable(void)
{
	current->preempt_count--;
}


void _schedule(void)
{
	/* ensure no context happens in the following code region
		we still leave irq on, because irq handler may set a task to be TASK_RUNNING, which 
		will be picked up by the scheduler below */
	preempt_disable(); 
	int next,c;
	struct task_struct * p;
	while (1) {
		c = -1; // the maximum counter of all tasks 
		next = 0;

		/* Iterates over all tasks and tries to find a task in 
		TASK_RUNNING state with the maximum counter. If such 
		a task is found, we immediately break from the while loop 
		and switch to this task. */

		for (int i = 0; i < NR_TASKS; i++){
			p = task[i];
			if (p && p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next = i;
			}
		}
		if (c) {
			break;
		}

		/* If no such task is found, this is either because i) no 
		task is in TASK_RUNNING state or ii) all such tasks have 0 counters.
		in our current implemenation which misses TASK_WAIT, only condition ii) is possible. 
		Hence, we recharge counters. Bump counters for all tasks once. */
		
		for (int i = 0; i < NR_TASKS; i++) {
			p = task[i];
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}
	switch_to(task[next]);
	preempt_enable();
}

void schedule(void)
{
	current->counter = 0;
	_schedule();
}

void switch_to(struct task_struct * next) 
{
	// printing info every 50 switches
	if (num_switches == 50) {
		for (int j = 0; j < 50; j++) {
			struct switch_struct s = context_switches[j];
			printf("%d from task%d (PC: %x SP: %x) to task%d (PC: %x SP: %x)\n", s.timestamp, s.prev_pid, s.prev_pc, s.prev_sp, s.next_pid, s.next_pc, s.next_sp);
		}
		num_switches = 0;
	}
	int timestamp = elapsed_time();
	int prev_pid = current->pid;
	int next_pid = next->pid;
	if (current == next) {
		struct switch_struct a = {timestamp, prev_pid, next_pid, prev_pc, next_pc, prev_sp, next_sp};
		context_switches[num_switches++] = a; 
		return;
	}
	else {
		struct task_struct * prev = current;
		current = next;
		cpu_switch_to(prev, next);
		struct switch_struct b = {timestamp, prev_pid, next_pid, prev_pc, next_pc, prev_sp, next_sp};
		context_switches[num_switches++] = b;
	}
}

void schedule_tail(void) {
	preempt_enable();
}


void timer_tick()
{
	--current->counter;
	if (current->counter>0 || current->preempt_count >0) {
		return;
	}
	current->counter=0;
	enable_irq();
	_schedule();
	disable_irq();
}
