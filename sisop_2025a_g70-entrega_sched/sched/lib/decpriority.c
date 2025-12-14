
#include <inc/lib.h>

void
decpriority(int envid, uint32_t valor_a_decrementar) 
{
    if (envid < 0) {
        cprintf("envid invalido\n"); 
        return; 
    }

    int r = sys_decpriority((envid_t) envid, valor_a_decrementar); 

    if (r == -E_BAD_ENV) {
        cprintf("No existe env\n");
    }
 
}