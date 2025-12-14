#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t env = sys_getenvid();

    while (1) {
        cprintf("I am the current env de ID: %e ...\n", env);
        cprintf("My priority is %d\n", getpriority(env)); 
        cprintf("Decreasing...\n"); 
        decpriority(env, 3); 
        cprintf("Decreasing priority of the curenv. My priority now is %d\n", getpriority(env)); 
        cprintf("Decreasing...\n"); 
        decpriority(env, 1); 
        cprintf("Decreasing priority of curenv. My priority now is %d\n", getpriority(env)); 
        sys_yield(); 
    }
}
