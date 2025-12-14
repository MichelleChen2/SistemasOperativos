#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define MAX_IRQ_TO_BOOST 6

#define MAX_RUNS 1  /// best primes time
#define MAX_PRIOR_DEPTH 7

#define EXPECTED_ENVS_STATS 50

struct RunStat {
	int parent_id;
	int env_id;	
};

struct SchedStats {
	int total_runs;
	int total_reboost;
	struct RunStat last_runs[EXPECTED_ENVS_STATS];
	int last_runs_index;
};

struct SchedStats Stats = {
	.total_runs = 0,
	.total_reboost = 0,
	.last_runs_index = 0,
};

void sched_halt(void);
void reboost(void);
void update_curenv(int idx);

extern int irq_count;

void
reboost(void)
{
	cprintf("LLEGO A REBOOST\n");
	Stats.total_reboost  ++;
	for (int i = 0; i < NENV; i++) {
		envs[i].env_runs = 0;
		envs[i].env_priority = 0;
	}
}

void 
update_stats(int idx){
	Stats.total_runs ++;
	struct RunStat env_stats = {
		.env_id = envs[idx].env_id,
		.parent_id = envs[idx].env_parent_id,
	};
	Stats.last_runs[Stats.last_runs_index  % EXPECTED_ENVS_STATS] = env_stats;
	Stats.last_runs_index ++;
}

/// Actualización del proceso que se ejecutó y que está por ceder la CPU
void
update_curenv(int idx)
{
	update_stats(idx);
	/// Que no tenga en cuenta al primer proceso (cuando se inician todas las ejecuciones)
	if (!curenv)
		return;

	if (envs[idx].env_runs >= MAX_RUNS) {
		envs[idx].env_runs = 0;
		envs[idx].env_priority += 1;
		if (envs[idx].env_priority >= MAX_PRIOR_DEPTH)
			reboost();
	}
}


int
get_min_idx(int min_idx, int cur_idx)
{
	struct Env *e = envs;

	if (e[cur_idx].env_priority < e[min_idx].env_priority) {
		return cur_idx;
	} else if (e[cur_idx].env_priority == e[min_idx].env_priority) {
		if (e[cur_idx].env_runs < e[min_idx].env_runs) {
			return min_idx;
		} else if (e[cur_idx].env_runs > e[min_idx].env_runs) {
			return cur_idx;
		} else {
			return min_idx;
		}
	}
	return min_idx;
}


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	if (irq_count > 0) {
		if (irq_count > MAX_IRQ_TO_BOOST) {  /// 7, 8 (2.6 - 2.8s)
			irq_count = 0;
			reboost();
		}
	}
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here - Round robin
	int idx = curenv ? ENVX(curenv->env_id) + 1 : 0;
	int cur_idx;

	for (int i = 0; i < NENV; i++) {
		cur_idx = (idx + i) % NENV;

		if (envs[cur_idx].env_status == ENV_RUNNABLE) {
			update_stats(cur_idx);
			env_run(&envs[cur_idx]);
		}
	}

	if (curenv && curenv->env_status == ENV_RUNNING) {
		update_stats(idx-1);
		env_run(curenv);
	}

	sched_halt();
#endif


#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	// Your code here - Priorities

	int cur_idx = curenv ? ENVX(curenv->env_id) : 0;
	int actual_pr = curenv ? curenv->env_priority : 0;

	/// Se busca y ejecuta al proceso con prioridad "<=" que la de curenv
	for (int i = 0; i < NENV; i++) {
		int n_idx = (cur_idx + i + 1) % NENV;
		struct Env *e = &envs[n_idx];
		if (e->env_status == ENV_RUNNABLE && e->env_priority <= actual_pr) {
			update_curenv(cur_idx);
			env_run(e);
		}
	}
	/// De no hayarlo anteriormente, indica que no hay más procesos con
	/// mínimo la prioridad de curenv por lo que se volverá a ejecutar a
	/// curenv para incrementar sus runs y eventualmente su prioridad
	if (curenv && curenv->env_status == ENV_RUNNING) {
		update_curenv(cur_idx);
		env_run(curenv);
	}
	/// Se salta de nivel de prioridad a uno "random" (al primero RUNNABLE)
	for (int i = 0; i < NENV; i++) {
		int n_idx = (cur_idx + i + 1) % NENV;
		struct Env *e = &envs[n_idx];
		if (e->env_status == ENV_RUNNABLE) {
			update_curenv(cur_idx);
			env_run(e);
		}
	}

	sched_halt();
#endif


	// Without scheduler, keep runing the last environment while it exists
	if (curenv) {
		env_run(curenv);
	}

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	cprintf("Scheduler run stats:\n"
		"        total runs: %d\n"
		"        total reboost: %d\n",
		Stats.total_runs, Stats.total_reboost);
	
	
	int envs_shown = EXPECTED_ENVS_STATS > Stats.last_runs_index? Stats.last_runs_index: EXPECTED_ENVS_STATS;

	cprintf("Last %d executions were:  \n", envs_shown);
	for (int i=0; i<envs_shown; i++){
		struct RunStat env_stat = Stats.last_runs[(Stats.last_runs_index-i)%EXPECTED_ENVS_STATS];
		cprintf("%d: PID: %d | EID: %d \n",(envs_shown -  i) ,env_stat.parent_id, env_stat.env_id);
	}

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	int i;

	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}

	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}
	


	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
